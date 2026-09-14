# © 2026 Massachusetts Institute of Technology
# MIT License

import json
import shlex
from enum import Enum
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from runtests.orchestrator.error import InternalError, ExitCode


# Duplicate definition to maintain parallel commits
def _fmt_cmd(cmd: list[str]) -> str:
    """Return a shell-escaped preview string for an argv-style command list."""
    return " ".join([shlex.quote(str(arg)) for arg in cmd])


@dataclass
class BuildContext:
    """Context needed for building - must be picklable."""

    cmake: Path
    cargo: Path
    jobs: Optional[int]
    timeout: Optional[float]
    skip_lib_tests: bool
    fuzz: bool
    asan: bool
    rust: bool


@dataclass
class CommandResult:
    """Lightweight record of a completed subprocess.

    Attributes
    ----------
    cmd : list[str]
        Command (argv list) that was executed.
    cwd : str|None
        Working directory used for the subprocess.
    returncode : int
        Process exit status.
    stdout : str|None
        Captured stdout if capture_output=True, else None.
    stderr : str|None
        Captured stderr if capture_output=True and merge_stderr=False, else None.
    """

    cmd: list[str]
    cwd: Optional[str]
    returncode: int
    stdout: Optional[str]
    stderr: Optional[str]


class CommandError(RuntimeError):
    """Simple wrapper error for command failures"""

    is_timeout: bool = False

    def __init__(self, cmd, message: Optional[str] = None, timeout: bool = False):
        msg = message or f"Command failed: {_fmt_cmd(cmd)}"
        self.is_timeout = timeout
        super().__init__(msg)


@dataclass
class TestCase:
    test_root: Path  # Case root (contains test_vectors/, runner/, etc.)
    repo_root: Path
    rel_name: str
    is_library: bool

    # Build context for the test case
    build_dir: Path  # Where to run cmake/cargo for the test case
    target_dir: Path  # cmake binary dir OR cargo --target-dir (parent of release)

    # Runtime context for exec runner
    runtime_bin_dir: Path  # Directory that contains 'driver'

    # Lib harness context
    runner_dir: Path  # case_root / runner (Cando)


class VectorExitCode(Enum):
    """
    Exit code used within the container to signal the result of running a test vector.
    We're also using this as an easy way to signal the result of a test vector run in the junit
    """

    PASS = 0
    SKIP = 1
    NO_COMPARE = 2
    COMPARISON_FAIL = 3
    PANIC = 4
    SEGMENTATION_FAULT = 5
    ARTIFACT_NOT_FOUND = 6
    SYMBOL_NOT_FOUND = 7
    TIMEOUT = 8
    BENCHMARK = 9
    BENCHMARK_FAILURE = 10
    UNKNOWN_FAILURE = 11
    INTERNAL_ERROR = 12

    def is_fail(self) -> bool:
        """Returns True if the result represents an actual fail (not Pass, Skip or NoCompare)"""
        return self not in [
            VectorExitCode.PASS,
            VectorExitCode.SKIP,
            VectorExitCode.NO_COMPARE,
            VectorExitCode.BENCHMARK,
        ]


class VectorResult:
    """Information about the result of a test vector from the exec_test_vector container"""
    exit_code: VectorExitCode
    cando_output: Optional[dict]

    def __init__(self, exit_code: VectorExitCode, cando_output: Optional[dict]) -> None:
        self.exit_code = exit_code
        self.cando_output = cando_output

    @classmethod
    def from_path(cls, exit_code: VectorExitCode, path: Optional[Path]) -> 'VectorResult':
        try:
            if path is None:
                output = None
            else:
                with open(path, "r") as f:
                    output = json.load(f)
        except FileNotFoundError:
            output = None
        except json.JSONDecodeError as e:
            raise InternalError(f"Malformed JSON output from cando: {e}")
        return cls(exit_code, output)

    @classmethod
    def from_output(cls, exit_code: VectorExitCode, output: Optional[dict]) -> 'VectorResult':
        return cls(exit_code, output)

    @classmethod
    def skipped(cls) -> 'VectorResult':
        return cls(VectorExitCode.SKIP, None)


class BenchMode(Enum):
    MEMORY = "memory"
    RUNTIME = "runtime"

    def get_bin_name(self) -> str:
        match self:
            case BenchMode.MEMORY:
                return "valgrind"
            case BenchMode.RUNTIME:
                return "perf"

@dataclass
class JUnitCase:
    """Representation of the the data in the junit XML output"""

    name: str
    ok: bool
    message: str = ""
    skipped: bool = False
    error: bool = False
    vector_result: Optional[VectorResult] = None


@dataclass
class RunReport:
    """Full report of what happened during the run"""

    totals: dict
    junit_suites: dict[str, list[JUnitCase]]
    failures: list[tuple[str, str]]  # (case_name, message)

    @property
    def exit_code(self) -> ExitCode:
        return (
            ExitCode.SUCCESS if len(self.totals["failed_cases"]) == 0 else ExitCode.FAIL
        )
