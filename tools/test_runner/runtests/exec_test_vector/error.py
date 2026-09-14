# © 2026 Massachusetts Institute of Technology
# MIT License

from ..types import VectorExitCode

class BaseFail(Exception):
    """Base class that exception inherit from. Should not be used directly"""
    exit_code = VectorExitCode.INTERNAL_ERROR
    default_message = None

    def __init__(self, message=None):
        self.message = message or self.default_message
        super().__init__(self.message)
    
    def __str__(self):
        string = self.__class__.__name__
        if self.message:
            string = f"{string}: {self.message}"
        return string
    
class Skip(BaseFail):
    """Vector was skipped this run (most likely for UB)"""
    exit_code = VectorExitCode.SKIP

class NoCompare(BaseFail):
    """Vector was not compared to anything (passed --no-compare flag)"""
    exit_code = VectorExitCode.NO_COMPARE

class ComparisonFail(BaseFail):
    """Comparison failed (either library state, stdout, stderr, fileysystem state, etc...)"""
    exit_code = VectorExitCode.COMPARISON_FAIL

class Panic(BaseFail):
    """Rust panic during execution of test vector"""
    exit_code = VectorExitCode.PANIC

class SegmentationFault(BaseFail):
    """Segmentation fault (either from C or Rust) during execution of test vector"""
    exit_code = VectorExitCode.SEGMENTATION_FAULT

class ArtifactNotFound(BaseFail):
    """Vector not run because the expected binary/library wasn't generated properly"""
    exit_code = VectorExitCode.ARTIFACT_NOT_FOUND

class SymbolNotFound(BaseFail):
    """Vector not run because the excpected symbol was not in the library"""
    exit_code = VectorExitCode.SYMBOL_NOT_FOUND

class Timeout(BaseFail):
    exit_code = VectorExitCode.TIMEOUT

class Benchmark(BaseFail):
    exit_code = VectorExitCode.BENCHMARK

class BenchmarkFailure(BaseFail):
    exit_code = VectorExitCode.BENCHMARK_FAILURE

class UnknownFailure(BaseFail):
    """Some other failure occurred while running the test vector (not our fault)"""
    exit_code = VectorExitCode.UNKNOWN_FAILURE

class CliError(BaseFail):
    """Error with CLI"""
    exit_code = VectorExitCode.INTERNAL_ERROR # We're calling the container so a CliError is out fault

class InternalError(BaseFail):
    """Some other error that's our fault (e.g., so)"""
    exit_code = VectorExitCode.INTERNAL_ERROR
