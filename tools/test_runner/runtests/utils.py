# © 2026 Massachusetts Institute of Technology
# MIT License

import re, subprocess, os, platform, shlex, logging, signal
from collections.abc import Sequence
from pathlib import Path
from typing import Optional, Union, Mapping
from .types import CommandResult, CommandError

logger = logging.getLogger(__name__)

SNIPPET_LEN = 800
RUNTIME_TRIALS = 5

C_TEST_CASE_DIR = "test_case"
RUST_TEST_CASE_DIR = "translated_rust"
TEST_VECTORS_DIR = "test_vectors"
CANDO_RUNNER_DIR = "runner"
CANDO_RUNNER_FILE = "cando_runner"
CMAKE_TARGET_DIR = "build-ninja"

CONTAINER_VOLUME_DIR = Path("/workspace")
CONTAINER_OUTPUT_DIR = Path("/output")
CONTAINER_LOG_LABEL_ENV = "RUNTESTS_VECTOR_LABEL"
# The label attached to all of the test vector containers so the 
# shutdown handler knows which containers to cleanup
CONTAINER_LABEL_NAME = "tractor.runtests.test_vector"

CONTAINER_FALCO_LOG_DIR = Path("/var/log/falco")
FALCO_LOG_FILE = "file_changes.log"


def _fmt_cmd(cmd: list[str]) -> str:
    """Return a shell-escaped preview string for an argv-style command list."""
    return " ".join([shlex.quote(str(arg)) for arg in cmd])


def _was_user_interrupt(returncode: int, stderr: Optional[str]) -> bool:
    """Checks if returncode/stderr indicate that a user interrupt occurred"""
    sig_num = returncode - 128
    try:
        sig_name = signal.Signals(sig_num)
    except ValueError:
        sig_name = None

    if sig_name is not None and sig_name in (
        signal.SIGINT,
        signal.SIGTERM,
        signal.SIGQUIT,
        signal.SIGHUP,
    ):
        return True

    if stderr is None:
        return False

    return "user interrupt" in stderr.lower()


def run_command(
    cmd: Sequence[Union[str, Path]],
    cwd: Optional[Union[str, Path]] = None,
    stdin: Optional[Union[str, bytes]] = None,
    capture_output: bool = False,
    merge_stderr: bool = False,
    check: bool = False,
    env: Optional[Mapping[str, str]] = None,
    timeout: Optional[float] = None,
) -> CommandResult:
    """Run a subprocess with consistent logging, capture, and error handling.

    Parameters
    ----------
    cmd : list[str|Path]
        Program and arguments (argv style). Paths are stringified safely.
    cwd : str|Path|None
        Working directory for the child process.
    verbose : bool
        If True, prints a one-line `[run]` log (and capture previews).
    stdin : str|bytes|None
        Input to send to the process. Bytes are decoded as UTF-8 with 'replace'.
    capture_output : bool
        If True, capture stdout/stderr as text; else inherit parent's stdio.
    merge_stderr : bool
        If True and capturing, merge stderr into stdout; stderr will be None in the result.
    check : bool
        If True, raise CommandError on non-zero exit.
    timeout : float|None
        Kill the process if it runs longer than this many seconds.
    env : Mapping[str,str]|None
        Additional/overridden environment variables for the child.

    Returns
    -------
    CommandResult
        Completed process info including return code and any captured output.

    Raises
    ------
    CommandError
        On timeout, spawn error, or (if check=True) non-zero exit code.
    """
    cmd_list = [str(arg) for arg in cmd]
    cwd_str = str(cwd) if isinstance(cwd, Path) else cwd

    if capture_output:
        stdout_opt = subprocess.PIPE
        stderr_opt = subprocess.STDOUT if merge_stderr else subprocess.PIPE
    else:
        stdout_opt = None
        stderr_opt = None if not merge_stderr else subprocess.STDOUT

    if isinstance(stdin, bytes):
        try:
            stdin = stdin.decode("utf-8", "replace")
        except Exception:
            stdin = stdin.decode(errors="replace")

    where = f" (cwd = {cwd_str})" if cwd_str else ""
    logger.debug(f"[run] {_fmt_cmd(cmd_list)}{where}")

    completed = None
    try:
        completed = subprocess.run(
            cmd_list,
            cwd=cwd_str,
            input=stdin,
            text=True,  # ensure stdout/stderr are str, not bytes
            stdout=stdout_opt,
            stderr=stderr_opt,
            env=env,
            timeout=timeout,
            check=False,  # we raise manually below if check=True
        )
    except subprocess.TimeoutExpired as te:
        raise CommandError(
            cmd_list,
            message=f"Timed out after {timeout}s: {_fmt_cmd(cmd_list)}",
            timeout=True,
        ) from te
    except Exception as e:
        raise CommandError(
            cmd_list, message=f"Command Failed {_fmt_cmd(cmd_list)} {repr(e)}"
        ) from e

    out = completed.stdout if capture_output else None
    err = None if merge_stderr else (completed.stderr if capture_output else None)

    if capture_output:

        def _preview(label: str, data: Optional[str]):
            if data is None:
                logger.debug(f"[run] {label}: <None>")
                return
            snippet = data[:SNIPPET_LEN] + (
                "... [truncated]\n" if len(data) > SNIPPET_LEN else ""
            )
            logger.debug(f"[run] {label} preview:\n{snippet.encode('utf-8')}")

        _preview("stdin", stdin)
        _preview("stdout", out)
        if not merge_stderr:
            _preview("stderr", err)

    if check and completed.returncode != 0:
        # If this is from some sort of interrupt don't log it as an error.
        if _was_user_interrupt(completed.returncode, completed.stderr):
            message = f"Command received user interrupt: {_fmt_cmd(cmd_list)}"
        else:
            message = f"Command exited {completed.returncode}: {_fmt_cmd(cmd_list)}"
            logger.error(f"{message}\nStderr: {completed.stderr}")
        raise CommandError(cmd_list, message=message)

    return CommandResult(
        cmd=cmd_list,
        cwd=cwd_str,
        returncode=completed.returncode,
        stdout=out,
        stderr=err,
    )


def regex_match(pattern: str, text: str) -> bool:
    """Return True if the pattern fully matches the input text"""
    compiled = re.compile(pattern, flags=re.MULTILINE)
    return bool(compiled.fullmatch(text))


def get_lib_target_name(rel_name: str) -> str:
    """Returns the name of the cando lib runner executable for a given test case name"""
    return f"_{Path(rel_name).name.replace('_lib', '_cando_librunner')}"


def find_asan_library() -> Optional[str]:
    """Find libclang_rt.asan-<arch>.so via or clang."""

    lib_name = f"libclang_rt.asan-{platform.machine()}.so"
    try:
        result = subprocess.run(
            ["clang", f"-print-file-name={lib_name}"],
            capture_output=True,
            text=True,
            check=True,
        )
        # clang echoes the bare filename back when it can't find the file.
        path = result.stdout.strip()
        if path and path != lib_name and os.path.isfile(path):
            return path
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass

    return None

def handle_common_invalid_args(args):
    """Make sure that input arguments don't contain invalid combinations"""
    if args.no_compare:
        opt = None
        if args.rust:
            opt = "--rust"
        elif args.bench:
            opt = "--bench"
        if opt is not None:
            logger.warning(f"Using `{opt}` and `--no-compare` probably isn't what you want")
