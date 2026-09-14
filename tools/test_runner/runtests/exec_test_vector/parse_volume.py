# © 2026 Massachusetts Institute of Technology
# MIT License

import logging
from dataclasses import dataclass
from pathlib import Path

from .error import InternalError
from ..utils import (
    C_TEST_CASE_DIR,
    CANDO_RUNNER_FILE,
    CONTAINER_VOLUME_DIR,
    RUST_TEST_CASE_DIR,
)
from ..test_vector import CONTAINER_VECTOR_DIR_NAME, TestVector

logger = logging.getLogger(__name__)


@dataclass
class Context:
    """Context from input volume necessary to run test vector"""

    volume: Path
    runner: Path
    project: Path  # this will be `translated_rust` if we're using Rust and `test_case` if C
    test_vector: TestVector


def parse_volume(rust: bool) -> Context:
    """
    Parses 'VOLUME_DIR', the path containing the context of the test to
    run that has been mounted into the container.

    Returns:
        Context: the paths for the necessary componenets of a test run

    Raises:
        InternalError: if the given `mount` directory doesn't exist or doesn't have
            the following necessary paths:
                runner (an executable cando runner, either for a library or binary)
                <test_case_name>/test_case/ (if `rust` is False) OR <test_case_name>/translated_rust/ (if `rust` is True)
                    NOTE: We're only actually using the build directory from this
                test_vector.json | test_vector/
    """

    def assert_exists(path: Path):
        if not path.exists():
            raise InternalError(f"Given path `{path}` doesn't exist")

    volume = CONTAINER_VOLUME_DIR
    assert_exists(volume)

    runner = volume.joinpath(CANDO_RUNNER_FILE).resolve()
    assert_exists(runner)

    # There should be a single directory NOT called test_vector here to preserve the naming
    # of the generated build artifacts
    dirs = [
        item
        for item in volume.iterdir()
        if item.is_dir() and item.name != CONTAINER_VECTOR_DIR_NAME
    ]
    if len(dirs) != 1:
        raise InternalError(f"Couldn't get the named test case directory from the give input volume")
    test_case_root = volume.joinpath(dirs[0])

    if rust:
        project = test_case_root.joinpath(RUST_TEST_CASE_DIR).resolve()
    else:
        project = test_case_root.joinpath(C_TEST_CASE_DIR).resolve()
    assert_exists(project)

    test_vector = TestVector.from_path(volume.joinpath(CONTAINER_VECTOR_DIR_NAME))

    return Context(volume, runner, project, test_vector)
