# © 2026 Massachusetts Institute of Technology
# MIT License

import argparse, sys
from argparse import ArgumentParser

from runtests.types import BenchMode
from runtests.utils import handle_common_invalid_args
from .error import CliError


class CustomArgParser(ArgumentParser):
    def exit(self, status=0, message=None):
        """Overrides the default implementation of `exit()` to raise our custom `CliError`"""
        _ = status  # Don't care about status
        if message:
            sys.stderr.write(message)
        raise CliError()


def parse_args():
    ap = CustomArgParser(
        description="Entry point for the execution of a single test vector inside a Docker container",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    ap.add_argument(
        "-l",
        "--log-level",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        default="DEBUG",
        help="Set logging level",
    )
    ap.add_argument(
        "--rust", action="store_true", help="This is a Rust vs. C test case"
    )
    ap.add_argument(
        "--lib", action="store_true", help="This is a library vs. binary test case"
    )
    ap.add_argument(
        "--no-compare",
        action="store_true",
        help="Don't do comparison, just run the vector and dump it's result",
    )
    ap.add_argument(
        "--timeout",
        type=float,
        default=120,
        help="Set a timeout (in seconds) for running tests",
    )
    ap.add_argument(
        "--bench",
        choices=[mode.value for mode in BenchMode]
    )
    # =============================== TODO: Check this ===============================
    ap.add_argument(
        "--fuzz",
        action="store_true",
        help="Configure test_cases to build with fuzzing support",
    )
    # ================================================================================

    args = ap.parse_args()
    handle_common_invalid_args(args)

    return ap.parse_args()
