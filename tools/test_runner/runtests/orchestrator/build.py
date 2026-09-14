# © 2026 Massachusetts Institute of Technology
# MIT License

import json, os, glob, logging, shutil
from pathlib import Path
from typing import Optional, Tuple

from .error import CliError, InternalError
from ..types import TestCase, BuildContext
from ..utils import C_TEST_CASE_DIR, get_lib_target_name, run_command

logger = logging.getLogger(__name__)
BIN_RUNNER_NAME = "cando_binrunner"
ORIGIN_RPATH = "$ORIGIN"


def preset_build_dir(test_case_dir: Path, preset_name: str = "test") -> Optional[Path]:
    """Try to read CMakePresets.json and return the binaryDir for the given configure preset (if present)."""
    presets = test_case_dir / "CMakePresets.json"
    if not presets.exists():
        return None
    data = json.loads(presets.read_text(encoding="utf-8"))
    for cfg in data.get("configurePresets", []):
        if cfg.get("name") == preset_name:
            bd = (cfg.get("binaryDir") or "").replace(
                "${sourceDir}", str(test_case_dir)
            )
            return Path(bd).resolve() if bd else None
    return None


def configure_c_test_case(tc: TestCase, ctx: BuildContext) -> None:
    """
    Initializes build context for C test cases (sets up `build-ninja`).
    Equivalent to something like `cmake ..`.

    Raises:
        CommandError: if configuration fails or times out
    """
    _configure_c_test_case(tc, ctx.cmake, ctx.timeout, ctx.fuzz, ctx.asan)


def _configure_c_test_case(
    tc: TestCase,
    cmake: Path,
    timeout: Optional[float],
    fuzz: bool,
    asan: bool,
):
    """
    Internal function to iniailize build context

    Args:
        tc: the test case to configure
        cmake: the path to the cmake binary
        timeout: max time to run the configuration for
        fuzz: if the test case should be configured with fuzzing support
        asan: if the test case should be configured with ASAN/UBSAN support

    Raises:
        CommandError: if configuration fails or times out
    """
    use_preset = (tc.test_root / "CMakePresets.json").exists()

    cmd = [cmake]
    if use_preset:
        logger.debug("Using CMake preset")
        cmd += ["-S", tc.test_root, "--preset", "test"]
    else:
        cmd += [
            "-S",
            tc.test_root / C_TEST_CASE_DIR,
            "-B",
            tc.target_dir,
            "-G",
            "Ninja",
        ]

    cflags = []
    exe_ldflags = []
    sh_ldflags = []

    if asan:
        base_dbg = ["-g", "-fPIC", "-fno-omit-frame-pointer"]
        san = [
            "-fsanitize=address",
            "-fsanitize=undefined",
            "-fno-sanitize-recover=undefined",
        ]
        cflags += base_dbg
        cflags += san
        exe_ldflags += san
        sh_ldflags += san
        if tc.is_library:
            # Library-mode tests load user .so files into a separate runner process.
            # Use the shared ASan runtime there so the loaded library does not depend
            # on the loader already exporting `__asan_init`.
            sh_ldflags += ["-shared-libasan"]

    if fuzz:
        cflags += ["-fsanitize=fuzzer-no-link"]
        exe_ldflags += ["-fsanitize=fuzzer"]
        sh_ldflags += ["-fsanitize=fuzzer"]

    if cflags:
        cmd += [f"-DCMAKE_C_FLAGS={' '.join(cflags)}"]
    if exe_ldflags:
        cmd += [f"-DCMAKE_EXE_LINKER_FLAGS={' '.join(exe_ldflags)}"]
    if sh_ldflags:
        cmd += [
            f"-DCMAKE_SHARED_LINKER_FLAGS={' '.join(sh_ldflags)}",
            f"-DCMAKE_MODULE_LINKER_FLAGS={' '.join(sh_ldflags)}",
        ]

    # require clang to build
    cmd += ["-DCMAKE_C_COMPILER=clang"]

    # Make build-tree binaries resolve sibling shared libraries relative to themselves so the
    # mounted artifacts still work when run from a different absolute path inside a container.
    cmd += [
        "-DCMAKE_BUILD_RPATH_USE_ORIGIN=ON",
        f"-DCMAKE_BUILD_RPATH={ORIGIN_RPATH}",
    ]

    return run_command(
        cmd,
        cwd=tc.build_dir,
        capture_output=True,
        check=True,
        timeout=timeout,
    )


def build_test_case(tc: TestCase, ctx: BuildContext) -> None:
    """
    Builds a generic (either C or Rust) test case

    Raises:
        CommandError: if build fails or times out
    """
    if ctx.rust:
        if ctx.asan or ctx.fuzz:
            raise CliError(
                "We're not currently supporting use of ASan/UBSan/fuzzing for Rust"
            )
        logger.debug("Building Rust test case")
        build_rust_test_case(ctx.cargo, tc.build_dir, tc.target_dir, ctx.timeout)
    else:
        logger.debug("Building C test case")
        build_c_test_case(ctx.cmake, tc.test_root, tc.target_dir, ctx.timeout)


def build_rust_test_case(
    cargo: Path,
    build_dir: Path,
    target_dir: Path,
    timeout: Optional[float],
):
    """
    Build the translated rust test case. The corresponding Cargo.toml should build a cdylib

    Args:
        cargo: the path to the cargo binary
        build_dir: the directory to run the build in
        target_dir: the directory to output the build artifacts to
        timeout: max time to run build

    Raises:
        CommandError: if build fails or times out
    """
    cmd = [cargo]

    cmd += ["build", "--release", "--target-dir", target_dir]
    result = run_command(
        cmd,
        cwd=build_dir,
        capture_output=False,
        check=True,
        timeout=timeout,
    )

    # Some translation tools are placing libraries in the dependencies folder `deps`.
    # To allow our tools to find these translated libraries, we're moving them into release.
    if result.returncode == 0:
        release_dir = target_dir / "release"
        files = glob.glob(os.path.join(release_dir / "deps", "*.so"), recursive=False)
        for f in files:
            if not (release_dir / Path(f).name).exists():
                shutil.copy(f, release_dir)
    return result


def build_c_test_case(
    cmake: Path,
    test_root: Path,
    target_dir: Path,
    timeout: Optional[float],
):
    """
    Build the test case. If configured with a preset, build using that preset from the test case root
    otherwise, build from the explicit binary dir. `configure_c_test_case()` must have been called before
    calling this function.

    Args:
        cmake: the path to the cmake binary
        test_root: the directory of the test case
        target_dir: the directory to output the build artifacts to
        timeout: max time to run build

    Raises:
        CommandError: if build fails or times out
    """
    use_preset = (test_root / "CMakePresets.json").exists()

    cmd = [cmake, "--build"] + (["--preset", "test"] if use_preset else ["./"])
    cwd = test_root if use_preset else target_dir
    cmd += ["--parallel"]
    return run_command(cmd, cwd=cwd, capture_output=True, check=True, timeout=timeout)


def build_bin_runner(cargo: Path, repo_root: Path):
    """
    Builds the binary for cando_binrunner

    Raises:
        CommandError if build fails
    """
    logger.info(f"Building `{BIN_RUNNER_NAME}`")
    cmd = [cargo, "build", "--release", "--bin", BIN_RUNNER_NAME]
    run_command(cmd, cwd=repo_root, check=True)


def build_lib_runner(tc: TestCase, ctx: BuildContext) -> None:
    """
    Build the binary for cando_librunner

    Raises:
        CommandError: if build fails or times out
    """
    if ctx.skip_lib_tests or not tc.is_library:
        return
    _build_lib_runner(
        ctx.cargo,
        tc.repo_root,
        get_lib_target_name(tc.rel_name),
        ctx.timeout,
        False,
    )


def _handle_asan() -> Tuple[Path, Path]:
    """
    Finds the appropriate cargo/rustc binaries to add address sanitization for Rust (needs nightly)

    Returns: (path to nightly cargo, path to nightly rustc)
    """

    def assert_env_exists(env_var: str) -> Path:
        res = os.environ.get(env_var)
        if not res:
            raise InternalError(f"{env_var} is not set. Required for ASAN builds")
        path = Path(res)
        if not path.exists:
            raise InternalError(
                f"Path `{path}` does not exist. Required for ASAN builds"
            )
        return path

    nightly_cargo = assert_env_exists("CARGO_NIGHTLY")
    nightly_rustc = assert_env_exists("RUSTC_NIGHTLY")
    return (nightly_cargo, nightly_rustc)


def _build_lib_runner(
    cargo: Path,
    test_case_dir: Path,
    runner_name: str,
    timeout: Optional[float],
    asan: bool,
):
    """
    Internal helper for building the library runner

    Args:
        cargo: path to cargo binary
        test_case_dir: the directory for the library test case to build its runner for
        runner_name: the name of the binary for the given test case
        timeout: max time to run build
        asan: if the binary should be built with ASAN support (nightly rustc feature)

    Raises:
        CommandError: if build fails or times out
    """
    selected_cargo = cargo
    env = os.environ.copy()

    if asan:
        nightly_cargo, nightly_rustc = _handle_asan()
        selected_cargo = nightly_cargo
        # Set RUSTC env var so nightly cargo knows to use nightly rustc
        env["RUSTC"] = str(nightly_rustc)

    cmd = [selected_cargo, "rustc", "-p", runner_name, "--release"]

    if asan:
        cmd += ["--", "-Zsanitizer=address"]

    return run_command(
        cmd=cmd,
        cwd=test_case_dir,
        capture_output=True,
        check=True,
        env=env,
        timeout=timeout,
    )
