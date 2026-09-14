# © 2026 Massachusetts Institute of Technology
# MIT License

from pathlib import Path
from ...utils import CANDO_RUNNER_DIR, RUST_TEST_CASE_DIR, TEST_VECTORS_DIR


def is_case_dir_rust(p: Path) -> bool:
    return (p / RUST_TEST_CASE_DIR).exists() and (p / TEST_VECTORS_DIR).exists()


def resolve_case_paths_rust(case_root: Path) -> tuple[Path, Path, Path, Path]:
    """
    Resolve the build directory and target directory from the test case's root.
    Return value is a tuple (build directory, target directory, runtime binary directory, runner directory)
    """
    build_project_dir = (case_root / RUST_TEST_CASE_DIR).resolve()
    target_dir = build_project_dir / "target"
    runtime_bin_dir = target_dir / "release"
    runner_dir = (case_root / CANDO_RUNNER_DIR).resolve()
    return (build_project_dir, target_dir, runtime_bin_dir, runner_dir)
