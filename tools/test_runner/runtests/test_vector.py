# © 2026 Massachusetts Institute of Technology
# MIT License

import logging, os, re
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

logger = logging.getLogger(__name__)

CONTAINER_VECTOR_DIR_NAME = "test_vector"
CANDO_VECTOR_FILE = "cando_vector.json"
CANDO_RESULT_FILE = "cando_result.json"
SETUP_FILE = "setup"
FILE_CHANGES_TARFILE = "file_changes.tar.gz"
TEST_RESOURCES = "resources"


@dataclass
class TestVector:
    """
    Representation of a test vector.
    Originally this was just a single JSON file, but has since expanded
    to also include a `setup` file, gzipped tar archive specified in `file_changes_tar`
    and a `resources` folder for test assets.
    Both formats are still allowed by this.

    File format (per test JSON) specified in `cando_vector`
    ---------------------------
    {
      "argv":  [ "...", ... ]  | None, # optional command-line args to pass to driver
      "stdin":  "..."          | None, # optional stdin input string
      "stdout": { "pattern",   | None, # optional expected stdout. pattern is the output string
                "is_regex" }   | None, # if is_regex is true, compile pattern as regex
      "stderr": { "pattern",   | None, # optional expected stderr. pattern is the error string
                "is_regex" }   | None, # if is_regex is true, compile pattern as regex
      "lib_state_in": {...}    | None, # The state of a program running a library function
      "lib_state_out": {...}   | None, # The state after library function execution
      "rc":     N              | None, # optional expected exit status (default 0)
      "has_ub": "..."          | None, # optional flag to indicate UB. Provides a text explanation
    }
    """

    name: str
    cando_vector: Path
    setup: Optional[Path] = None
    file_changes_tar: Optional[Path] = None
    test_resources: Optional[Path] = None

    @classmethod
    def from_path(cls, path: Path) -> "TestVector":
        """
        Constructs a test vector from the given `path`.
        `path` can either be a JSON vector file or a directory with a JSON vector
        file with optional setup scripts and file changes archive.
        """
        if path.is_dir():
            cando_vector = path / CANDO_VECTOR_FILE
            if not cando_vector.exists():
                raise FileNotFoundError(f"Couldn't find `CANDO_VECTOR_FILE`: {CANDO_VECTOR_FILE}")

            setup = path / SETUP_FILE
            file_changes_tar = path / FILE_CHANGES_TARFILE
            test_resources = path / TEST_RESOURCES
            return cls(
                path.name,
                cando_vector,
                setup if setup.exists() else None,
                file_changes_tar if file_changes_tar.exists() else None,
                test_resources if test_resources.exists() else None,
            )
        else:
            # Make sure it's a JSON file
            if path.suffix != ".json":
                raise ValueError(f"Given file was not JSON. Got name: {path}")
            return cls(path.name, path)


def get_vectors(test_case_root: Path, match_regexes: list[str] | None) -> list[Path]:
    """
    Returns the paths to test vectors for a given test case.

    If `match_regexes` is provided, only vectors whose path relative to `test_vectors/`
    matches at least one regex are returned.
    """
    test_vectors_dir = test_case_root / "test_vectors"
    if not test_vectors_dir.exists():
        raise FileNotFoundError(f"Couldn't find `test_vectors` directory at `{test_vectors_dir}`")

    rxes = [re.compile(r) for r in match_regexes or []]

    test_vectors = []
    for entry in os.scandir(test_vectors_dir):
        entry = Path(entry)
        if entry.is_dir() or entry.suffix == ".json":
            rel_name = str(entry.relative_to(test_vectors_dir))
            if rxes and not any(rx.search(rel_name) for rx in rxes):
                continue
            test_vectors.append(entry)

    test_vectors.sort(key=lambda v: v.name)
    return test_vectors
