"""
pytest fixture: launches Sugarbox headlessly and opens a TCP connection
to the debug server.  Tests import helpers from test_step.py.

Usage:
    cd Sugarbox/debugers
    pytest test_protocol.py test_conformance.py -v

Environment variables:
    SUGARBOX_BINARY   path to Sugarbox binary  (default: ../../build/Sugarbox/Sugarbox)
    SUGARBOX_PORT     TCP port for debug server (default: 1234)
"""

import os
import platform
import shutil
import signal
import socket
import subprocess
import time

import pytest

REPO_ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
_IS_WINDOWS = platform.system() == "Windows"
_IS_MACOS   = platform.system() == "Darwin"

# Default binary path: .exe on Windows, plain executable elsewhere.
# On macOS a Qt app may be bundled as Sugarbox.app — check both locations.
def _default_binary() -> str:
    if _IS_WINDOWS:
        return os.path.join(REPO_ROOT, "build", "Sugarbox", "Release", "Sugarbox.exe")
    if _IS_MACOS:
        # Prefer plain binary; fall back to .app bundle
        plain = os.path.join(REPO_ROOT, "build", "Sugarbox", "Sugarbox")
        bundle = os.path.join(REPO_ROOT, "build", "Sugarbox",
                              "Sugarbox.app", "Contents", "MacOS", "Sugarbox")
        return plain if os.path.isfile(plain) else bundle
    return os.path.join(REPO_ROOT, "build", "Sugarbox", "Sugarbox")


STARTUP_TIMEOUT = 30  # seconds (Windows CI can be slower)


def _kill(proc: subprocess.Popen) -> None:
    """Terminate the process tree cross-platform."""
    if _IS_WINDOWS:
        proc.kill()
    else:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except (ProcessLookupError, OSError):
            pass


def _terminate(proc: subprocess.Popen) -> None:
    """Gracefully terminate the process tree cross-platform."""
    if _IS_WINDOWS:
        proc.terminate()
    else:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except (ProcessLookupError, OSError):
            pass


@pytest.fixture(scope="session")
def emulator():
    binary = os.environ.get("SUGARBOX_BINARY", _default_binary())
    port   = int(os.environ.get("SUGARBOX_PORT", "1234"))

    if not os.path.isfile(binary):
        pytest.skip(f"Sugarbox binary not found: {binary}")

    cmd = [binary, "--hide", "-d", "--ds", str(port)]

    # Linux headless: wrap with xvfb-run when available
    if not _IS_WINDOWS and not _IS_MACOS and shutil.which("xvfb-run"):
        cmd = ["xvfb-run", "-a"] + cmd

    # macOS headless: use offscreen platform when no display is available
    extra_env = {}
    if _IS_MACOS:
        extra_env["QT_QPA_PLATFORM"] = os.environ.get("QT_QPA_PLATFORM", "")

    proc_env = {**os.environ, **extra_env} if extra_env else None

    # On Unix, start_new_session=True creates a new process group so we can
    # kill the whole subtree.  On Windows it opens a new console (harmless).
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        start_new_session=not _IS_WINDOWS,
        env=proc_env,
    )

    sock = None
    deadline = time.time() + STARTUP_TIMEOUT
    while time.time() < deadline:
        try:
            sock = socket.create_connection(("127.0.0.1", port), timeout=1)
            break
        except (ConnectionRefusedError, OSError):
            if proc.poll() is not None:
                pytest.fail(
                    f"Sugarbox exited prematurely (rc={proc.returncode}). "
                    "Check that the binary supports --hide -d --ds flags."
                )
            time.sleep(0.3)

    if sock is None:
        _kill(proc)
        pytest.fail(f"Debug server did not open on port {port} within {STARTUP_TIMEOUT}s")

    reader = sock.makefile("r")
    # Yield a mutable list so test_reconnect can replace the socket after closing it.
    shared = [sock, reader]
    yield shared

    shared[1].close()
    shared[0].close()
    _terminate(proc)
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        _kill(proc)
