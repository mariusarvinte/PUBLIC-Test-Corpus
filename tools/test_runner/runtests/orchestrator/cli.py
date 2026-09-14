# © 2026 Massachusetts Institute of Technology
# MIT License

import argparse, os, sys, logging
from pathlib import Path

from .run_phases import PHASES
from .error import CliError
from ..types import BenchMode
from ..utils import handle_common_invalid_args

logger = logging.getLogger(__name__)

DEFAULT_TEST_TIMEOUT = 120
BENCH_TEST_TIMEOUT = 600


class CustomArgParser(argparse.ArgumentParser):
    def exit(self, status=0, message=None):
        """Overrides the default implementation of `exit()` to raise our custom `CliError`"""
        if message:
            sys.stderr.write(message)
        if status == 0:
            raise SystemExit(0)
        raise CliError()


def parse_args():
    ap = CustomArgParser(
        description="Orchestrator for running C/Rust test cases inside a container",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    ap.add_argument("--root", type=Path, default=os.getcwd(), help="Path to root")
    ap.add_argument(
        "-l",
        "--log-level",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        default="INFO",
        help="Set logging level",
    )
    ap.add_argument(
        "--rust", action="store_true", help="We're running C vs. Rust test cases"
    )
    ap.add_argument(
        "--no-compare",
        action="store_true",
        help="Don't evaluate test vectors for equality, but write their results to the appropriate test vector",
    )
    ap.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=None,
        help="Parallel build and configure jobs (default = use all cores)",
    )
    ap.add_argument(
        "-m",
        "--match-regex",
        action="append",
        default=[],
        help="Regex to select test cases by their relative path; can be repeated",
    )
    ap.add_argument(
        "-mv",
        "--match-vector-regex",
        action="append",
        default=[],
        help="Regex to select test vectors by their name; can be repeated",
    )
    ap.add_argument(
        "-s",
        "--subset",
        type=Path,
        action="append",
        default=[],
        help="Explicit directories to search for tests; can be repeated. "
        "Relative to --root unless absolute.",
    )
    ap.add_argument(
        "--build-timeout",
        type=float,
        default=None,
        help="Set a timeout (in seconds) for building tests",
    )
    ap.add_argument(
        "--test-timeout",
        type=float,
        default=argparse.SUPPRESS,
        help=(
            "Set a timeout (in seconds) for running tests "
            f"(default: {DEFAULT_TEST_TIMEOUT}s, or {BENCH_TEST_TIMEOUT}s with --bench)"
        ),
    )
    ap.add_argument(
        "-x",
        "--junit-xml",
        type=Path,
        default=None,
        help="Write a JUnit XML report to this path",
    )
    ap.add_argument(
        "--clean", action="store_true", help="Delete temporary build artifacts"
    )
    ap.add_argument(
        "--keep-going",
        action="store_true",
        help="Continue building and running other testcases if one fails",
    )
    ap.add_argument(
        "--list", action="store_true", help="List selected test cases and exit"
    )
    ap.add_argument(
        "--no-color",
        action="store_true",
        help="Don't use escape codes for coloring output",
    )
    ap.add_argument(
        "--skip-lib-tests",
        action="store_true",
        help="Don't run the tests for libraries",
    )
    ap.add_argument(
        "--only",
        type=str,
        choices=PHASES,
        default=None,
        help="Select a single pass to perform",
    )
    ap.add_argument(
        "--asan",
        action="store_true",
        help="Enable building with asan and ubsan. \
            NOTE: P02_libgit2 test cases that use the `FAKETIME` environment variable are currently \
            incompatible with this option and will be skipped",
    )
    ap.add_argument(
        "-b",
        "--bench",
        choices=[mode.value for mode in BenchMode],
        help="Select benchmarking option. The runtime option currently uses perf-stat which \
            is required to be installed, set in the user's PATH and will \
            usually require root permissions and/or setting kernel.perf_event_paranoid to run. \
            Additionally, for memory, valgrind should be installed and set in PATH",
    )
    ap.add_argument(
        "--bench-out",
        type=Path,
        default=None,
        help="Path to output benchmarking JSON (requires --bench)",
    )
    # =================================== TODO: Check this =================================
    ap.add_argument(
        "--config-fuzz",
        action="store_true",
        help="Configure test cases to build with fuzzing support",
    )
    # ======================================================================================

    args = ap.parse_args()

    if not hasattr(args, "test_timeout"):
        args.test_timeout = BENCH_TEST_TIMEOUT if args.bench else DEFAULT_TEST_TIMEOUT

    handle_common_invalid_args(args)

    if args.bench_out and not args.bench:
        ap.error("--bench-out requires --bench")

    return args
