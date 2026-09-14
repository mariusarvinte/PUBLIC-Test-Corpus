# © 2026 Massachusetts Institute of Technology
# MIT License

from pathlib import Path
import logging, shutil
from typing import NoReturn, Optional

from ..types import BenchMode, ExitCode
from .error import CliError, InternalError
from .discovery import collect_cases
from .cli import parse_args
from .run_phases import run_all_phases, BuildContext
from .falco import FalcoManager
from .reporters import finalize_reporting
from .shutdown_handler import shutdown_handler

logger = logging.getLogger(__name__)
PROJECT_DIRS = ["Hidden-Tests", "Public-Tests"]


def check_binaries(bench_mode: Optional[BenchMode]) -> tuple[Path, Path]:
    """
    Ensures that user has `cmake`, `ninja`, and `cargo` in PATH
    Additionally the appropriate binary for benchmarking (either perf or valgring)

    Returns:
        Tuple of paths to cmake and cargo in that order
        Tuple of a path to those binaries in that order
    """

    def tool_error(tool: str) -> NoReturn:
        raise InternalError(f"Required tool '{tool}' not found in PATH")

    def check_bin(name: str) -> Path:
        binary = shutil.which(name)
        if binary is None:
            tool_error(name)
        return Path(binary)

    cmake = check_bin("cmake")
    _ = check_bin("ninja")
    cargo = check_bin("cargo")

    if bench_mode is not None:
        _ = check_bin(bench_mode.get_bin_name())

    return (Path(cmake), Path(cargo))


def main() -> ExitCode:
    shutdown_handler.install()

    try:
        args = parse_args()
    except CliError as e:
        return e.exit_code

    logging.basicConfig(level=args.log_level)

    root = args.root.resolve()
    if not args.subset:
        args.subset = [root / top for top in PROJECT_DIRS]

    test_cases = collect_cases(root, args.subset, args.match_regex, args.rust)

    if args.asan:
        skipped_cases = [
            tc for tc in test_cases if "P02_libgit2" in Path(tc.rel_name).parts
        ]
        if skipped_cases:
            logger.warning(
                "ASan is not compatible with P02_libgit2; skipping %d case(s): %s",
                len(skipped_cases),
                ", ".join(tc.rel_name for tc in skipped_cases),
            )
            test_cases = [tc for tc in test_cases if tc not in skipped_cases]

    # Process --list flag, ignoring other flags if present
    if args.list:
        for c in test_cases:
            print(c.rel_name)
        return ExitCode.SUCCESS

    if not test_cases:
        logger.error("No test cases found")
        return ExitCode.FAIL

    # Process --clean flag, ignoring other flags if present
    if args.clean:
        for c in test_cases:
            try:
                shutil.rmtree(c.target_dir)
                logger.info(f"Deleted {c.target_dir}")
            except FileNotFoundError:
                pass
        return ExitCode.SUCCESS

    found_case_count = len(test_cases)
    logger.info(
        f"Found {found_case_count} test case{'s' if found_case_count != 1 else ''}"
    )

    bench_mode = BenchMode(args.bench) if args.bench else None
    cmake, cargo = check_binaries(bench_mode)
    context = BuildContext(
        cmake,
        cargo,
        jobs=args.jobs,
        timeout=args.build_timeout,
        skip_lib_tests=args.skip_lib_tests,
        fuzz=args.config_fuzz,
        asan=args.asan,
        rust=args.rust,
    )

    falco_manager: Optional[FalcoManager] = None
    try:
        if bench_mode is None:
            falco_manager = FalcoManager()
            falco_manager.start()
            shutdown_handler.register_falco(falco_manager)

        report = run_all_phases(args, test_cases, context, falco_manager)
        logger.info(f"Completed execution of test vectors")

        finalize_reporting(
            args.junit_xml,
            args.bench_out,
            report,
            bench_mode=bench_mode,
            color=(not args.no_color),
        )
        return ExitCode.SUCCESS
    except KeyboardInterrupt:
        return ExitCode.INTERRUPT
    except InternalError as e:
        logger.exception(e)
        return e.exit_code
    except Exception as e:
        logger.exception(f"An unexpected error occurred: {e}")
        return ExitCode.INTERNAL_ERROR
    finally:
        shutdown_handler.cleanup()


if __name__ == "__main__":
    raise SystemExit(main().value)
