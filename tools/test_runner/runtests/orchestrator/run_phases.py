# © 2026 Massachusetts Institute of Technology
# MIT License

import os, time, logging, multiprocessing, signal
from contextlib import contextmanager
from pathlib import Path
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, field
from typing import Callable, Optional, Tuple
from statistics import fmean

from .build import (
    configure_c_test_case,
    build_test_case,
    build_bin_runner,
    build_lib_runner,
)
from .container import run_test_vector
from .error import InternalError
from .falco import FalcoManager
from .shutdown_handler import shutdown_handler
from ..test_vector import get_vectors
from ..types import (
    BenchMode,
    TestCase,
    JUnitCase,
    BuildContext,
    VectorExitCode,
    RunReport,
    VectorResult,
)
from ..utils import RUNTIME_TRIALS, find_asan_library

logger = logging.getLogger(__name__)

CONFIGURE_PHASE = "config"
BUILD_PHASE = "build"
BUILD_RUNNERS_PHASE = "build-runners"
TEST_PHASE = "test"
BENCH_PHASE = "bench"
PHASES = [CONFIGURE_PHASE, BUILD_PHASE, BUILD_RUNNERS_PHASE, TEST_PHASE, BENCH_PHASE]


def _init_pool_worker() -> None:
    """Let the parent own Ctrl+C handling for the process pool."""
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGTERM, signal.SIG_IGN)


@dataclass
class PhaseResult:
    """Result of running a single phase on a test case."""

    test_case: TestCase
    success: bool
    error_msg: Optional[str] = None


@dataclass
class CaseResult:
    """Results from running the vectors of a test case"""

    test_case: TestCase
    # Need the default_factory so each instance get's it's own list
    vector_results: list[Tuple[str, VectorResult]] = field(
        default_factory=list
    )  # [(vector_name, result)]


@contextmanager
def profile_elapsed(msg: str):
    """Output how long code-surrounded-by-context-manager takes to run

    Args:
        msg (str): description message to log
    """
    logger.info(f"Starting phase: {msg}")
    start = time.time()
    try:
        yield
    finally:
        elapsed = time.time() - start
        if not shutdown_handler.cleanup_started():
            logger.info(f"Profile: {msg} took {elapsed:.2f}s")


def _run_phase_worker(
    test_case: TestCase, phase_func: Callable, phase_name: str, context: BuildContext
) -> PhaseResult:
    """
    Helper function for running a phase on a test case

    Args:
        test_case: the test case to call `phase_func` on
        phase_func: the function to call when running this phase
        phase_name: the name of the phase we're currently running
        context: the build context required to configure and build test cases

    Returns
        the result of the run of this test case

    Raises:
        InternalError: re-raised from calling `phase_func`
    """
    if shutdown_handler.cleanup_started():
        return PhaseResult(test_case, success=False, error_msg="User interrupt")

    logger.info(f"{phase_name.capitalize()} {test_case.rel_name}")
    try:
        phase_func(test_case, context)
        return PhaseResult(test_case, True)
    except InternalError as e:
        raise e
    except Exception as e:
        return PhaseResult(test_case, False, f"{e!r}")


def _run_parallel_phase(
    args,
    phase_name: str,
    cases: list[TestCase],
    phase_func: Callable,
    context: BuildContext,
    n_jobs: Optional[int],
    totals: dict,
    failures: list[tuple[str, str]],
    junit_suites: dict,
    allow_errors: bool = True,
) -> dict[str, TestCase]:
    """
    Run a phase (configure/build) in parallel across test cases.

    Args:
        args: command line arguments from user
        phase_name: the name of the phase to run (e.g., "configure")
        cases: the active cases that still need to be run for correctness
        phase_func: the function to run against each of the test cases in `cases`
        context: the build context required to configure and build test cases
        n_jobs: # of workers to run in parallel for the ProcessPoolExecutor
        totals: the totals from the run so far (will get updated when calling this function)
        failures: the failures from the run so far (will get updated when calling this function)
        junit_suites: the junit results from the run so far (will get updated when calling this function)
        allow_errors: if the phase run should have any errors (e.g., for building the cando runners this
            should be False because an error there is our fault, for building the test cases themselves this
            should be True and it will catch errors)

    Returns:
        A dictionary of the test cases that are still active (those that haven't failed)

    Raises:
        InternalError: this will capture normal InternalErrors from things we explicity raise
            However, if `allow_errors` is False, then this will raise an InternalError if
            any test cases were not sucessfull during the run

    """
    still_active = {}

    with ProcessPoolExecutor(
        max_workers=n_jobs,
        mp_context=multiprocessing.get_context("fork"),
        initializer=_init_pool_worker,
    ) as executor:
        shutdown_handler.register_executor(executor)
        try:
            future_to_case = {
                executor.submit(
                    _run_phase_worker, tc, phase_func, phase_name, context
                ): tc
                for tc in cases
            }

            for future in as_completed(future_to_case):
                tc = future_to_case[future]
                try:
                    result = future.result()
                except InternalError as e:
                    shutdown_handler.cleanup()
                    raise e
                except Exception as e:
                    shutdown_handler.cleanup()
                    raise InternalError(f"Worker error: {e!r}")

                if result.success:
                    junit_suites[tc.rel_name].append(JUnitCase(phase_name, ok=True))
                    still_active[tc.rel_name] = tc
                else:
                    msg = f"{phase_name.capitalize()} failed: {result.error_msg}"
                    logger.error(msg)

                    if not allow_errors:
                        raise InternalError(
                            "Error occurred in phase that should always succeed"
                        )

                    junit_suites[tc.rel_name].append(
                        JUnitCase(name=phase_name, ok=False, error=True, message=msg)
                    )
                    failures.append((tc.rel_name, msg))
                    totals["failed_cases"].add(tc.rel_name)

                    if not args.keep_going:
                        for f in future_to_case:
                            f.cancel()
                        executor.terminate_workers()
                        break
        finally:
            shutdown_handler.unregister_executor()
    return still_active


def avg_vector_results(results: list[VectorResult]) -> VectorResult:
    """
    Average the values from the vectors across all of its runs. If any of the values in the run
    have None we set the overall to None just to be safe
    """
    if len(results) == 1:
        return results[0]

    if len(results) == 0:
        raise ValueError("Got empty results list. This should never happen")
    if len(set([result.exit_code for result in results])) != 1:
        raise ValueError(f"Exit codes did not match for all runs")

    exit_code = results[0].exit_code
    if exit_code == VectorExitCode.SKIP:
        return results[0]
    if exit_code == VectorExitCode.INTERNAL_ERROR:
        raise InternalError("Got InternalError while aggregating benchmark runs")

    outputs: list[dict] = []
    vector_name = None
    benchmark_result = None
    for result in results:
        if result.cando_output is None:
            raise ValueError("Didn't get output from all runs of a test vector")
        if len(result.cando_output) != 1:
            raise InternalError(
                "Got more than one test vector output which isn't possible"
            )

        if vector_name is None:
            vector_name = next(iter(result.cando_output.keys()))

        container_res = next(iter(result.cando_output.values()))
        if not isinstance(container_res, dict) or not isinstance(
            container_res.get("output"), dict
        ):
            raise InternalError("Malformed benchmark output from test vector")

        if benchmark_result is None:
            benchmark_result = container_res.copy()

        outputs.append(container_res["output"])

    # Check if all the results have the same keys + exit codes
    if not outputs or not all(
        set(d.keys()) == set(outputs[0].keys()) for d in outputs[1:]
    ):
        raise ValueError("Not all output have the same keys")

    averaged = {}
    for key in outputs[0].keys():
        values = []
        for output in outputs:
            # assert result.cando_output is not None  # Help type checker
            val = output[key]

            # If any of the keys are None we'll set the avg to be None too to be safe
            if val is None:
                averaged[key] = None
                break
            values.append(val)
        else:
            # Only do this if the loop doesn't end early (e.g., got a None)
            averaged[key] = fmean(values)

    if vector_name is None or benchmark_result is None:
        raise InternalError("Missing benchmark output while aggregating benchmark runs")

    benchmark_result["output"] = averaged
    return VectorResult.from_output(exit_code, {vector_name: benchmark_result})


def _run_test_worker(
    host_falco_log_dir: Optional[Path],
    test_case: TestCase,
    test_vector: Path,
    rust: bool,
    no_compare: bool,
    bench_mode: Optional[BenchMode],
    env: Optional[dict[str, str]] = None,
    timeout: Optional[float] = None,
) -> Tuple[TestCase, str, VectorResult]:
    """
    Helper for running a test vector

    Args:
        host_falco_log_dir: optional Falco log directory on the host machine
        test_case: the test case to run the associated `test_vector` on
        test_vector: the test vector (either directory or simple JSON file) to run against `test_case`
        rust: run C vs. Rust
        no_compare: don't compare output, but run to get expected output
        env: environment variables to pass to container
        timeout: timeout for the run of the test vector

    Returns:
        A tuple with the test case, string representation of the path of the test vector,
        and the result of running that vector

    Raises:
        InternalError: Any errors that have raise to this point are catastrophic
            internal errors that get re-raised
    """
    if shutdown_handler.cleanup_started():
        return (test_case, test_vector.name, VectorResult.skipped())

    logger.info(f"Running {test_case.rel_name} - {test_vector.name}")
    try:
        # Run multiple trials for runtime and average them
        trials = 1 if bench_mode != BenchMode.RUNTIME else RUNTIME_TRIALS
        vector_results: list[VectorResult] = []

        for _ in range(trials):
            vector_result = run_test_vector(
                host_falco_log_dir,
                test_case,
                test_vector,
                rust,
                no_compare,
                bench_mode,
                env,
                timeout,
            )
            vector_results.append(vector_result)

        vector_result = avg_vector_results(vector_results)
        return (test_case, test_vector.name, vector_result)
    except InternalError as e:
        raise e
    except Exception as e:
        # Any uncaught exception at this point is an internal error
        raise InternalError(e)


def _run_test_phase(
    args,
    falco_manager: Optional[FalcoManager],
    cases: list[TestCase],
    n_jobs: Optional[int],
    totals: dict,
    failures: list[tuple[str, str]],
    junit_suites: dict,
    env: Optional[dict[str, str]] = None,
    timeout: Optional[float] = None,
) -> dict[str, TestCase]:
    """
    Runs all of the given test cases isolating each test vector run in its own container

    Args:
        args: command line arguments from user
        falco_manager: optional Falco manager for filesystem-change tracking
        cases: the active cases that still need to be run for correctness
        n_jobs: # of workers to run in parallel for the ProcessPoolExecutor
        totals: the totals from the run so far (will get updated when calling this function)
        failures: the failures from the run so far (will get updated when calling this function)
        junit_suites: the junit results from the run so far (will get updated when calling this function)
        env: environment variables to pass to container
        timeout: timeout for the run of the test vector

    Returns:
        A dictionary of the test cases that are still active (those that haven't failed)

    Raises:
        InternalError: Any errors that have raise to this point are catastrophic
            internal errors that get re-raised
    """
    still_active = {}
    case_results: dict[str, CaseResult] = {tc.rel_name: CaseResult(tc) for tc in cases}

    bin_env = env or os.environ.copy()
    lib_env = env or os.environ.copy()

    # Handle ASAN options
    if args.asan:
        asan_env = {
            "ASAN_OPTIONS": "detect_leaks=0:leak_check_at_exit=0",
            "LSAN_OPTIONS": "detect_leaks=0:leak_check_at_exit=0",
        }
        bin_env.update(asan_env)
        lib_env.update(asan_env)

        # For library tests we need ASAN to be loaded first so set LD_PRELOAD
        asan_lib = find_asan_library()
        lib_env.update({"LD_PRELOAD": str(asan_lib)})

    # Handle benchmarking arguments
    if args.bench:
        logger.info(f"Running `{args.bench}` benchmark with 1 job")
        bench_mode = BenchMode(args.bench)
        n_jobs = 1
    else:
        bench_mode = None

    try:
        host_falco_log_dir = (
            falco_manager.get_host_log_dir() if falco_manager is not None else None
        )
        with ProcessPoolExecutor(
            max_workers=n_jobs,
            mp_context=multiprocessing.get_context("fork"),
            initializer=_init_pool_worker,
        ) as executor:
            shutdown_handler.register_executor(executor)

            future_to_result = {
                executor.submit(
                    _run_test_worker,
                    host_falco_log_dir,
                    tc,
                    test_vector,
                    args.rust,
                    args.no_compare,
                    bench_mode,
                    lib_env if tc.is_library else bin_env,
                    timeout,
                ): (tc, test_vector)
                for tc in cases
                for test_vector in get_vectors(tc.test_root, args.match_vector_regex)
            }

            for future in as_completed(future_to_result):
                # Collect all of the futures and pack them into case_results
                try:
                    test_case, vector_name, vector_result = future.result()
                    case_results[test_case.rel_name].vector_results.append(
                        (vector_name, vector_result)
                    )

                    # Break early if necessary
                    if not args.keep_going and vector_result.exit_code.is_fail():
                        for f in future_to_result:
                            f.cancel()
                        executor.terminate_workers()
                        break
                except InternalError as e:
                    shutdown_handler.cleanup()
                    raise e
                except Exception as e:
                    shutdown_handler.cleanup()
                    # Any uncaught exception at this point is an internal error
                    raise InternalError(e)
    finally:
        shutdown_handler.unregister_executor()

    # Now process them
    all_vectors_skipped = True
    for case_name, case_result in case_results.items():
        case_has_failure = False

        if not case_result.vector_results:
            totals["skipped_cases"] += 1
            junit_suites[case_name].append(
                JUnitCase(name=case_name, ok=False, skipped=True)
            )
            msg = (
                f"No test vectors matched for: {case_name}"
                if args.match_vector_regex
                else f"No test vectors found for: {case_name}"
            )
            logger.warning(msg)
            continue

        for vector_name, result in case_result.vector_results:
            exit_code = result.exit_code
            if exit_code == VectorExitCode.NO_COMPARE and not args.no_compare:
                raise InternalError(
                    f"Got a result of NO_COMPARE from container but didn't run with `--no-compare`"
                )

            if exit_code == VectorExitCode.SKIP:
                totals["skipped_vectors"] += 1
                junit_suites[case_name].append(
                    JUnitCase(
                        name=vector_name, ok=True, skipped=True, vector_result=result
                    )
                )
            elif (
                exit_code == VectorExitCode.PASS
                or exit_code == VectorExitCode.BENCHMARK
                or (exit_code == VectorExitCode.NO_COMPARE and args.no_compare)
            ):
                totals["passed_vectors"] += 1
                junit_suites[case_name].append(
                    JUnitCase(name=vector_name, ok=True, vector_result=result)
                )
                all_vectors_skipped = False
            else:
                # Anything else is a fail
                totals["failed_vectors"] += 1
                junit_suites[case_name].append(
                    JUnitCase(name=vector_name, ok=False, vector_result=result)
                )
                failures.append((case_name, f"{result.exit_code} for `{vector_name}`"))
                case_has_failure = True
                all_vectors_skipped = False

        if all_vectors_skipped:
            totals["skipped_cases"] += 1
        else:
            totals["tested_cases"] += 1

        if case_has_failure:
            totals["failed_cases"].add(case_name)
        else:
            still_active[case_name] = case_result.test_case

    return still_active


def run_all_phases(
    args,
    cases: list[TestCase],
    context: BuildContext,
    falco_manager: Optional[FalcoManager],
) -> RunReport:
    """
    Main entrypoint for running all of the phases against the corpus of test casese

    Args:
        args: command line arguments from user
        cases: the active cases that still need to be run for correctness
        context: the build context required to configure and build test cases
        falco_manager: optional Falco manager for filesystem-change tracking

    Returns:
        a report of everything that happened during the run

    Raises:
        InternalError: from called functions
    """
    n_jobs = args.jobs if args.jobs and args.jobs > 0 else None
    totals = {
        "total_cases": len(cases),
        "tested_cases": 0,
        "skipped_cases": 0,
        "failed_cases": set(),
        "passed_vectors": 0,
        "skipped_vectors": 0,
        "failed_vectors": 0,
    }

    failures: list[tuple[str, str]] = []
    junit_suites: dict[str, list[JUnitCase]] = {c.rel_name: [] for c in cases}

    # Track which test cases are still active (haven't failed yet)
    active_cases = {
        tc.rel_name: tc
        for tc in cases
        if (not tc.is_library or (tc.is_library and not context.skip_lib_tests))
    }

    # Phase 1: Configure test cases
    if not args.rust and (args.only is None or args.only == CONFIGURE_PHASE):
        with profile_elapsed("Configure test cases"):
            active_cases = _run_parallel_phase(
                args=args,
                phase_name=CONFIGURE_PHASE,
                cases=list(active_cases.values()),
                phase_func=configure_c_test_case,
                context=context,
                n_jobs=n_jobs,
                totals=totals,
                failures=failures,
                junit_suites=junit_suites,
                allow_errors=False,
            )
        if not args.keep_going and not active_cases:
            return RunReport(
                totals=totals, junit_suites=junit_suites, failures=failures
            )

    # Phase 2: Build test cases
    if args.only is None or args.only == BUILD_PHASE:
        with profile_elapsed("Build test cases"):
            active_cases = _run_parallel_phase(
                args=args,
                phase_name=BUILD_PHASE,
                cases=list(active_cases.values()),
                phase_func=build_test_case,
                context=context,
                n_jobs=n_jobs,
                totals=totals,
                failures=failures,
                junit_suites=junit_suites,
            )
        if not args.keep_going and not active_cases:
            return RunReport(
                totals=totals, junit_suites=junit_suites, failures=failures
            )

    lib_cases: dict[str, TestCase] = {}
    bin_cases: dict[str, TestCase] = {}
    for name, tc in active_cases.items():
        if tc.is_library:
            lib_cases[name] = tc
        else:
            bin_cases[name] = tc

    # Phase 3: Build cando binary and library runners
    if args.only is None or args.only == BUILD_RUNNERS_PHASE:
        with profile_elapsed("Build cando binary and library runners"):
            if bin_cases:
                build_bin_runner(context.cargo, args.root)

            if lib_cases and not args.skip_lib_tests:
                active_cases = _run_parallel_phase(
                    args=args,
                    phase_name=BUILD_RUNNERS_PHASE,
                    cases=list(lib_cases.values()),
                    phase_func=build_lib_runner,
                    context=context,
                    n_jobs=n_jobs,
                    totals=totals,
                    failures=failures,
                    junit_suites=junit_suites,
                    allow_errors=False,
                )
        active_cases.update(bin_cases)
        if not args.keep_going and not active_cases:
            return RunReport(
                totals=totals, junit_suites=junit_suites, failures=failures
            )

    # Phase 4: Kick off the containers to run the test cases/vectors
    # Used for both the correctness testing and the benchmarking (memory + runtime)
    if args.only is None or args.only == TEST_PHASE:
        with profile_elapsed("Running test vectors in containers"):
            active_cases = _run_test_phase(
                args,
                falco_manager,
                cases=list(active_cases.values()),
                n_jobs=n_jobs,
                totals=totals,
                failures=failures,
                junit_suites=junit_suites,
                timeout=args.test_timeout,
            )

    return RunReport(totals=totals, junit_suites=junit_suites, failures=failures)
