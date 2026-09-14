# © 2026 Massachusetts Institute of Technology
# MIT License

{
  description = "Reproducible development environment and Docker images for test runners";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, rust-overlay }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        overlays = [ (import rust-overlay) ];
        pkgs = import nixpkgs {
          inherit system overlays;
        };

        lib = pkgs.lib;
        llvm = pkgs.llvmPackages;
        clang = llvm.clang;

        nightlyRustToolchain = pkgs.pkgsBuildHost.rust-bin.nightly.latest.default;

        runtestsPackage = pkgs.python314Packages.buildPythonPackage {
          pname = "runtests";
          version = "0.1.0";
          src = ./.;
          format = "pyproject";

          nativeBuildInputs = with pkgs.python314Packages; [
            setuptools
          ];

          propagatedBuildInputs = with pkgs.python314Packages; [
            docker
            xxhash
          ];
        };

        pyEnv = pkgs.python314.withPackages (ps: [
          runtestsPackage
          ps.docker
          ps.xxhash
        ]);

        runtimeLibs = with pkgs; [
          llvm.compiler-rt
          openssl
          llvm.libclang
          gcc.cc.lib
          glibc
          libfaketime
          pcre2.dev
          pcre2
          zlib.dev
          zlib
        ];

        runtimeTools = with pkgs; [
          nightlyRustToolchain
          pyEnv
          pkg-config
          cmake
          ninja
          llvm.clang
          bash
          coreutils
          rustup
          git
          gnutar
          gzip
          perf
          valgrind
        ];

        headerInputs = with pkgs; [
          glibc.dev
          openssl.dev
          pcre2.dev
          zlib.dev
        ];

        clangResourceInclude = "${clang}/resource-root/include";

        runtimeDeps = runtimeTools ++ runtimeLibs;

        runtimeLibraryPath = lib.makeLibraryPath runtimeLibs;

        cHeaderPath = lib.concatStringsSep ":"
          ([ clangResourceInclude ] ++ map (p: "${p}/include") headerInputs);

        buildEnv = {
          LIBRARY_PATH = runtimeLibraryPath;
          LIBCLANG_PATH = "${lib.getLib llvm.libclang}/lib";
          PKG_CONFIG_PATH = "${pkgs.pcre2.dev}/lib/pkgconfig";

          # Specify libfaketime so cando can load it for vectors that require it
          LIBFAKETIME_SO = "${pkgs.libfaketime}/lib/libfaketime.so.1";

          # Nightly for ASAN
          CARGO_NIGHTLY = "${nightlyRustToolchain}/bin/cargo";
          RUSTC_NIGHTLY = "${nightlyRustToolchain}/bin/rustc";

          # Pin the C compiler so cmake/cargo build scripts agree on clang
          CC = "${clang}/bin/clang";
          CMAKE_C_COMPILER = "${clang}/bin/clang";

          # Build dependencies of the test cases.
          OPENSSL_DIR = "${pkgs.openssl.dev}";
          OPENSSL_LIB_DIR = "${pkgs.openssl.out}/lib";
          OPENSSL_INCLUDE_DIR = "${pkgs.openssl.dev}/include";

          BINDGEN_EXTRA_CLANG_ARGS = "-I${clangResourceInclude}";

          # clang intrinsics + libc + test-case deps.
          CPATH = cHeaderPath;
        };

        envExports = lib.concatStringsSep "\n"
          (lib.mapAttrsToList (k: v: ''export ${k}="${v}"'') buildEnv);
        envList = lib.mapAttrsToList (k: v: "${k}=${v}") buildEnv;

        dockerImage = pkgs.dockerTools.buildLayeredImage {
          name = "exec_test_vector";
          tag = "latest";
          contents = runtimeDeps ++ headerInputs;

          config = {
            Entrypoint = [ "${pyEnv}/bin/python3" "-m" "runtests.exec_test_vector" ];
            WorkingDir = "/app";
            Env = envList ++ [
              "CMAKE_BUILD_WITH_INSTALL_RPATH=ON"
              "LD_LIBRARY_PATH=${runtimeLibraryPath}"
            ];
          };
        };

        runOrchestrator = pkgs.writeShellScriptBin "runOrchestrator" ''
          # Exit on all failures
          set -euo pipefail

          # Make sure that rustup's cargo, rustc, ... are first 
          export PATH="$HOME/.cargo/bin:${lib.makeBinPath runtimeDeps}"
          ${envExports}

          echo "Loading Nix image into Docker"
          ${pkgs.docker-client}/bin/docker load < ${dockerImage}

          exec "${pyEnv}/bin/python3" "-m" "runtests.orchestrator" "$@"
        '';

        cleanupImages = pkgs.writeShellScriptBin "cleanupImages" ''
          ${pkgs.docker-client}/bin/docker image rm exec_test_vector:latest
        '';

      in
      {
        apps.default = flake-utils.lib.mkApp {
          drv = runOrchestrator;
        };

        packages = {
          default = runOrchestrator;
          docker = dockerImage;
        };

        checks.smoke = pkgs.runCommand "runtests-smoke" { } ''
          ${pyEnv}/bin/python3 -m runtests.orchestrator --help > $out
        '';

        devShells.default = pkgs.mkShell {
          buildInputs = runtimeDeps;
          shellHook = ''
            export TMPDIR="/tmp" # Don't use the nix shell tmpdir that doesn't play well with Docker
            ${envExports}
          '';
        };

        apps.clean = flake-utils.lib.mkApp {
          drv = cleanupImages;
        };
      }
    );
}
