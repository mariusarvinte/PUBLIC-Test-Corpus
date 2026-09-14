# © 2026 Massachusetts Institute of Technology
# MIT License

import gzip, logging, os, socket, tarfile
from io import BytesIO
from pathlib import Path
from typing import Optional

from .error import *
from .cli import parse_args
from .parse_falco import FilesystemChange, get_filesystem_changes
from .parse_volume import parse_volume
from .run_cando import CandoTestResult, run_cando
from .file_diff import get_filesystem_diff
from ..utils import CONTAINER_OUTPUT_DIR, CONTAINER_LOG_LABEL_ENV
from ..test_vector import FILE_CHANGES_TARFILE
from ..types import BenchMode

logger = logging.getLogger(__name__)
DELETIONS_TARFILE = ".cando/deletions.txt"


def configure_logging(log_level: str):
    """Configures the container logging to display both the vector name and container id"""
    vector_label = os.environ.get(CONTAINER_LOG_LABEL_ENV, "unknown")
    container_id = socket.gethostname()[:12]
    log_format = "%(levelname)s:%(name)s:[vector=%(vector_label)s container=%(container_id)s] %(message)s"

    base_factory = logging.getLogRecordFactory()

    def record_factory(*args, **kwargs):
        record = base_factory(*args, **kwargs)
        record.vector_label = vector_label
        record.container_id = container_id
        return record

    logging.setLogRecordFactory(record_factory)
    logging.basicConfig(level=log_level, format=log_format)


def write_tar_gz(filesystem_changes: list[FilesystemChange], output_path: Path):
    """
    Writes filesystem changes to a reproducible tar.gz archive using PAX format.
    Surviving paths are archived directly and deleted paths are stored in a reserved
    newline-delimited manifest.

    Args:
        filesystem_changes: the list of filesystem changes from Falco
        output_path: where to output the tarfile
    """
    if not filesystem_changes:
        return

    surviving_files = sorted(
        change.path for change in filesystem_changes if not change.is_deleted
    )
    deleted_files = sorted(
        f"{change.path}/" if change.is_dir else str(change.path)
        for change in filesystem_changes
        if change.is_deleted
    )

    # Using mtime=0 to make this not depend on when the tarfile was created
    # We do actually care about the uid, gid, uname, and gname so don't strip those
    with open(output_path, "wb") as raw:
        with gzip.GzipFile(fileobj=raw, mode="wb", mtime=0) as gz:
            with tarfile.open(
                fileobj=gz, mode="w|", format=tarfile.PAX_FORMAT, dereference=True
            ) as tfile:
                for file in surviving_files:
                    tar_info = tfile.gettarinfo(file)
                    tar_info.mtime = 0

                    # TODO: We need to figure out a good way to still capture this
                    # However, for now I'm going to get rid of it so they're reproducible
                    tar_info.uid = 0
                    tar_info.gid = 0
                    tar_info.uname = ""
                    tar_info.gname = ""

                    if tar_info.isfile():
                        with open(file, "rb") as f:
                            tfile.addfile(tar_info, f)
                    else:
                        tfile.addfile(tar_info)

                if deleted_files:
                    manifest = ("\n".join(deleted_files) + "\n").encode()
                    tar_info = tarfile.TarInfo(DELETIONS_TARFILE)
                    tar_info.size = len(manifest)
                    tar_info.mtime = 0
                    tfile.addfile(tar_info, BytesIO(manifest))


def fmt_diff(cando_diff: Optional[str], filesystem_diff: Optional[str]) -> str:
    """Format combined diff from cando and files"""
    diff = ""
    if cando_diff:
        # Need to add extra spacing to cando diff
        diff += "Cando Diff:\n  " + "\n  ".join(cando_diff.splitlines())
    if filesystem_diff:
        diff += "Filesystem Diff:\n  " + filesystem_diff
    return diff


def run(args):
    """
    Main entrypoint for running a test vector inside of a container

    Raises:
        Various errors derived from `BaseFail` that desribe an error from running a test vector
    """
    ctx = parse_volume(args.rust)
    logger.debug(f"Using context: {ctx}")

    bench_mode = BenchMode(args.bench) if args.bench else None
    cando_outcome = run_cando(ctx, args.rust, args.lib, args.timeout, bench_mode)

    if bench_mode:
        logger.debug(f"Benchmarking so skipping filesystem equivalence checking")
        if not cando_outcome.result_type == CandoTestResult.BENCHMARK:
            raise BenchmarkFailure("Ran cando with `--bench` but result wasn't 'Benchmark'")
        raise Benchmark()

    filesystem_changes = get_filesystem_changes()

    if args.no_compare:
        # Just dump the tar file
        tar_out = CONTAINER_OUTPUT_DIR / FILE_CHANGES_TARFILE
        write_tar_gz(filesystem_changes, tar_out)
        raise NoCompare(f"Dumped file changes to tar file `{tar_out}` (path inside container)")
    else:
        tar_file = ctx.test_vector.file_changes_tar
        if not tar_file and filesystem_changes:
            # NOTE: This could probably be an actual error, but because of existing B02 tests 
            # we will just note the issues and continue 
            logger.warning(f"Got changed files {filesystem_changes} but couldn't find a tarfile to compare to")

        if tar_file:
            filesystem_diff = get_filesystem_diff(filesystem_changes, tar_file)
        else:
            logger.debug("Skipping file comparison because vector tarfile not specified")
            filesystem_diff = None

        full_diff = fmt_diff(cando_outcome.diff, filesystem_diff)
        if full_diff:
            raise ComparisonFail(full_diff)


def main() -> VectorExitCode:
    try:
        args = parse_args()
        configure_logging(args.log_level)
        run(args)
        return VectorExitCode.PASS
    except (Skip, NoCompare, Benchmark) as e:
        logger.info(e)
        return e.exit_code
    except BaseFail as e:
        if isinstance(e, InternalError):
            logger.exception(e)
        else:
            logger.error(e)
        return e.exit_code
    except Exception as e:
        # Anything else is an InternalError
        logger.exception("An internal error occurred: %s", e)
        return VectorExitCode.INTERNAL_ERROR


if __name__ == "__main__":
    raise SystemExit(main().value)
