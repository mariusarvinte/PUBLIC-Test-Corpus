# © 2026 Massachusetts Institute of Technology
# MIT License

import logging, signal, docker, multiprocessing
from concurrent.futures import ProcessPoolExecutor
from docker.errors import NotFound, APIError
from typing import Optional

from .falco import FalcoManager
from ..utils import CONTAINER_LABEL_NAME

logger = logging.getLogger(__name__)


class _ShutdownHandler:
    """Gracefully handle shutdown (kill active Falco container, kill executor jobs, kill containers)"""

    def __init__(self) -> None:
        self.falco_manager: Optional[FalcoManager] = None
        self.executor: Optional[ProcessPoolExecutor] = None
        self._cleanup_started = multiprocessing.Event()
        self._installed = False

    def install(self) -> None:
        """Installs the signal handler for the current process. Should only be called once"""
        if self._installed:
            return

        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)
        self._installed = True

    def _signal_handler(self, sig, frame):
        _ = frame
        signal_name = signal.Signals(sig).name

        if self._cleanup_started.is_set():
            logger.warning(f"Received {signal_name} again. Already cleaning up")
            return

        logger.warning(f"Received {signal_name}")
        self.cleanup()
        raise KeyboardInterrupt

    def cleanup(self):
        if self._cleanup_started.is_set():
            return
        logger.info("Starting cleanup")

        self._cleanup_started.set()
        self._shutdown_executor()
        self._cleanup_containers()
        self._cleanup_falco()

    def _shutdown_executor(self) -> None:
        """Kills the executor and all of it's workers"""
        logger.info("Removing executor workers")
        if self.executor:
            try:
                self.executor.terminate_workers()
            except Exception as e:
                logger.warning(f"Failed to cancel executor futures: {e}")
            self.executor = None

    def _cleanup_containers(self) -> None:
        """Forcefully removes all the containers that have `CONTAINER_LABEL_NAME`"""
        logger.info("Removing test vector Docker containers")
        client = docker.from_env()
        try:
            containers = client.containers.list(
                ignore_removed=True,
                sparse=True,
                all=True,
                filters={"label": [f"{CONTAINER_LABEL_NAME}=true"]},
            )
        except Exception as e:
            logger.warning(f"Failed to enumerate test vector containers: {e}")
            return

        for container in containers:
            try:
                container.remove(force=True)
            except NotFound:
                continue
            except APIError as e:
                logger.warning(
                    f"Failed to remove test vector container `{container}`: {e}"
                )

    def _cleanup_falco(self) -> None:
        """Kills the Falco container"""
        if self.falco_manager:
            try:
                self.falco_manager.stop()
            except Exception as e:
                logger.warning(f"Failed to stop Falco during shutdown: {e}")
            self.falco_manager = None

    def register_falco(self, falco_manager: Optional[FalcoManager]) -> None:
        """Registers Falco for cleanup"""
        self.falco_manager = falco_manager

    def register_executor(self, executor: ProcessPoolExecutor) -> None:
        """Registers ProcessPoolExecutor for cleanup"""
        self.executor = executor

    def unregister_executor(self) -> None:
        """Removes the current ProcessPoolExecutor from cleanup"""
        self.executor = None

    def cleanup_started(self) -> bool:
        """Returns True if cleanup has already started"""
        return self._cleanup_started.is_set()


shutdown_handler = _ShutdownHandler()
