# © 2026 Massachusetts Institute of Technology
# MIT License

from enum import Enum


class ExitCode(Enum):
    """Exit code for the main orchestrator"""

    SUCCESS = 0
    FAIL = 1  # Some vector didn't pass
    CLI_ERROR = 2  # User supplied incorrect command line arguments
    INTERNAL_ERROR = 3  # A catostrophic error that is our fault
    INTERRUPT = 4  # User interrupt (e.g., Ctrl+C)


class CliError(Exception):
    exit_code = ExitCode.CLI_ERROR

class InternalError(Exception):
    """Some other error that's our fault (e.g., couldn't build the cando runner)"""
    exit_code = ExitCode.INTERNAL_ERROR
