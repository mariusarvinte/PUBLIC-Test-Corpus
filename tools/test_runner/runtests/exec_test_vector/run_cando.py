# © 2026 Massachusetts Institute of Technology
# MIT License

import logging, json, os
from typing import Optional
from tempfile import NamedTemporaryFile
from enum import Enum
from dataclasses import dataclass
from pathlib import Path

from runtests.test_vector import CANDO_VECTOR_FILE
from runtests.types import BenchMode, CommandError
from .parse_volume import Context
from ..utils import CONTAINER_OUTPUT_DIR, run_command
from .error import *

logger = logging.getLogger(__name__)

CWD_FILE_ENV = (
    "WORK_DIR"  # Specified by setup scripts to say what directory to run cando in
)
CANDO_DEFAULT_RUN_DIR = Path("/tmp/runtime")


class CandoReturnCode(Enum):
    """Matches Rust cando implementation"""

    Success = 0
    VectorRunFailure = 1
    ArtifactNotFound = 2
    SymbolNotFound = 3
    Usage = 4
    CandoFailure = 5
    BenchFailure = 6


def _fmt_err(stdout: str | None, stderr: str | None, msg: str | None = None) -> str:
    stdout = stdout if stdout else ""
    stderr = stderr if stderr else ""
    out = f"Stdout: {stdout}\nStderr: {stderr}"
    if msg:
        out = f"{msg}:\n{out}"
    return out


class CandoTestResult(Enum):
    """Vector result type directly from Rust cando implementation"""

    SKIP = "Skip"
    NO_COMPARE = "NoCompare"
    PASS = "Pass"
    VECTOR_COMPARISON_FAILED = "VectorComparisonFailed"
    PANIC = "Panic"
    SEGMENTATION_FAULT = "SegmentationFault"
    TIMEOUT = "Timeout"
    BENCHMARK = "Benchmark"
    UNKNOWN_FAILURE = "UnknownFailure"

    def to_container_err(
        self, stdout: Optional[str], stderr: Optional[str], msg: Optional[str] = None
    ) -> Optional[BaseFail]:
        """Converts the Rust cando error representation to a Python exception that can be raised"""
        match self:
            case CandoTestResult.SKIP:
                return Skip(msg)
            case CandoTestResult.PANIC:
                return Panic(msg)
            case CandoTestResult.SEGMENTATION_FAULT:
                return SegmentationFault(msg)
            case CandoTestResult.TIMEOUT:
                # Cando won't be handling timeouts (will be done at the process level (when running cando))
                return InternalError(
                    _fmt_err(
                        stdout, stderr, "Got Timeout from Cando which was not expected"
                    )
                )
            case CandoTestResult.UNKNOWN_FAILURE:
                return UnknownFailure(msg)
            case _:
                # Anything else (e.g., PASS or No_COMPARE) we should move on to the filesystem state checking
                return None


@dataclass
class CommonOutput:
    """Output captured from a test run."""

    stdout: str
    stderr: str


@dataclass
class CandoTestOutcome:
    """
    Mirros the cando `TestOutcome` enum but uses `result_type` to
    indicate the variant

    Attributes:
        result_type: The type of test result (Skip, Pass, Panic, etc.)
        common_out: Common stdout/stderr output (present for most result types)
        diff: Diff string (only for VectorComparisonFailed)
        wait_status: Wait status code (only for UnknownFailure)
        vector_data: Raw test vector data (only for NoCompare with Some(vector))
    """

    result_type: CandoTestResult
    common_out: Optional[CommonOutput] = None
    diff: Optional[str] = None
    wait_status: Optional[int] = None
    vector_data: Optional[dict] = None

    @classmethod
    def from_cando_json(cls, cando_json: dict) -> "CandoTestOutcome":
        """
        Initializes the `CandoTestOutcome` struct from the JSON output of cando

        Raises:
            InternalError: if the given JSON is invalid
        """
        if len(cando_json) != 1:
            raise InternalError(
                f"JSON returned from cando has too many entries: {cando_json}"
            )

        # We know that the length the dict is 1 so this is fine
        key = list(cando_json.keys())[0]
        result = cando_json[key]
        result_type = CandoTestResult(result.get("result"))

        common_out = None
        # Valgrind has it's output as a list so don't try to get these for benchmarking
        # as they won't be there anyway
        if (
            "output" in result
            and result["output"] is not None
            and result_type != CandoTestResult.BENCHMARK
        ):
            common_out = CommonOutput(
                stdout=result["output"].get("stdout", ""),
                stderr=result["output"].get("stderr", ""),
            )

        return cls(
            result_type,
            common_out,
            result.get("diff"),
            result.get("wait_status"),
            result.get("output") if result_type == CandoTestResult.NO_COMPARE else None,
        )

    def is_failure(self) -> bool:
        """Returns True if this result represents a test failure."""
        return self.result_type in [
            CandoTestResult.VECTOR_COMPARISON_FAILED,
            CandoTestResult.PANIC,
            CandoTestResult.SEGMENTATION_FAULT,
            CandoTestResult.TIMEOUT,
            CandoTestResult.UNKNOWN_FAILURE,
        ]

    def to_dict(self) -> dict:
        return {
            "result_type": self.result_type.value,
            "output": (
                {"stdout": self.common_out.stdout, "stderr": self.common_out.stderr}
                if self.common_out
                else None
            ),
            "diff": self.diff,
            "wait_status": self.wait_status,
            "vector_data": self.vector_data,
        }


def setup(ctx: Context) -> Path:
    """
    Run setup script defined in test vector if there is one else `CANDO_DEFAULT_RUN_DIR`

    Returns:
        The path specified by the setup script as the cwd for this vector

    Raises:
        InternalError: error running setup file
        FileNotFoundError: setup script set the cwd for cando but it doesn't exist
    """
    # We will always create the default runtime directory for cando even if
    # before we call setup so we can't determine if the setup script will
    # set a different working directory. This means that the setup scripts
    # then don't have to manually create /tmp/runtime evertime
    CANDO_DEFAULT_RUN_DIR.mkdir(parents=True)

    setup_file = ctx.test_vector.setup
    if not setup_file:
        return CANDO_DEFAULT_RUN_DIR

    with NamedTemporaryFile("r+") as cwd_out:
        try:
            # Pass the environment variable so setup can write cwd to file
            env = os.environ.copy()
            env["DEFAULT_RUN_DIR"] = str(
                CANDO_DEFAULT_RUN_DIR
            )  # The default directory cando gets run in
            # Vectors can write to the file specified in `WORKING_DIR_FILE` if they want to specify
            # a different working directory to run cando in
            env["WORKING_DIR_FILE"] = cwd_out.name
            if ctx.test_vector.test_resources is not None:
                env["TEST_RESOURCES"] = str(ctx.test_vector.test_resources)

            res = run_command([setup_file], env=env, capture_output=True)
            if res.returncode != 0:
                logger.error(
                    "Setup file `%s` failed with exit code %s (DEFAULT_RUN_DIR=%s, WORKING_DIR_FILE=%s)",
                    setup_file,
                    res.returncode,
                    env["DEFAULT_RUN_DIR"],
                    env["WORKING_DIR_FILE"],
                )
                logger.error("Setup stdout:\n%s", res.stdout or "")
                logger.error("Setup stderr:\n%s", res.stderr or "")
                raise InternalError(
                    _fmt_err(
                        res.stdout,
                        res.stderr,
                        msg=f"Failed to run setup file `{setup_file}` (exit code {res.returncode})",
                    )
                )
        except CommandError as e:
            raise InternalError(
                f"Failed to run setup file `{setup_file}`. Got CommandError: {e}"
            )

        # Check if the setup script specifies a cwd
        cwd_out.seek(0)
        cwd = cwd_out.readlines()
        if not cwd:
            return CANDO_DEFAULT_RUN_DIR

        # Do some integrity checking
        if len(cwd) > 1:
            raise InternalError("Setup script didn't properly write cwd")

        cwd = Path(cwd[0].strip())
        if not cwd.exists():
            raise FileNotFoundError(
                f"Couldn't find path `{cwd}` specified by setup script"
            )
        if not cwd.is_dir():
            raise FileNotFoundError(
                f"Path `{cwd}` specified by setup script isn't a directory"
            )

        return cwd


def run_cando(
    ctx: Context,
    rust: bool,
    lib: bool,
    timeout: Optional[float],
    bench_mode: Optional[BenchMode],
) -> CandoTestOutcome:
    """Runs cando on the current test vector"""
    logging.info("Running cando")
    cwd = setup(ctx)

    # Cando will write it's result to the output volume
    cando_out = CONTAINER_OUTPUT_DIR / CANDO_VECTOR_FILE
    cmd = [
        ctx.runner,
        "--log-level",
        "none",
        "--test-root-dir",
        ctx.project.parent,
        "--output",
        cando_out,
        "-v",
        ctx.test_vector.cando_vector,
    ]
    if rust:
        cmd.append("--rust")
    if bench_mode is not None:
        cmd.extend(["--bench", bench_mode.value])
    cmd.append("lib" if lib else "bin")

    try:
        res = run_command(
            cmd=cmd,
            cwd=cwd,
            capture_output=True,
            timeout=timeout,
        )
    except CommandError as e:
        if not e.is_timeout:
            # If it's not a timeout error then it's a catastrophic internal error
            raise InternalError(e)
        raise Timeout(e)

    try:
        cando_rc = CandoReturnCode(res.returncode)
    except ValueError:
        raise InternalError(f"Got invalid return code from cando: {res.returncode}")

    match cando_rc:
        case CandoReturnCode.BenchFailure:
            raise BenchmarkFailure(f"Got benchmarking failure from cando")
        case CandoReturnCode.Usage | CandoReturnCode.CandoFailure:
            if bench_mode == BenchMode.RUNTIME:
                raise InternalError(
                    f"Got error during runtime benchmarking: \n{res.stderr}.\n Are you sudo and/or did you set perf_event_paranoid?"
                )
            else:
                raise InternalError(
                    _fmt_err(res.stdout, res.stderr, msg="Cando internal error")
                )
        case CandoReturnCode.ArtifactNotFound:
            raise ArtifactNotFound(_fmt_err(res.stdout, res.stderr))
        case CandoReturnCode.SymbolNotFound:
            raise SymbolNotFound(_fmt_err(res.stdout, res.stderr))

    with open(cando_out) as f:
        cando_json = json.load(f)
    cando_outcome = CandoTestOutcome.from_cando_json(cando_json)
    logger.debug(f"Got cando outcome: {cando_outcome}")

    stdout = cando_outcome.common_out.stdout if cando_outcome.common_out else ""
    stderr = cando_outcome.common_out.stderr if cando_outcome.common_out else ""

    # Raise catastrophic errors (see to_container_err) so they skip the rest of the comparison checking
    err = cando_outcome.result_type.to_container_err(stdout, stderr)
    if err:
        raise err

    return cando_outcome
