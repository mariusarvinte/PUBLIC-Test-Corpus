#!/usr/bin/env python3

# © 2026 Massachusetts Institute of Technology
# MIT License

import logging, subprocess, re, sys
from pathlib import Path
from argparse import ArgumentParser
from typing import Sequence, cast

# Miscellaneous files / directories that don't need license headers
IGNORE = [
    "procspawn", # TODO: the fork we have, not sure if this needs a license
    "node_modules",
    "resources",
    "README.md",
    ".github",
    ".gitignore",
    ".gitkeep",
    ".gitmodules",
    ".dockerifngore",
    "uv.lock",
    "flake.lock",
]
# File extensions that either don't need or support comments and don't need a header
# Exception of CMakeLists.txt
IGNORE_SUFFIXES = [
    ".json",
    ".md",
    ".txt",
    ".patch",
    ".yml",
    ".yaml",
    ".gz",
    ".bak",
    ".backup",
    ".fixme",
]
# Regexes that are organic / taken from the wild
ORGANIC_PATTERNS = [
    r"B0\d_organic", # Batteries
    r"P\d{2}_\w+", # Projects
]
# Files / directories to keep even if an ignored pattern
ALWAYS_KEEP = [
    "CMakeLists.txt",
    "runner",
    "test_vectors"
]
# File types that can have a shebang at the top of the file
FILE_SUFFIX_SHEBANG = [
    ".sh",
    ".py",
]
# Paths to the license file for an organic test case or project relative to the test case
ORGANIC_LICENSE_PATHS = [
    "LICENSE",
    "COPYING",
    "test_case/LICENSE",
    "test_case/license.txt",
    "project/CREDITS",
]

MAIN_LICENSE = """MIT License

Copyright (c) 2026 Massachusetts Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""
LICENSE_HEADER = """© 2026 Massachusetts Institute of Technology
MIT License"""

HIDDEN_TESTS = "Hidden-Tests"

class CliArgs:
    check: bool
    root: Path
    verbose: bool


class LicensePaths:
    missing_mit_header: list[Path]
    missing_organic_license: list[Path]


def get_comment_str(file: Path) -> str:
    """Looks at the file name/suffix to determin the correct comment string"""
    match file.suffix:
        case ".c" | ".h" | ".rs" | ".js":
            return "//"
        case ".py" | ".toml" | ".yml" | ".sh" | ".nix":
            return "#"

    match file.name:
        case "CMakeLists.txt" | "Dockerfile" | "setup":
            return "#"

    raise RuntimeError(f"Couldn't determine comment leader for file: {file}")


def get_license_header(file: Path, add_newline: bool = False) -> list[str]:
    """Returns the license header with the appropriate comments based on the given `file`"""
    comment_str = get_comment_str(file)
    newline = "\n" if add_newline else ""
    commented = []
    for line in LICENSE_HEADER.splitlines():
        commented.append(f"{comment_str} {line.strip()}{newline}")
    return commented


def can_have_shebang(file: Path) -> bool:
    """Returns True/False if the given `file` can have a shebang"""
    if file.suffix in FILE_SUFFIX_SHEBANG:
        return True
    if file.name == "setup":
        return True
    return False


def add_license_headers(files: list[Path]):
    """Adds the appropriate license headers to the given list of files"""
    for file in files:
        with open(file, "r") as f:
            file_content = f.readlines()

        license_header = get_license_header(file, True)

        updated_content = []
        if can_have_shebang(file):
            if len(file_content) and file_content[0].startswith("#!"):
                # Preserve the shebang
                updated_content.append(file_content.pop(0))
                updated_content.append("\n")

        # Now add the license and newline if needed
        updated_content.extend(license_header)
        if len(file_content) and file_content[0] != "\n":
            updated_content.append("\n")

        updated_content.extend(file_content)

        with open(file, "w") as f:
            logger.debug(f"Writing liccense to {file}")
            f.writelines(updated_content)

        # Check the license as a sanity check
        if not check_license_header(file):
            raise RuntimeError(f"Didn't add appropriate license for {file}")


def check_license_header(file: Path) -> bool:
    """
    Checks if `file` has the correct license at the top (or below if a shebang is present).
    Leading/trailing whitespace is ignored.
    """
    with open(file, "r") as f:
        file_content = f.readlines()
    file_content = [line.strip() for line in file_content]
    license_header = get_license_header(file)

    # Check two lines down for files with shebangs
    if can_have_shebang(file):
        lines = file_content[2 : len(license_header) + 2]
        if license_header == lines:
            return True

    # If we couldn't find it there check at the beginning
    return file_content[0 : len(license_header)] == license_header


def matches_organic(path: Path) -> bool:
    for pattern in ORGANIC_PATTERNS:
        if re.search(pattern, str(path)):
            return True
    return False


def organic_needs_check(path: Path) -> bool:
    """
    Checks if we need to check the license for a file/directory that's organic.
    This will be the case for entries in `ALWAYS_KEEP` and organic directories
    that we need to check they have a license file. If the path isn't organic we 
    return True, and handle it upstream.
    """
    if matches_organic(path):
        if path.is_dir():
            return True
        if path.is_file and any(keep in str(path) for keep in ALWAYS_KEEP):
            return True
    else:
        return True

    return False


def get_paths(root: Path) -> list[Path]:
    """Finds the list of files / directories that need to be checked for licenses"""
    public_git_tracked_files = subprocess.check_output(
        ["git", "ls-files", str(root)],
        text=True
    ).splitlines()
    # Have to separately do this on the `Hidden-Tests` submodule
    hidden_git_tracked_files = subprocess.check_output(
        ["git", "-C", HIDDEN_TESTS, "ls-files", str(root)],
        text=True
    ).splitlines()
    hidden_git_tracked_files = [Path(HIDDEN_TESTS) / file for file in hidden_git_tracked_files]

    git_tracked_files = [Path(file) for file in public_git_tracked_files + hidden_git_tracked_files]

    if len(git_tracked_files) == 0:
        raise RuntimeError(f"Root `{root}` has no git tracked files")

    # Add the test case name directories as well
    git_tracked_paths = set()
    for file in git_tracked_files:
        git_tracked_paths.add(file)
        for parent in file.parents:
            # Test case paths will only have three nested directories
            if len(parent.parts) == 3:
                git_tracked_paths.add(parent)

    needs_check = []
    for path in git_tracked_paths:
        # Skip symlinks
        if path.is_symlink():
            continue
        # Skip ignored suffixes
        if path.suffix in IGNORE_SUFFIXES and not path.name == "CMakeLists.txt":
            continue
        # Skip ignored substrings (either files or directories)
        if any(ignored_path in str(path) for ignored_path in IGNORE):
            continue
        # Skip organic paths other than the ones we need to check
        if matches_organic(path):
            if not organic_needs_check(path):
                continue
        elif not path.is_file():
            continue

        needs_check.append(path)
    return needs_check


def check_organic_license_file(path: Path) -> bool:
    """Checks if given `path` has one of the allows license files"""
    for license_path in ORGANIC_LICENSE_PATHS:
        if (path / license_path).exists():
            return True
    return False


def check_all(paths: list[Path]) -> list[Path]:
    """
    Checks all of the files/directories in `paths` and returns those that don't have 
    an appropriate license header or file.
    """
    missing_licenses = []
    for path in paths:
        logger.debug(f"Checking: {path}")
        # Check that there is a license file
        if path.is_dir():
            if check_organic_license_file(path):
                continue
        else:
            if str(path) == "LICENSE" and str(path) != MAIN_LICENSE:
                continue
            if check_license_header(path):
                continue
                
        missing_licenses.append(path)
    return missing_licenses


def handle_missing(missing: list[Path], check: bool):
    """
    Handles files/directories that don't have a license.
    If `check == True` then just prints paths without the appropriate license.
    if `check == False` then adds license headers and prints organic test cases
    without a license file.
    """
    def fmt_paths(paths: Sequence[str | Path]) -> str:
        return "\t" + "\n\t".join(str(p) for p in paths)

    def check_license_files(missing_license_files: list[Path]) -> bool:
        if missing_license_files:
            logger.warning(
                "The following %d organic test case(s) are missing a license file:\n%s\n"
                "To fix them add one of the following files relative to the directory:\n%s\n",
                len(missing_license_files),
                fmt_paths(missing_license_files),
                fmt_paths(ORGANIC_LICENSE_PATHS),
            )
            return True
        return False
        

    missing_license_files = sorted(p for p in missing if p.is_dir())
    missing_license_headers = sorted(p for p in missing if not p.is_dir())

    if len(missing):
        if check:
            missing_license = False
            if missing_license_headers:
                logger.warning(
                    "The following %d file(s) are missing license headers:\n%s\n"
                    "To fix them run `tools/license.py .` or `tools/license.py <FILE_NAME>`"
                    "from the root of the repo",
                    len(missing_license_headers),
                    fmt_paths(missing_license_headers),
                )
                missing_license = True
                
            if check_license_files(missing_license_files):
                missing_license = True

            if missing_license:
                sys.exit(1)
        else:
            add_license_headers(missing_license_headers)
            logger.info(
                f"Added license headers to the following files:\n{fmt_paths(missing_license_headers)}"
            )
    else:
        logger.info("No licenses are missing")
        

def main():
    ap = ArgumentParser(
        description="Checks/adds MIT license header to all necessary files \
        and ensures organic test cases and projects have a license file"
    )
    ap.add_argument(
        "-c",
        "--check",
        action="store_true",
        default=False,
        help="Just check for missing license instead of adding them. Return non-0 if missing licenses.",
    )
    ap.add_argument(
        "root", 
        type=Path, 
        help="Root directory or file to inspect",
    )
    ap.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Show verbose output",
    )
    args = cast(CliArgs, ap.parse_args())

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
    global logger
    logger = logging.getLogger("license.py")

    needs_check = get_paths(args.root)
    missing = check_all(needs_check)
    handle_missing(missing, args.check)

if __name__ == "__main__":
    main()
