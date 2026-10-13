"""
Protocol integration tests for the Sugarbox debug server.

Each test calls the corresponding helper from test_step.py and asserts
zero failures.  The 'emulator' fixture (conftest.py) owns the process
lifecycle; all tests share the same session-scoped connection.
"""

import json
import socket as _socket

import pytest

from test_step import get_pc, recv_msg, send
from test_step import (
    test_nop_steps            as _nop_steps,
    test_call_stepover        as _call_stepover,
    test_conditional_call_stepover as _conditional_call_stepover,
    test_djnz_stepover        as _djnz_stepover,
    test_rst_stepover         as _rst_stepover,
    test_ldir_stepover        as _ldir_stepover,
    test_step_in              as _step_in,
    test_step_out             as _step_out,
    test_set_breakpoints      as _set_breakpoints,
    test_read_memory          as _read_memory,
    test_set_registers_primed as _set_registers_primed,
    test_async_events         as _async_events,
    test_restart              as _restart,
    test_disconnect_reconnect as _disconnect_reconnect,
)


# ── Step-over tests ──────────────────────────────────────────────────────────

def test_nop(emulator):
    sock, reader = emulator
    assert _nop_steps(sock, reader) == 0


def test_call(emulator):
    sock, reader = emulator
    assert _call_stepover(sock, reader) == 0


def test_conditional_call(emulator):
    sock, reader = emulator
    assert _conditional_call_stepover(sock, reader) == 0


def test_djnz(emulator):
    sock, reader = emulator
    assert _djnz_stepover(sock, reader) == 0


def test_rst(emulator):
    sock, reader = emulator
    assert _rst_stepover(sock, reader) == 0


def test_ldir(emulator):
    sock, reader = emulator
    assert _ldir_stepover(sock, reader) == 0


# ── Step-in / step-out ───────────────────────────────────────────────────────

def test_stepin(emulator):
    sock, reader = emulator
    assert _step_in(sock, reader) == 0


def test_stepout(emulator):
    sock, reader = emulator
    assert _step_out(sock, reader) == 0


# ── Breakpoints ──────────────────────────────────────────────────────────────

def test_breakpoints(emulator):
    sock, reader = emulator
    assert _set_breakpoints(sock, reader) == 0


# ── Memory / registers ───────────────────────────────────────────────────────

def test_memory(emulator):
    sock, reader = emulator
    assert _read_memory(sock, reader) == 0


def test_registers_primed(emulator):
    sock, reader = emulator
    assert _set_registers_primed(sock, reader) == 0


# ── Async events ─────────────────────────────────────────────────────────────

def test_async(emulator):
    sock, reader = emulator
    assert _async_events(sock, reader) == 0


# ── Reset ────────────────────────────────────────────────────────────────────

def test_reset(emulator):
    sock, reader = emulator
    assert _restart(sock, reader) == 0


# ── Malformed requests ───────────────────────────────────────────────────────
# Each of these used to terminate Sugarbox (an uncaught JSON type_error) or
# make it index past a fixed array (bank, drive, side, track). The server has
# to answer with an error and keep serving.

_MALFORMED = [
    ("not_an_object_number", "42"),
    ("not_an_object_array", "[1,2]"),
    ("address_as_string", {"cmd": "readMemory", "address": "0x100", "size": 4}),
    ("bytes_not_an_array", {"cmd": "writeMemory", "address": 0x4000, "bytes": 5}),
    ("pc_as_string", {"cmd": "setRegisters", "pc": "abc"}),
    ("setpc_as_string", {"cmd": "setPC", "address": "abc"}),
    ("settings_not_an_array", {"cmd": "setEmulatorSettings", "settings": 3}),
    ("read_ram_bank_out_of_range", {"cmd": "readMemory", "address": 0, "size": 16, "memType": "ram", "bank": 100}),
    ("read_rom_bank_out_of_range", {"cmd": "readMemory", "address": 0, "size": 16, "memType": "rom", "bank": 100000}),
    ("disassemble_bank_out_of_range", {"cmd": "disassemble", "address": 0, "count": 4, "memType": "ram", "bank": 100}),
    ("insert_disk_drive_out_of_range", {"cmd": "insertDisk", "drive": 5, "path": "/nonexistent.dsk"}),
    ("insert_disk_negative_drive", {"cmd": "insertDisk", "drive": -1, "path": "/nonexistent.dsk"}),
    ("track_raw_negative_drive", {"cmd": "getTrackRaw", "drive": -1, "side": 0, "track": 0}),
    ("track_raw_negative_side", {"cmd": "getTrackRaw", "drive": 0, "side": -1, "track": 0}),
    ("track_raw_negative_track", {"cmd": "getTrackRaw", "drive": 0, "side": 0, "track": -1}),
]


@pytest.mark.parametrize("payload", [p for _, p in _MALFORMED], ids=[n for n, _ in _MALFORMED])
def test_malformed_request_is_refused(emulator, payload):
    sock, reader = emulator
    line = payload if isinstance(payload, str) else json.dumps(payload)
    sock.sendall((line + "\n").encode())
    assert "error" in recv_msg(reader)
    # Still serving.
    assert isinstance(get_pc(sock, reader), int)


def test_evaluate_with_bank_out_of_range_gives_no_value(emulator):
    sock, reader = emulator
    resp = send(sock, reader, {"cmd": "evaluate", "expression": "rom[999]:0x0000"})
    assert resp.get("text") == "?"
    assert isinstance(get_pc(sock, reader), int)


# ── Reconnect (runs after connection is closed at session teardown) ───────────
# This test is intentionally last; it opens its own connection.

def test_reconnect(emulator):
    sock, reader = emulator[0], emulator[1]
    host, port = sock.getpeername()
    # Close shared connection so the server loops back to accept()
    reader.close()
    sock.close()
    assert _disconnect_reconnect(host, port) == 0
    # Restore the shared connection so later tests (conformance) can use it.
    new_sock = _socket.create_connection((host, port), timeout=10)
    emulator[0] = new_sock
    emulator[1] = new_sock.makefile("r")
