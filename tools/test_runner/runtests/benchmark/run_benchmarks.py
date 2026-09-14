# © 2026 Massachusetts Institute of Technology
# MIT License

from enum import Enum
import argparse, hashlib, json, logging, random, re, shlex, subprocess, sys, tempfile
from pathlib import Path
from dataclasses import dataclass

from ..orchestrator.discovery import collect_cases
from ..test_vector import get_vectors


BENCH_MODES = ["runtime", "memory"]
PROJECT_DIRS = ["Public-Tests", "Hidden-Tests"]
SCRIPT_DIR = Path(__file__).resolve().parent
logger = logging.getLogger(__name__)


type ProjectsInfo = dict[Performer, Path]


class BenchError(RuntimeError):
    pass


class Performer(Enum):
    AARNO = "aarno"
    GALOIS = "galois"
    HARVEST = "harvest"
    INTEL = "intel"
    UWISC = "uwisc"
    YALE = "yale"
    BASELINE_C = "baseline_c"


@dataclass
class BenchTuple:
    performer: str
    root: Path
    rust: bool
    case: str
    vector: str


def mode_seed(base_seed: int, mode: str) -> int:
    digest = hashlib.sha256(f"{base_seed}:{mode}".encode("ascii")).digest()
    return int.from_bytes(digest[:8], "big")


def bench_modes(selection: str) -> list[str]:
    return BENCH_MODES if selection == "both" else [selection]


def short_error(stdout: str, stderr: str, fallback: str) -> str:
    text = stderr.strip() or stdout.strip() or fallback
    return " ".join(text.split())


def build_base_command() -> list[str]:
    return [sys.executable, "-m", "runtests.orchestrator"]


def run_command(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=SCRIPT_DIR,
        text=True,
        capture_output=True,
        check=False,
    )


def format_command(cmd: list[str]) -> str:
    return shlex.join(cmd)


def add_common_flags(cmd: list[str], args: argparse.Namespace) -> None:
    if args.test_timeout is not None:
        cmd.extend(["--test-timeout", str(args.test_timeout)])
    if args.keep_going:
        cmd.append("--keep-going")


def discover_projects(performers_root: Path, c_root: Path) -> ProjectsInfo:
    c_root = c_root.resolve()
    performers_root = performers_root.resolve()

    if not c_root.exists():
        raise BenchError(f"Couldn't find given c_root: `{c_root}`")
    if not performers_root.exists():
        raise BenchError(f"Couldn't find given performers_root: `{performers_root}`")

    projects = {Performer.BASELINE_C: c_root}
    for performer in Performer:
        if performer is Performer.BASELINE_C:
            continue
        candidate = performers_root / performer.value
        if not (candidate.exists() and candidate.is_dir()):
            raise BenchError(f"Couldn't find performer path: `{candidate}`")
        projects[performer] = candidate.resolve()
    return projects


def discover_tuples(
    projects: ProjectsInfo,
    include_hidden: bool,
    match_regexes: list[str],
    match_vector_regexes: list[str],
) -> list[BenchTuple]:
    subset = (
        [Path(PROJECT_DIRS[0])]
        if not include_hidden
        else [Path(name) for name in PROJECT_DIRS]
    )

    tuples = []
    for performer, root in projects.items():
        rust = performer is not Performer.BASELINE_C
        performer_name = performer.value
        cases = collect_cases(root, subset, match_regexes, rust)

        for case in cases:
            for vector in get_vectors(case.test_root, match_vector_regexes):
                tuples.append(
                    BenchTuple(
                        performer=performer_name,
                        root=root,
                        rust=rust,
                        case=case.rel_name,
                        vector=vector.name,
                    )
                )

    tuples.sort(key=lambda item: (item.performer, item.case, item.vector))
    return tuples


def nested_set(
    out: dict[str, dict[str, dict[str, object]]],
    performer: str,
    case: str,
    vector: str,
    value: object,
) -> None:
    out.setdefault(performer, {}).setdefault(case, {})[vector] = value


def benchmark_error_value(message: str) -> dict[str, str]:
    return {"error": message}


def benchmark_skipped_value() -> dict[str, str]:
    return {"status": "skipped"}


def is_skipped_benchmark_value(value: object) -> bool:
    return isinstance(value, dict) and value.get("status") == "skipped"


def format_bench_tuple(bench_tuple: BenchTuple) -> str:
    return f"{bench_tuple.performer} {bench_tuple.case} {bench_tuple.vector}"


def list_mode_tuples(
    mode: str, ordered_tuples: list[BenchTuple], show_mode: bool
) -> None:
    if show_mode:
        print(f"[{mode}]")
    for index, bench_tuple in enumerate(ordered_tuples, start=1):
        print(f"{index}: {format_bench_tuple(bench_tuple)}")


def extract_benchmark_output(
    bench_json: Path, bench_tuple: BenchTuple, mode: str
) -> object:
    with open(bench_json, "r", encoding="utf-8") as handle:
        data = json.load(handle)

    benchmark = data.get("benchmark")
    if benchmark != mode:
        raise BenchError(f"benchmark JSON reported `{benchmark}` instead of `{mode}`")

    results = data.get("results")
    if not isinstance(results, dict):
        raise BenchError("missing `results` in benchmark JSON")

    case_results = results.get(bench_tuple.case)
    if not isinstance(case_results, dict):
        raise BenchError(f"missing case `{bench_tuple.case}` in benchmark JSON")

    if bench_tuple.vector not in case_results:
        raise BenchError(
            f"missing vector `{bench_tuple.vector}` for `{bench_tuple.case}` in benchmark JSON"
        )

    value = case_results[bench_tuple.vector]
    if is_skipped_benchmark_value(value):
        return benchmark_skipped_value()

    return value


def run_benchmark_tuple(
    bench_tuple: BenchTuple,
    mode: str,
    args: argparse.Namespace,
) -> tuple[bool, object]:
    with tempfile.TemporaryDirectory(prefix="run_bench_") as temp_dir:
        bench_out = Path(temp_dir) / f"{mode}.json"
        case_path = Path(bench_tuple.case)

        cmd = build_base_command()
        cmd.extend(
            [
                "--root",
                str(bench_tuple.root),
                "--bench",
                mode,
                "--only",
                "test",
                "--subset",
                str(case_path.parent),
                "--match-regex",
                f"^{re.escape(bench_tuple.case)}$",
                "--match-vector-regex",
                f"^{re.escape(bench_tuple.vector)}$",
                "--bench-out",
                str(bench_out),
            ]
        )
        if bench_tuple.rust:
            cmd.append("--rust")
        add_common_flags(cmd, args)

        result = run_command(cmd)
        if result.returncode != 0:
            error = short_error(
                result.stdout,
                result.stderr,
                f"benchmark failed with exit code {result.returncode}",
            )
            return False, benchmark_error_value(
                f"{error} [command: {format_command(cmd)}]"
            )

        try:
            return True, extract_benchmark_output(bench_out, bench_tuple, mode)
        except Exception as exc:
            return False, benchmark_error_value(
                f"{exc} [command: {format_command(cmd)}]"
            )


def get_error_message(value: object) -> str:
    if isinstance(value, dict):
        error = value.get("error")
        if isinstance(error, str):
            return error
    return str(value)


def run_mode(
    mode: str,
    ordered_tuples: list[BenchTuple],
    args: argparse.Namespace,
) -> tuple[dict[str, dict[str, dict[str, object]]], bool]:
    out: dict[str, dict[str, dict[str, object]]] = {}
    had_error = False

    logger.info("Running %s benchmarks on %d tuples", mode, len(ordered_tuples))

    if mode == "runtime" and args.warmup_count > 0:
        warmup = ordered_tuples[: min(args.warmup_count, len(ordered_tuples))]
        if warmup:
            logger.info("Runtime warmup on %d tuples", len(warmup))
        for index, bench_tuple in enumerate(warmup, start=1):
            logger.info(
                "Warmup tuple %d/%d: %s",
                index,
                len(warmup),
                format_bench_tuple(bench_tuple),
            )
            ok, value = run_benchmark_tuple(bench_tuple, mode, args)
            if not ok:
                logger.warning(
                    "Warmup failed for %s: %s",
                    format_bench_tuple(bench_tuple),
                    get_error_message(value),
                )

    for index, bench_tuple in enumerate(ordered_tuples, start=1):
        logger.info(
            "Running %s tuple %d/%d: %s",
            mode,
            index,
            len(ordered_tuples),
            format_bench_tuple(bench_tuple),
        )
        ok, value = run_benchmark_tuple(bench_tuple, mode, args)

        nested_set(
            out, bench_tuple.performer, bench_tuple.case, bench_tuple.vector, value
        )
        if ok and is_skipped_benchmark_value(value):
            logger.info(
                "%s benchmark skipped for %s",
                mode,
                format_bench_tuple(bench_tuple),
            )
            continue

        if not ok:
            had_error = True
            logger.warning(
                "%s benchmark failed for %s: %s",
                mode,
                format_bench_tuple(bench_tuple),
                get_error_message(value),
            )
            if not args.keep_going:
                break

    return out, had_error


def run_benchmarks(
    args: argparse.Namespace, performers_root: Path, c_root: Path, seed: int
) -> int:
    try:
        paths = discover_projects(performers_root, c_root)
    except BenchError as exc:
        logger.error("%s", exc)
        return 2

    all_tuples = discover_tuples(
        paths,
        args.include_hidden,
        args.match_regex,
        args.match_vector_regex,
    )

    logger.info("Discovered %d benchmark tuples", len(all_tuples))
    if not all_tuples:
        logger.warning("No benchmark tuples matched the current filters")

    modes = bench_modes(args.bench)
    if args.list:
        show_mode = len(modes) > 1
        for mode in modes:
            ordered = list(all_tuples)
            random.Random(mode_seed(seed, mode)).shuffle(ordered)
            list_mode_tuples(mode, ordered, show_mode)
        return 0

    mode_outputs: dict[str, dict[str, dict[str, dict[str, object]]]] = {}
    any_bench_error = False

    for mode in modes:
        ordered = list(all_tuples)
        random.Random(mode_seed(seed, mode)).shuffle(ordered)
        mode_output, had_error = run_mode(mode, ordered, args)
        mode_outputs[mode] = mode_output
        any_bench_error = any_bench_error or had_error
        if had_error and not args.keep_going:
            logger.warning("Stopping after %s benchmark error", mode)
            break

    output_path = args.output.resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as handle:
        if args.bench == "both":
            json.dump(mode_outputs, handle, indent=2)
        else:
            json.dump(mode_outputs[args.bench], handle, indent=2)

    logger.info("Wrote benchmark results to %s", output_path)

    return 1 if any_bench_error else 0
