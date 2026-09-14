# © 2026 Massachusetts Institute of Technology
# MIT License

import logging, json, socket, time
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Tuple

from ..utils import CONTAINER_FALCO_LOG_DIR, FALCO_LOG_FILE

"""
Parse Falco syscall event logs and extract files that were created, deleted,
content-modified, or had permissions changed.
"""

logger = logging.getLogger(__name__)


@dataclass()
class FilesystemChange:
    path: Path
    is_deleted: bool
    is_dir: bool  # With deleted files you can't use path.is_dir()


# Falco event type classifications
CONTENT_MODIFIED = {"open", "openat", "write", "rename", "renameat"}
PERMISSION_CHANGED = {"chmod", "fchmod", "fchmodat"}
CREATED = {"mkdir", "mkdirat", "creat", "symlink", "symlinkat"}
DELETED = {"unlink", "unlinkat", "rmdir"}


# open/openat is write-intent only if opened with O_WRONLY/O_RDWR. If the flags also
# include O_CREAT, we treat it as creation so create-then-delete temp files can be ignored.
# Rename: source is deleted, target is created/modified.
def classify_falco_event(evt_type: str, fields: dict) -> list[Tuple[str, str]]:
    """Classifies Falco events from logs. Returns as (path, event category)"""
    results = []

    name = fields.get("fs.path.name")
    src = fields.get("fs.path.source")
    tgt = fields.get("fs.path.target")
    open_flags = fields.get("evt.arg.flags", "")

    if evt_type in CREATED:
        if evt_type == "symlink":
            # source is the new symlink path
            if src:
                results.append((src, "created"))
        elif name:
            results.append((name, "created"))

    elif evt_type in DELETED:
        if name:
            results.append((name, "deleted_dir" if evt_type == "rmdir" else "deleted"))

    elif evt_type in PERMISSION_CHANGED:
        if name:
            results.append((name, "permission_changed"))

    elif evt_type in {"rename", "renameat"}:
        # source path ceases to exist; target path is written/created
        if src:
            src_category = "deleted_dir" if tgt and Path(tgt).is_dir() else "deleted"
            results.append((src, src_category))
        if tgt:
            results.append((tgt, "content_modified"))

    elif evt_type in {"open", "openat"}:
        if name:
            category = "created" if "O_CREAT" in open_flags else "content_modified"
            results.append((name, category))

    elif evt_type in CONTENT_MODIFIED:
        if name:
            results.append((name, "content_modified"))

    return results


def parse_falco(hostname: str) -> dict[str, set[str]]:
    """Parses the Falco logs looking for events associated with this container"""
    # FIXME: Waiting 0.5 seconds here because it seems to occasionally take some time
    # for the Falco events to populate the log file, and I was running into sporadic errors.
    # I'm not sure if there's a better way to do this
    time.sleep(0.5)
    affected = defaultdict(set)

    falco_log = CONTAINER_FALCO_LOG_DIR / FALCO_LOG_FILE
    with open(falco_log, "r") as f:
        for lineno, line in enumerate(f):
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError as e:
                logger.warning(
                    f"Line {lineno}: JSON parse error: {e} for line `{line}`"
                )
                continue

            fields = rec.get("output_fields", {})
            evt_type = fields.get("evt.type", "")

            if fields.get("container.id", "") != str(hostname):
                continue

            for file_path, category in classify_falco_event(evt_type, fields):
                affected[file_path].add(category)

    return affected


def get_filesystem_changes() -> list[FilesystemChange]:
    """Returns the final filesystem changes observed for this container run."""

    # The container's kernel hostname matches `container.id` as logged by Falco. We avoid
    # the HOSTNAME env var because the orchestrator copies the host's environment into the
    # container, which clobbers HOSTNAME with the host's name.
    hostname = socket.gethostname()
    logging.debug(f"Processing falco output with hostname: {hostname}")

    changed = parse_falco(hostname)

    filesystem_changes = []
    for path_str, categories in changed.items():
        path = Path(path_str)

        if path.exists():
            filesystem_changes.append(
                FilesystemChange(path, is_deleted=False, is_dir=path.is_dir())
            )
        # Ignore create-then-delete temporary paths; they do not affect final state.
        elif "created" not in categories:
            if "deleted_dir" in categories or "deleted" in categories:
                is_dir = "deleted_dir" in categories
                filesystem_changes.append(
                    FilesystemChange(path, is_deleted=True, is_dir=is_dir)
                )

    filesystem_changes.sort(key=lambda change: (str(change.path), change.is_deleted))
    logger.debug(f"Got filesystem changes: {filesystem_changes}")
    return filesystem_changes
