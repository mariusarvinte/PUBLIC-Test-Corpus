# © 2026 Massachusetts Institute of Technology
# MIT License

import logging, json
from pathlib import Path
from typing import Optional

from .console import print_summary, print_failures
from .junit_xml import write_junit_xml
from ...types import BenchMode, RunReport, VectorExitCode, VectorResult
from ...utils import RUNTIME_TRIALS

logger = logging.getLogger(__name__)


def _get_benchmark_output(vector_result: VectorResult):
    if vector_result.exit_code == VectorExitCode.SKIP:
        return {"status": "skipped"}

    if vector_result.exit_code != VectorExitCode.BENCHMARK:
        return None

    output = vector_result.cando_output
    if not isinstance(output, dict) or len(output) != 1:
        return output

    bench_result = next(iter(output.values()))
    if isinstance(bench_result, dict) and "output" in bench_result:
        return bench_result["output"]

    return output


def write_benchmark(bench_out: Path, report: RunReport, bench_mode: BenchMode):
    bench_out.parent.mkdir(parents=True, exist_ok=True)

    bench_results = {}
    for case_name, suite in report.junit_suites.items():
        vector_results = {}
        for case in suite:
            if case.vector_result is None:
                continue

            bench_output = _get_benchmark_output(case.vector_result)
            if bench_output is not None:
                vector_results[case.name] = bench_output

        bench_results[case_name] = vector_results or None

    bench_report = {
        "benchmark": bench_mode.value,
        "results": bench_results,
    }
    if bench_mode == BenchMode.RUNTIME:
        bench_report["trials"] = RUNTIME_TRIALS

    with open(bench_out, "w") as f:
        json.dump(bench_report, f, indent=4)


def finalize_reporting(
    junit_path: Path,
    bench_out: Optional[Path],
    report: RunReport,
    bench_mode: Optional[BenchMode] = None,
    color=True,
):
    if junit_path:
        try:
            write_junit_xml(junit_path, report.junit_suites)
            logger.info(f"Wrote Junit report to `{junit_path}`")
        except Exception as e:
            logger.warning(f"Failed to write JUnit XML: {e}")

    if bench_mode:
        if bench_out is None:
            bench_out = Path.cwd() / f"{bench_mode.value}_bench.json"

        try:
            write_benchmark(bench_out, report, bench_mode)
            logger.info(f"Wrote benchmarking JSON output to `{bench_out}`")
        except Exception as e:
            logger.warning(f"Failed to write benchmarking JSON: `{e}`")

    print_failures(report.failures)
    print_summary(
        report.totals["total_cases"],
        report.totals["skipped_cases"],
        report.totals["tested_cases"],
        len(report.totals["failed_cases"]),
        report.totals["passed_vectors"],
        report.totals["skipped_vectors"],
        report.totals["failed_vectors"],
        colorize=color,
    )
