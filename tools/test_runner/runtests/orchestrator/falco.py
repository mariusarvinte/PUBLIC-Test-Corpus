# © 2026 Massachusetts Institute of Technology
# MIT License

import docker, logging, time, shutil, os
from typing import Optional
from pathlib import Path
from tempfile import mkdtemp
from docker.models.containers import Container
from docker.errors import APIError, NotFound

from ..utils import CONTAINER_FALCO_LOG_DIR, FALCO_LOG_FILE

logger = logging.getLogger(__name__)

FALCO_DOCKER = "falcosecurity/falco"
FALCO_IMAGE_DIGEST = "sha256:b4166a61f41e2fa638c041cac881d8bb32c284e3aaf282fdfed80f15f6eb55e3"  # Version 0.43.1
FALCO_IMAGE_NAME = f"{FALCO_DOCKER}@{FALCO_IMAGE_DIGEST}"
FALCO_CONTAINER_NAME = "tractor-falco"


class FalcoManager:
    def __init__(self) -> None:
        """Initializes FalcoManager with tmpdir for the logging directory"""
        self._container: Optional[Container] = None
        self._client = docker.from_env()
        self._host_log_dir = Path(mkdtemp(dir="/tmp", prefix="tractor_falco_"))

    def __enter__(self):
        """Context manager setup"""
        self.start()
        return self

    def __exit__(self, *_):
        """Context manager cleanup"""
        self.stop()

    def start(self) -> None:
        """Starts the Falco container if not already started"""
        if self._container is not None:
            logger.warning(f"Falco container is already running")
            return

        # Check if Falco container is already running
        try:
            old_falco = self._client.containers.get(FALCO_CONTAINER_NAME)
            old_falco.remove(force=True)
            logger.info(f"Removed existing container `{FALCO_CONTAINER_NAME}`")
        except NotFound:
            pass

        logger.info(f"Starting `{FALCO_IMAGE_NAME}` image")

        # Create the host logfile
        falco_log = self._host_log_dir / FALCO_LOG_FILE
        falco_log.touch()
        # Anyone can read/write to these files (necessary because of container permissions)
        os.chmod(self._host_log_dir, 0o777)
        os.chmod(falco_log, 0o666)

        # Volume paths needed for Falco configuration
        volumes = {
            "/sys/kernel/tracing": {
                "bind": "/sys/kernel/tracing",
                "mode": "ro",
            },
            "/proc": {
                "bind": "/host/proc",
                "mode": "ro",
            },
            "/etc": {
                "bind": "/host/etc",
                "mode": "ro",
            },
            str(Path(__file__).parent / "falco_configs" / "falco.yaml"): {
                "bind": "/etc/falco/falco.yaml",
                "mode": "ro",
            },
            str(Path(__file__).parent / "falco_configs" / "files.yaml"): {
                "bind": "/etc/falco/falco_rules.yaml",
                "mode": "ro",
            },
            "/var/run/docker.sock": {
                "bind": "/host/var/run/docker.sock",
                "mode": "ro",
            },
            str(self._host_log_dir): {
                "bind": str(CONTAINER_FALCO_LOG_DIR),
                "mode": "rw",
            },
        }

        self._container = self._client.containers.run(
            image=FALCO_IMAGE_NAME,
            name=FALCO_CONTAINER_NAME,
            volumes=volumes,
            entrypoint="falco -U",  # run unbuffered, costs CPU but allows us to pull the results immediately
            detach=True,
            auto_remove=True,
            cap_drop=["ALL"],
            cap_add=["SYS_ADMIN", "SYS_RESOURCE", "SYS_PTRACE"],
        )

        # FIXME: We need to wait a little bit for the Falco container to initialize this
        # Ideally there would be some sort of the healthcheck or at the very least parse
        # the logs but alas we wait a second
        time.sleep(1)

    def stop(self) -> None:
        """Stops the Falco container if not already stopped"""
        logger.info("Stopping Falco container")
        if self._container:
            try:
                self._container.stop()
            except APIError as e:
                logger.warning(f"Failed to stop Falco container `{e}`")
            self._container = None
        shutil.rmtree(self._host_log_dir, ignore_errors=True)

    def get_host_log_dir(self) -> Path:
        return self._host_log_dir
