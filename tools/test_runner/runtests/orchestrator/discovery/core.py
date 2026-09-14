# © 2026 Massachusetts Institute of Technology
# MIT License

import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable, Pattern
from runtests.types import TestCase
from .c import is_case_dir_c, resolve_case_paths_c
from .rust import is_case_dir_rust, resolve_case_paths_rust


@dataclass(frozen=True)
class DiscoveryProfile:
    name: str
    is_case_dir: Callable[[Path], bool]
    # Returns (build_dir, target_dir, runtime_bin_dir, runner_dir) that runners use at run time
    resolve_case_paths: Callable[[Path], tuple[Path, Path, Path, Path]]
    bundle_rx: Pattern[str] = re.compile(r"[BP]([0-9]+)_.*")


def _is_library(p: Path) -> bool:
    return p.name.endswith("_lib")


def _walk_case_dirs(seed: Path, is_case_dir: Callable[[Path], bool]) -> list[Path]:
    out = []
    for dirpath, dirs, _ in os.walk(seed):
        p = Path(dirpath)
        if is_case_dir(p):
            out.append(p.resolve())
            dirs.clear()  # Don't descend into a case dir
        # Pruning likely neighboring directories
        dirs[:] = [d for d in dirs if d not in ("build", "build-ninja", "target")]

    return out


def collect_cases(
    root: Path, subset: Iterable[Path], match_regexes: list[str], rust: bool
) -> list[TestCase]:
    root = root.resolve()
    rxes = [re.compile(r) for r in match_regexes]
    seeds: list[Path] = []

    if rust:
        profile = DiscoveryProfile(
            name="rust",
            is_case_dir=is_case_dir_rust,
            resolve_case_paths=resolve_case_paths_rust,
        )
    else:
        profile = DiscoveryProfile(
            name="c",
            is_case_dir=is_case_dir_c,
            resolve_case_paths=resolve_case_paths_c,
        )

    def add_seed(p: Path):
        p = (p if p.is_absolute() else (root / p)).resolve()
        if p.is_dir():
            if profile.is_case_dir(p):
                seeds.append(p)
            else:
                seeds.extend(_walk_case_dirs(p, profile.is_case_dir))

    if not subset:
        add_seed(root)
    else:
        for s in subset:
            # If subset entry is a bundle (BNN_ / PNN_), expand it deterministically
            p = s if s.is_absolute() else (root / s)
            if p.exists() and p.is_dir() and profile.bundle_rx.fullmatch(p.name):
                for child in sorted(p.iterdir(), key=lambda c: (c.is_dir(), c.name)):
                    if child.is_dir() and profile.is_case_dir(child):
                        seeds.append(child.resolve())
            else:
                add_seed(p)

    # Deduplicate and filter by regex
    seen = set()
    cases: list[TestCase] = []
    for c in seeds:
        rel = str(c.relative_to(root))
        if rxes and not any(rx.search(rel) for rx in rxes):
            continue
        if c not in seen:
            seen.add(c)
            build_dir, target_dir, runtime_bin_dir, runner_dir = (
                profile.resolve_case_paths(c)
            )
            cases.append(
                TestCase(
                    test_root=c,
                    repo_root=root,
                    rel_name=rel,
                    is_library=_is_library(c),
                    build_dir=build_dir,
                    target_dir=target_dir,
                    runtime_bin_dir=runtime_bin_dir,
                    runner_dir=runner_dir,
                )
            )
    return cases
