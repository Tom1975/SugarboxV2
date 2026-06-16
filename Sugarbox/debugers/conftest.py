"""
pytest fixture: launches Sugarbox headlessly and opens a TCP connection
to the debug server.  Tests import helpers from test_step.py.

Usage:
    cd Sugarbox/debugers
    pytest test_protocol.py -v

Environment variables:
    SUGARBOX_BINARY   path to Sugarbox binary  (default: ../../build/Sugarbox/Sugarbox)
    SUGARBOX_PORT     TCP port for debug server (default: 1234)
"""

import os
import shutil
import signal
import socket
import subprocess
import time

import pytest

REPO_ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEFAULT_BINARY = os.path.join(REPO_ROOT, "build", "Sugarbox", "Sugarbox")
STARTUP_TIMEOUT = 20  # seconds


@pytest.fixture(scope="session")
def emulator():
    binary = os.environ.get("SUGARBOX_BINARY", DEFAULT_BINARY)
    port   = int(os.environ.get("SUGARBOX_PORT", "1234"))

    if not os.path.isfile(binary):
        pytest.skip(f"Sugarbox binary not found: {binary}")

    cmd = [binary, "--hide", "-d", "--ds", str(port)]
    if shutil.which("xvfb-run"):
        cmd = ["xvfb-run", "-a"] + cmd

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        start_new_session=True,
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
                    "Check that the binary runs with --hide -d --ds flags."
                )
            time.sleep(0.3)

    if sock is None:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        pytest.fail(f"Debug server did not open on port {port} within {STARTUP_TIMEOUT}s")

    reader = sock.makefile("r")
    yield sock, reader

    reader.close()
    sock.close()
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        proc.wait(timeout=5)
    except (ProcessLookupError, subprocess.TimeoutExpired):
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
