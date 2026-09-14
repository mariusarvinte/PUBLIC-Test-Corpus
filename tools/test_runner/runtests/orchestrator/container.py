# © 2026 Massachusetts Institute of Technology
# MIT License

import os, shutil, docker, logging
from pathlib import Path
from typing import Optional
from tempfile import TemporaryDirectory

from .error import InternalError
from .shutdown_handler import shutdown_handler
from ..utils import (
    C_TEST_CASE_DIR,
    CANDO_RUNNER_FILE,
    CMAKE_TARGET_DIR,
    CONTAINER_FALCO_LOG_DIR,
    RUST_TEST_CASE_DIR,
    get_lib_target_name,
    CONTAINER_VOLUME_DIR,
    CONTAINER_OUTPUT_DIR,
    CONTAINER_LOG_LABEL_ENV,
    CONTAINER_LABEL_NAME,
)
from ..test_vector import (
    CANDO_VECTOR_FILE,
    CONTAINER_VECTOR_DIR_NAME,
    FILE_CHANGES_TARFILE,
)
from ..types import BenchMode, VectorExitCode, TestCase, VectorResult

logger = logging.getLogger(__name__)
EXEC_TEST_VECTOR_IMAGE = "exec_test_vector:latest"


def run_test_vector(
    host_falco_log_dir: Optional[Path],
    test_case: TestCase,
    test_vector: Path,
    rust: bool,
    no_compare: bool,
    bench_mode: Optional[BenchMode],
    env: Optional[dict[str, str]] = None,
    timeout: Optional[float] = None,
) -> VectorResult:
    """
    Runs a test vector inside a container. Constructs an input volume that get's passed
    to the container.

    Args:
        host_falco_log_dir: optional Falco log directory on the host machine
        test_case: the test case to run
        test_vector: the path to the directory/JSON file representing a test vector
        rust: run the C or Rust
        no_compare: don't compare output, but run to get expected output
        env: environment variables to pass to container
        timeout: maximum length the container can run for in seconds

    Returns:
        The return code from running the test vector container

    Raises:
        InternalError: various causes, raises InternalError for internal errors from within the container
    """
    # Create temporary directories for volumes for container
    with (
        TemporaryDirectory() as host_input_volume,
        TemporaryDirectory() as host_output_volume,
    ):
        host_input_volume = Path(host_input_volume)
        host_output_volume = Path(host_output_volume)

        if test_case.is_library:
            runner_name = get_lib_target_name(test_case.rel_name)
        else:
            runner_name = "cando_binrunner"

        runner_path = test_case.repo_root / "target" / "release" / runner_name
        if not runner_path.exists():
            raise InternalError(f"Runner path doesn't exist: `{runner_path}`")
        logger.debug(f"Using runner path: {runner_path}")

        if rust:
            subdir_name = RUST_TEST_CASE_DIR
            test_case_path = test_case.test_root / RUST_TEST_CASE_DIR
        else:
            subdir_name = C_TEST_CASE_DIR
            test_case_path = test_case.test_root / C_TEST_CASE_DIR
        if not test_case_path.exists():
            raise InternalError(f"Test case path doesn't exist: `{test_case_path}`")

        # Put things into a common directory regardless of if this is a filesystem vector
        # or just a normal cando vector
        if test_vector.is_dir():
            shutil.copytree(test_vector, host_input_volume / CONTAINER_VECTOR_DIR_NAME)
        else:
            out_dir = host_input_volume / CONTAINER_VECTOR_DIR_NAME
            os.mkdir(out_dir)
            shutil.copy(test_vector, out_dir / CANDO_VECTOR_FILE)

        shutil.copy(runner_path, host_input_volume / CANDO_RUNNER_FILE)

        # Because CMake for some of the test cases get's the name of the binary from the parent
        # directory of `test_case` this means we need to create a directory with the same name
        # as the test case to preserve the expected names
        full_test_case_dir = host_input_volume / test_case.test_root.name
        shutil.copytree(test_case_path, full_test_case_dir / subdir_name)
        shutil.copytree(
            test_case.runtime_bin_dir, full_test_case_dir / CMAKE_TARGET_DIR
        )

        vector_result = run_test_vector_container(
            host_falco_log_dir,
            host_input_volume,
            host_output_volume,
            f"{test_case.rel_name}:{test_vector.name}",
            test_case.is_library,
            rust,
            no_compare,
            bench_mode,
            env,
            timeout,
        )

        vector_code = vector_result.exit_code
        # We will immediately raise an InternalError because that should trigger an immediate exit
        if vector_code == VectorExitCode.INTERNAL_ERROR:
            raise InternalError(f"Got `InternalError` from test vector container")

        if no_compare:
            if vector_code != VectorExitCode.NO_COMPARE:
                raise InternalError(
                    f"Passed flag --no-compare to container but didn't get NO_COMPARE back. Got: `{vector_code}` for `{test_vector.name}`"
                )

            output_tar = host_output_volume / FILE_CHANGES_TARFILE
            if not output_tar.exists():
                logger.debug(
                    f"Got NoCompare from container but didn't get any file changes"
                )
                return vector_result
            if not test_vector.is_dir():
                raise InternalError(
                    f"Got NoCompare for `{test_vector}` that isn't a directory so we can't dump its results"
                )

            # Dump the tarfile to the vector dir
            shutil.copy(output_tar, test_vector)
            logger.info(
                f"Wrote `{output_tar}` to test vector at `{test_vector}` (on host machine)"
            )

        return vector_result


def run_test_vector_container(
    host_falco_log_dir: Optional[Path],
    host_input_volume: Path,
    host_output_volume: Path,
    container_log_label: str,
    is_library: bool,
    rust: bool,
    no_compare: bool,
    bench_mode: Optional[BenchMode],
    env: Optional[dict[str, str]] = None,
    timeout: Optional[float] = None,
) -> VectorResult:
    """
    Actually runs the `exec_test_vector` container setting up input/output volumes

    Args:
        host_falco_log_dir: optional Falco log directory to mount into the container
        host_input_volume: the path to the input volume (on the host machine)
        host_output_volume: the path to the output volume (on the host machine)
        container_log_label: the label to use when logging inside the container, because of the asynchronous
            calls the logs get messy so this makes analyzing the logs easier
        is_library: if this is a library test (if not binary)
        rust: run the C or Rust
        no_compare: don't compare output, but run to get expected output
        env: environment variables to pass to container
        timeout: maximum length the container can run for in seconds

    Returns:
        The return code from running the test vector container

    Raises:
        InternalError: various causes, raises InternalError for internal errors from within the container
    """
    client = docker.from_env()

    volumes = {
        str(host_input_volume): {
            "bind": str(CONTAINER_VOLUME_DIR),
            "mode": "ro",
        },
        str(host_output_volume): {
            "bind": str(CONTAINER_OUTPUT_DIR),
            "mode": "rw",
        },
    }

    if host_falco_log_dir is not None:
        volumes[str(host_falco_log_dir)] = {
            "bind": str(CONTAINER_FALCO_LOG_DIR),
            "mode": "ro",
        }

    args = ["-l", logging.getLevelName(logger.getEffectiveLevel())]
    if rust:
        args.append("--rust")
    if is_library:
        args.append("--lib")
    if timeout:
        args.extend(["--timeout", str(timeout)])
    if no_compare:
        args.append("--no-compare")
    if bench_mode:
        args.extend(["--bench", bench_mode.value])

    # Pass container log environment variable to container for better logging
    container_env = dict(env or os.environ)
    container_env[CONTAINER_LOG_LABEL_ENV] = container_log_label

    if shutdown_handler.cleanup_started():
        return VectorResult.skipped()

    logger.info(f"Running container for vector `{container_log_label}`")
    container = client.containers.run(
        image=EXEC_TEST_VECTOR_IMAGE,
        command=args,
        volumes=volumes,
        user=f"{os.getuid()}:{os.getgid()}",
        tmpfs={"/tmp": "size=1G,mode=1777"},  # CMake needs /tmp
        environment=container_env,
        labels={CONTAINER_LABEL_NAME: "true"},
        stdout=True,
        stderr=True,
        detach=True,
        cap_add=["CAP_PERFMON"],
    )

    # Stream logs from inside the container
    for line in container.logs(stream=True, follow=True):
        # Because some of the containers could still be running here we'll check if
        # we're shutting down
        if shutdown_handler.cleanup_started():
            break
        print(line.decode().strip())

    container_res = container.wait()
    vector_code = VectorExitCode(container_res["StatusCode"])
    container.remove()

    # Now get the result from the output file
    vector_result = VectorResult.from_path(
        vector_code, host_output_volume / CANDO_VECTOR_FILE
    )
    return vector_result
