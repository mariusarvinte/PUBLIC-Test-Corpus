# © 2026 Massachusetts Institute of Technology
# MIT License

import tarfile, os, stat 
import xxhash
from difflib import unified_diff
from pathlib import Path
from dataclasses import dataclass
from typing import IO, Optional

from .error import InternalError
from .parse_falco import FilesystemChange

DELETIONS_TARFILE = ".cando/deletions.txt"
IGNORED_FILE_PATHS = [".git/index"]


def ignored_file(path: str) -> bool:
    for ignored in IGNORED_FILE_PATHS:
        if ignored in str(path):
            return True
    return False


def compute_xxhash(ref: Path | IO[bytes]) -> str:
    """Computes xxhash checksum for file comparisons of the given `ref`"""
    if isinstance(ref, Path):
        with open(ref, "rb") as f:
            content = f.read()
    else:
        content = ref.read()
    
    return xxhash.xxh64(content).hexdigest()


@dataclass
class FileInfo:
    """
    Information about a file that we care about for a diff.
    This is subject to change.
    """

    path: str  # Includes a trailing '/' if the path is a directory
    xxhash_checksum: Optional[str]  # Not present for directories
    permissions: (
        int  # decimal representation of permissions (e.g., "rwxrwxrwx" (777) = 511)
    )
    is_deleted: bool = False

    @classmethod
    def from_path(cls, path: Path) -> "FileInfo":
        """Create `FileInfo` from a given path. Raises error if doesn't exist"""
        if not path.exists():
            raise FileNotFoundError(f"Couldn't find file at `{path}`")

        permissions = stat.S_IMODE(os.stat(path).st_mode)

        if path.is_file():
            str_path = str(path)
            xxhash_checksum = None if ignored_file(str_path) else compute_xxhash(path)
        elif path.is_dir():
            str_path = str(path) + "/"
            xxhash_checksum = None
        else:
            raise InternalError("Not currently supporting non file/directory paths")

        return cls(
            path=str_path,
            xxhash_checksum=xxhash_checksum,
            permissions=permissions,
        )

    @classmethod
    def from_tar(cls, tar_file: tarfile.TarFile, member: tarfile.TarInfo) -> "FileInfo":
        """
        Create `FileInfo` from a member within a tarfile

        Args:
            tar_file: the TarFile struct that `member` is from
            member: the member of `tar_file` to parse into FileInfo

        Raises:
            InternalError: miscellaneous errors
        """
        permissions = stat.S_IMODE(member.mode)

        if member.isfile():
            file = tar_file.extractfile(member)
            if file is None:
                raise InternalError("Couldn't extract content from tarfile member: `{member}`")

            str_path = member.name
            xxhash_checksum = None if ignored_file(str_path) else compute_xxhash(file)
        elif member.isdir():
            xxhash_checksum = None
            str_path = member.name + "/"
        else:
            raise InternalError("Not currently supporting non file/directory paths")

        return cls(
            path="/"
            + str_path,  # When using `.name` we lose the initial '/' so add it back to compare to changed files
            xxhash_checksum=xxhash_checksum,
            permissions=permissions,
        )

    @classmethod
    def from_deleted(cls, path: Path | str, is_dir: bool = False) -> "FileInfo":
        """Create 'FileInfo' from given `path` as a deleted file"""
        str_path = str(path)
        if is_dir and not str_path.endswith("/"):
            str_path += "/"

        return cls(path=str_path, xxhash_checksum=None, permissions=0, is_deleted=True)

    def __str__(self) -> str:
        if self.is_deleted:
            return f"{self.path} | DELETED"

        metadata = f"{self.path} | perms={oct(self.permissions)}"
        if self.xxhash_checksum:
            metadata += f" | xxhash={self.xxhash_checksum}"
        return metadata

    def is_dir(self) -> bool:
        return self.path.endswith("/")


def get_filesystem_diff(
    filesystem_changes: list[FilesystemChange], expected_changes_tar: Path
) -> str:
    """
    Gets the unified diff between `filesystem_changes` (changed during the current
    execution of this container) and those expected in `expected_changes_tar`.

    Returns:
        str with diff or empty string if there is no diff

    Raises:
        InternalError: miscellaneous errors
    """
    got = []
    for change in filesystem_changes:
        if change.is_deleted:
            got.append(FileInfo.from_deleted(change.path, is_dir=change.is_dir))
        else:
            got.append(FileInfo.from_path(change.path))

    expected = []
    with tarfile.open(expected_changes_tar) as tar_file:
        for member in tar_file:
            if member.name == DELETIONS_TARFILE:
                manifest_file = tar_file.extractfile(member)
                if manifest_file is None:
                    raise InternalError(f"Couldn't extract content from tarfile member: `{member}`")

                for deleted_path in manifest_file.read().decode().splitlines():
                    deleted_path = deleted_path.strip()
                    if deleted_path:
                        expected.append(
                            FileInfo.from_deleted(
                                deleted_path.rstrip("/"),
                                is_dir=deleted_path.endswith("/"),
                            )
                        )
                continue

            expected.append(FileInfo.from_tar(tar_file, member))

    # Build parallel lists for unified diff to handle formatting
    expected_lines: list[str] = []
    got_lines: list[str] = []

    expected_dict = {f.path: f for f in expected}
    got_dict = {f.path: f for f in got}
    all_paths = sorted(expected_dict.keys() | got_dict.keys())

    # Use empty string to signify that the file is not in one of the lists but is in the other
    for path in all_paths:
        in_expected = expected_dict.get(path)
        if in_expected:
            expected_lines.append(str(in_expected))
        else:
            expected_lines.append("")

        in_got = got_dict.get(path)
        if in_got:
            got_lines.append(str(in_got))
        else:
            got_lines.append("")

    diff = unified_diff(
        expected_lines, got_lines, fromfile="Expected", tofile="Got", lineterm=""
    )
    return "\n  ".join(diff)
