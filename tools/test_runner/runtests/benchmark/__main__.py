# © 2026 Massachusetts Institute of Technology
# MIT License

import argparse, logging, random
from pathlib import Path
from typing import Optional

from .run_benchmarks import run_benchmarks


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Benchmark the C baseline and six Rust performer trees. "
            "Assumes all projects and runners are already built."
        )
    )
    parser.add_argument(
        "--c-root",
        type=Path,
        default=Path.cwd(),
        help="Path to the C baseline repository root",
    )
    parser.add_argument(
        "--performers-root",
        type=Path,
        required=True,
        help="Directory containing performer roots as {performer}",
    )
    parser.add_argument(
        "--bench",
        choices=["runtime", "memory", "both"],
        required=True,
        help="Benchmark mode to run. If `both` will run one after the other",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("bench_results.json"),
        help="Path to JSON output",
    )
    parser.add_argument(
        "-m",
        "--match-regex",
        action="append",
        default=[],
        help="Regex to select test cases by their relative path; can be repeated",
    )
    parser.add_argument(
        "-mv",
        "--match-vector-regex",
        action="append",
        default=[],
        help="Regex to select test vectors by their name; can be repeated",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Seed for reproducible randomization",
    )
    parser.add_argument(
        "--warmup-count",
        type=int,
        default=10,
        help="Runtime-only warmup tuple count",
    )
    parser.add_argument(
        "--no-include-hidden",
        action="store_false",
        dest="include_hidden",
        default=True,
        help="Restrict discovery and builds to Public-Tests",
    )
    parser.add_argument(
        "--test-timeout",
        type=float,
        default=None,
        help="Timeout in seconds for per-tuple benchmark runs",
    )
    parser.add_argument(
        "--keep-going",
        action="store_true",
        default=False,
        help="Continue after build or benchmark failures",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        default=False,
        help="List benchmark tuples in randomized order and exit",
    )
    args = parser.parse_args()

    if args.warmup_count < 0:
        parser.error("--warmup-count must be non-negative")

    return args


def resolve_seed(seed: Optional[int]) -> int:
    if seed is not None:
        return seed
    return random.SystemRandom().randrange(0, 2**63)


def main():
    args = parse_args()
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")

    performers_root = args.performers_root.resolve()
    c_root = args.c_root.resolve()
    seed = resolve_seed(args.seed)

    run_benchmarks(args, performers_root, c_root, seed)


if __name__ == "__main__":
    raise SystemExit(main())
