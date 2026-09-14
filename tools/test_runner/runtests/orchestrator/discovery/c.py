# © 2026 Massachusetts Institute of Technology
# MIT License

from pathlib import Path

from ..build import preset_build_dir
from ...utils import (
    C_TEST_CASE_DIR,
    CANDO_RUNNER_DIR,
    CMAKE_TARGET_DIR,
    TEST_VECTORS_DIR,
)


def is_case_dir_c(p: Path) -> bool:
    return (p / C_TEST_CASE_DIR).exists() and (p / TEST_VECTORS_DIR).exists()


def resolve_case_paths_c(case_root: Path) -> tuple[Path, Path, Path, Path]:
    """
    Resolve the build directory and target directory from the test case's root.
    Return value is a tuple (build directory, target directory)
    """
    build_project_dir = case_root.resolve()
    preset_dir = preset_build_dir(build_project_dir, "test")
    target_dir = (preset_dir or (build_project_dir / CMAKE_TARGET_DIR)).resolve()
    runtime_bin_dir = target_dir
    runner_dir = build_project_dir / CANDO_RUNNER_DIR
    return (build_project_dir, target_dir, runtime_bin_dir, runner_dir)
