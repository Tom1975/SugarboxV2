"""
Protocol integration tests for the Sugarbox debug server.

Each test calls the corresponding helper from test_step.py and asserts
zero failures.  The 'emulator' fixture (conftest.py) owns the process
lifecycle; all tests share the same session-scoped connection.
"""

import pytest

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


# ── Reconnect (runs after connection is closed at session teardown) ───────────
# This test is intentionally last; it opens its own connection.

def test_reconnect(emulator):
    sock, reader = emulator
    host = sock.getpeername()[0]
    port = sock.getpeername()[1]
    # Close shared connection so the server loops back to accept()
    reader.close()
    sock.close()
    assert _disconnect_reconnect(host, port) == 0
