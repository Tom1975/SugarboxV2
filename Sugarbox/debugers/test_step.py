#!/usr/bin/env python3
"""
Test script for step-over behavior.

Usage:
    python3 test_step.py [--host 127.0.0.1] [--port 1234]

Requires Sugarbox running with: --hide -d --ds 1234
"""

import socket
import json
import time
import argparse
import sys


def connect(host, port, timeout=5):
    s = socket.create_connection((host, port), timeout=timeout)
    return s, s.makefile("r")


def recv_msg(reader):
    """Read one JSON message, skipping async stop events."""
    while True:
        line = reader.readline()
        if not line:
            raise EOFError("Connection closed by server")
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            continue
        # Skip async stop events sent by NotifyStop
        if msg.get("type") == "event":
            continue
        return msg


def recv_event(sock, reader, timeout=2.0):
    """Read messages until an async stop event arrives, discarding responses."""
    old_timeout = sock.gettimeout()
    sock.settimeout(timeout)
    try:
        while True:
            line = reader.readline()
            if not line:
                raise EOFError("Connection closed by server")
            try:
                msg = json.loads(line.strip())
            except json.JSONDecodeError:
                continue
            if msg.get("type") == "event":
                return msg
    except OSError:
        raise TimeoutError(f"No async event received within {timeout}s")
    finally:
        sock.settimeout(old_timeout)


def send(sock, reader, cmd):
    sock.sendall((json.dumps(cmd) + "\n").encode())
    return recv_msg(reader)


def get_state(sock, reader):
    return send(sock, reader, {"cmd": "getState"})


def get_pc(sock, reader):
    return get_state(sock, reader)["pc"]


def wait_running(sock, reader, timeout=1.0):
    """Poll getState until running == 'true'."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        state = get_state(sock, reader)
        if state.get("running") == "true":
            return
        time.sleep(0.02)


def wait_stopped(sock, reader, timeout=2.0):
    """Poll getState until running == 'false'."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        state = get_state(sock, reader)
        if state.get("running") == "false":
            return state["pc"]
        time.sleep(0.02)
    raise TimeoutError("Emulator did not stop within timeout")


def step(sock, reader):
    """Send step command, wait for emulator to stop, return new PC."""
    send(sock, reader, {"cmd": "step"})
    return wait_stopped(sock, reader)


def set_pc(sock, reader, address):
    send(sock, reader, {"cmd": "setPC", "address": address})


def write_mem(sock, reader, address, bytes_list):
    return send(sock, reader, {"cmd": "writeMemory", "address": address, "bytes": bytes_list})


def set_regs(sock, reader, **kwargs):
    return send(sock, reader, {"cmd": "setRegisters", **kwargs})


def halt(sock, reader):
    send(sock, reader, {"cmd": "halt"})


def check(label, got, expected):
    ok = got == expected
    status = "OK" if ok else "FAIL"
    print(f"  [{status}] {label:45s} got={hex(got)}  expected={hex(expected)}")
    return ok


# All test code lives in RAM (0x4000–0x8FFF, pure read/write RAM on CPC)

def test_nop_steps(sock, reader):
    """Step over 5 consecutive NOPs: PC must increment by 1 each time."""
    print("\n=== Step over NOPs ===")
    failures = 0

    BASE = 0x4000
    write_mem(sock, reader, BASE, [0x00] * 6)  # 6 NOPs
    halt(sock, reader)
    set_pc(sock, reader, BASE)

    for i in range(5):
        pc = step(sock, reader)
        failures += 0 if check(f"NOP #{i+1} → PC={hex(BASE+i+1)}", pc, BASE + i + 1) else 1

    return failures


def test_call_stepover(sock, reader):
    """Step over CALL nn: PC must land at PC+3, not inside the subroutine."""
    print("\n=== Step over CALL nn ===")
    failures = 0

    # 0x4100: CALL 0x4200   (CD 00 42)
    # 0x4103: NOP            ← expected landing
    # 0x4200: NOP + RET      (subroutine)
    write_mem(sock, reader, 0x4100, [0xCD, 0x00, 0x42])  # CALL 0x4200
    write_mem(sock, reader, 0x4103, [0x00])
    write_mem(sock, reader, 0x4200, [0x00, 0xC9])         # NOP + RET

    halt(sock, reader)
    set_pc(sock, reader, 0x4100)
    set_regs(sock, reader, sp=0x7F00)  # SP in RAM so CALL/RET work

    pc = step(sock, reader)
    failures += 0 if check("CALL 0x4200 step-over → PC=0x4103", pc, 0x4103) else 1

    return failures


def test_conditional_call_stepover(sock, reader):
    """Step over CALL Z,nn with Z flag set: must skip the call."""
    print("\n=== Step over CALL Z,nn (Z flag set) ===")
    failures = 0

    # 0x4300: XOR A          (AF)   sets Z flag
    # 0x4301: CALL Z,0x4400  (CC 00 44)
    # 0x4304: NOP             ← landing
    # 0x4400: NOP + RET
    write_mem(sock, reader, 0x4300, [0xAF])               # XOR A
    write_mem(sock, reader, 0x4301, [0xCC, 0x00, 0x44])   # CALL Z,0x4400
    write_mem(sock, reader, 0x4304, [0x00])
    write_mem(sock, reader, 0x4400, [0x00, 0xC9])

    halt(sock, reader)
    set_pc(sock, reader, 0x4300)
    set_regs(sock, reader, sp=0x7F00)
    step(sock, reader)  # XOR A

    pc = get_pc(sock, reader)
    failures += 0 if check("After XOR A → PC=0x4301", pc, 0x4301) else 1

    pc = step(sock, reader)
    failures += 0 if check("CALL Z step-over → PC=0x4304", pc, 0x4304) else 1

    return failures


def test_djnz_stepover(sock, reader):
    """Step over DJNZ with B=5: must land at PC+2, not loop."""
    print("\n=== Step over DJNZ (B=5) ===")
    failures = 0

    # 0x4500: LD B,5         (06 05)
    # 0x4502: DJNZ 0x4502    (10 FE)  ← infinite loop on itself
    # 0x4504: NOP             ← landing after step-over
    write_mem(sock, reader, 0x4500, [0x06, 0x05])   # LD B,5
    write_mem(sock, reader, 0x4502, [0x10, 0xFE])   # DJNZ $
    write_mem(sock, reader, 0x4504, [0x00])

    halt(sock, reader)
    set_pc(sock, reader, 0x4500)
    step(sock, reader)  # LD B,5

    pc = get_pc(sock, reader)
    failures += 0 if check("After LD B,5 → PC=0x4502", pc, 0x4502) else 1

    pc = step(sock, reader)
    failures += 0 if check("DJNZ step-over → PC=0x4504", pc, 0x4504) else 1

    return failures


def test_rst_stepover(sock, reader):
    """Step over RST 8: PC must land at PC+1, not inside handler."""
    print("\n=== Step over RST 8 ===")
    failures = 0

    # RST 8 jumps to 0x0008 — put a RET there (via setPC trick, write to RAM area)
    # We need 0x0008 to have a RET. But 0x0008 is ROM-mapped.
    # Workaround: use RST 0x38 (FF) which jumps to 0x0038, also ROM.
    # Instead: use a CALL to a known RAM subroutine to simulate the same test.
    # → test RST with 0x4600 area but jump vector at 0x0038 is also ROM.
    #
    # Simpler: just verify that PC after step = original_pc + 1 regardless
    # of whether the RST handler runs or not.

    # 0x4600: RST 0x38   (FF)  → jumps to 0x0038 (ROM area, has a RET there or loops)
    # 0x4601: NOP         ← expected PC after step-over
    write_mem(sock, reader, 0x4600, [0xFF])  # RST 0x38
    write_mem(sock, reader, 0x4601, [0x00])

    halt(sock, reader)
    set_pc(sock, reader, 0x4600)
    set_regs(sock, reader, sp=0x7F00)

    pc = step(sock, reader)
    failures += 0 if check("RST 0x38 step-over → PC=0x4601", pc, 0x4601) else 1

    return failures


def test_ldir_stepover(sock, reader):
    """Step over LDIR with BC != 0: must land at PC+2, not loop."""
    print("\n=== Step over LDIR (BC=8) ===")
    failures = 0

    # 0x4700: LD BC,8       (01 08 00)
    # 0x4703: LD HL,0x4800  (21 00 48)  source
    # 0x4706: LD DE,0x4900  (11 00 49)  dest
    # 0x4709: LDIR          (ED B0)
    # 0x470B: NOP            ← landing
    write_mem(sock, reader, 0x4700, [0x01, 0x08, 0x00])  # LD BC,8
    write_mem(sock, reader, 0x4703, [0x21, 0x00, 0x48])  # LD HL,0x4800
    write_mem(sock, reader, 0x4706, [0x11, 0x00, 0x49])  # LD DE,0x4900
    write_mem(sock, reader, 0x4709, [0xED, 0xB0])        # LDIR
    write_mem(sock, reader, 0x470B, [0x00])               # NOP
    write_mem(sock, reader, 0x4800, [0xAA] * 8)           # source data

    halt(sock, reader)
    set_pc(sock, reader, 0x4700)
    step(sock, reader)  # LD BC,8
    step(sock, reader)  # LD HL,0x4800
    step(sock, reader)  # LD DE,0x4900

    pc = get_pc(sock, reader)
    failures += 0 if check("Before LDIR → PC=0x4709", pc, 0x4709) else 1

    pc = step(sock, reader)
    failures += 0 if check("LDIR step-over → PC=0x470B", pc, 0x470B) else 1

    return failures


def step_in(sock, reader):
    """Send stepIn command, wait for emulator to stop, return new PC."""
    send(sock, reader, {"cmd": "stepIn"})
    return wait_stopped(sock, reader)


def step_out(sock, reader):
    """Send stepOut command, wait for emulator to stop, return new PC."""
    send(sock, reader, {"cmd": "stepOut"})
    return wait_stopped(sock, reader)


def set_breakpoints(sock, reader, addresses):
    bps = [{"type": "MemoryRead", "address": a} for a in addresses]
    return send(sock, reader, {"cmd": "setBreakpoints", "breakpoints": bps})


# ---------------------------------------------------------------------------

def test_step_in(sock, reader):
    """stepIn on CALL nn: PC must enter the subroutine (not skip it)."""
    print("\n=== stepIn (CALL) ===")
    failures = 0

    # 0x5100: CALL 0x5200  (CD 00 52) → stepIn should land at 0x5200
    # 0x5103: NOP           ← step-over would land here instead
    # 0x5200: NOP + RET
    write_mem(sock, reader, 0x5100, [0xCD, 0x00, 0x52])
    write_mem(sock, reader, 0x5103, [0x00])
    write_mem(sock, reader, 0x5200, [0x00, 0xC9])

    halt(sock, reader)
    set_pc(sock, reader, 0x5100)
    set_regs(sock, reader, sp=0x7F00)

    pc = step_in(sock, reader)
    failures += 0 if check("stepIn CALL 0x5200 → PC=0x5200", pc, 0x5200) else 1

    return failures


def test_step_out(sock, reader):
    """stepOut from inside a subroutine: PC must land at the instruction after CALL."""
    print("\n=== stepOut ===")
    failures = 0

    # 0x5400: CALL 0x5500  (CD 00 55)
    # 0x5403: NOP            ← expected landing after stepOut
    # 0x5500: NOP            ← subroutine entry; stepOut from here
    # 0x5501: NOP
    # 0x5502: RET
    write_mem(sock, reader, 0x5400, [0xCD, 0x00, 0x55])  # CALL 0x5500
    write_mem(sock, reader, 0x5403, [0x00])               # NOP
    write_mem(sock, reader, 0x5500, [0x00, 0x00, 0xC9])   # NOP, NOP, RET

    halt(sock, reader)
    set_pc(sock, reader, 0x5400)
    set_regs(sock, reader, sp=0x7F00)

    # Enter the subroutine via stepIn
    pc = step_in(sock, reader)
    failures += 0 if check("stepIn CALL 0x5500 → PC=0x5500", pc, 0x5500) else 1

    # Step out: should return to 0x5403 (after the CALL)
    pc = step_out(sock, reader)
    failures += 0 if check("stepOut → PC=0x5403", pc, 0x5403) else 1

    return failures


def test_set_breakpoints(sock, reader):
    """Set a breakpoint then continue: execution must stop at the breakpoint."""
    print("\n=== setBreakpoints + continue ===")
    failures = 0

    # 10 NOPs at 0x5300; breakpoint at 0x5305
    BASE = 0x5300
    BP   = BASE + 5
    write_mem(sock, reader, BASE, [0x00] * 10)

    halt(sock, reader)
    set_pc(sock, reader, BASE)
    set_breakpoints(sock, reader, [BP])

    send(sock, reader, {"cmd": "continue"})
    wait_running(sock, reader)  # wait until emulator is actually running
    pc = wait_stopped(sock, reader, timeout=3.0)
    failures += 0 if check("Breakpoint at 0x5305 → PC=0x5305", pc, BP) else 1

    # Clear breakpoints for subsequent tests
    set_breakpoints(sock, reader, [])

    return failures


def read_memory(sock, reader, address, size):
    return send(sock, reader, {"cmd": "readMemory", "address": address, "size": size})["bytes"]


def test_read_memory(sock, reader):
    """readMemory: write known bytes then read back; also tests size > 256."""
    print("\n=== readMemory ===")
    failures = 0

    BASE = 0x7000
    data = [0x11, 0x22, 0x33, 0x44, 0xAA, 0xBB, 0xCC, 0xDD]
    write_mem(sock, reader, BASE, data)

    result = read_memory(sock, reader, BASE, len(data))
    ok = result == data
    print(f"  [{'OK' if ok else 'FAIL'}] readMemory 8 bytes  got={[hex(b) for b in result]}")
    failures += 0 if ok else 1

    # Large read > 256 bytes (tests extended buffer)
    result512 = read_memory(sock, reader, 0x4000, 512)
    ok = len(result512) == 512
    print(f"  [{'OK' if ok else 'FAIL'}] readMemory 512 bytes  count={len(result512)}  expected=512")
    failures += 0 if ok else 1

    return failures


def test_set_registers_primed(sock, reader):
    """setRegisters: write primed registers and verify via readRegisters."""
    print("\n=== setRegisters (primed registers) ===")
    failures = 0

    halt(sock, reader)
    send(sock, reader, {"cmd": "setRegisters", "af'": 0x1234, "bc'": 0x5678, "de'": 0x9ABC, "hl'": 0xDEF0})

    regs = send(sock, reader, {"cmd": "readRegisters"})
    for name, expected in [("AF'", 0x1234), ("BC'", 0x5678), ("DE'", 0x9ABC), ("HL'", 0xDEF0)]:
        got = regs.get(name, -1)
        ok = got == expected
        print(f"  [{'OK' if ok else 'FAIL'}] {name} = 0x{got:04x}   expected=0x{expected:04x}")
        failures += 0 if ok else 1

    return failures


def test_async_events(sock, reader):
    """Verify async stop events carry the correct reason field."""
    print("\n=== async stop events ===")
    failures = 0

    BASE = 0x6000
    write_mem(sock, reader, BASE, [0x00] * 16)  # 16 NOPs
    halt(sock, reader)
    set_pc(sock, reader, BASE)

    # stepIn → reason must be "step"
    send(sock, reader, {"cmd": "stepIn"})
    try:
        evt = recv_event(sock, reader)
        reason = evt.get("body", {}).get("reason", "")
        ok = reason == "step"
        print(f"  [{'OK' if ok else 'FAIL'}] stepIn event reason='{reason}'  expected='step'")
        failures += 0 if ok else 1
    except TimeoutError as e:
        print(f"  [FAIL] stepIn event: {e}")
        failures += 1

    # step-over → reason must be "step"
    send(sock, reader, {"cmd": "step"})
    try:
        evt = recv_event(sock, reader)
        reason = evt.get("body", {}).get("reason", "")
        ok = reason == "step"
        print(f"  [{'OK' if ok else 'FAIL'}] step-over event reason='{reason}'  expected='step'")
        failures += 0 if ok else 1
    except TimeoutError as e:
        print(f"  [FAIL] step-over event: {e}")
        failures += 1

    # continue + breakpoint → reason must be "instruction breakpoint"
    BP = BASE + 4
    set_breakpoints(sock, reader, [BP])
    halt(sock, reader)
    set_pc(sock, reader, BASE)
    send(sock, reader, {"cmd": "continue"})  # reads {"status":"running"}
    try:
        evt = recv_event(sock, reader, timeout=3.0)
        reason = evt.get("body", {}).get("reason", "")
        ok = reason == "instruction breakpoint"
        print(f"  [{'OK' if ok else 'FAIL'}] breakpoint event reason='{reason}'  expected='instruction breakpoint'")
        failures += 0 if ok else 1
        pc = get_pc(sock, reader)
        failures += 0 if check("PC at breakpoint → 0x6004", pc, BP) else 1
    except TimeoutError as e:
        print(f"  [FAIL] breakpoint event: {e}")
        failures += 1

    set_breakpoints(sock, reader, [])
    return failures


def test_restart(sock, reader):
    """Reset: PC must return to 0x0000."""
    print("\n=== restart (reset) ===")
    failures = 0

    set_pc(sock, reader, 0x4000)
    send(sock, reader, {"cmd": "reset"})
    pc = wait_stopped(sock, reader)
    failures += 0 if check("After reset → PC=0x0000", pc, 0x0000) else 1

    return failures


def test_disconnect_reconnect(host, port):
    """After main connection closes, reconnect and verify server is responsive."""
    print("\n=== disconnect + reconnect ===")
    failures = 0

    # Main sock was closed by caller — give server time to loop back to accept()
    time.sleep(0.5)

    try:
        s, r = connect(host, port, timeout=10)
        state = send(s, r, {"cmd": "getState"})
        print(f"  [OK ] reconnect → server responsive (PC=0x{state.get('pc', 0):04x})")
        r.close()
        s.close()
    except Exception as e:
        print(f"  [FAIL] reconnect → {e}")
        failures += 1

    return failures


# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=1234)
    args = parser.parse_args()

    print(f"Connecting to {args.host}:{args.port} ...")
    try:
        sock, reader = connect(args.host, args.port)
    except Exception as e:
        print(f"ERROR: {e}")
        sys.exit(1)
    print("Connected.\n")

    failures = 0
    failures += test_nop_steps(sock, reader)
    failures += test_call_stepover(sock, reader)
    failures += test_conditional_call_stepover(sock, reader)
    failures += test_djnz_stepover(sock, reader)
    failures += test_rst_stepover(sock, reader)
    failures += test_ldir_stepover(sock, reader)
    failures += test_step_in(sock, reader)
    failures += test_step_out(sock, reader)
    failures += test_set_breakpoints(sock, reader)
    failures += test_read_memory(sock, reader)
    failures += test_set_registers_primed(sock, reader)
    failures += test_async_events(sock, reader)
    failures += test_restart(sock, reader)
    reader.close()  # must close reader before sock (makefile increments _io_refs)
    sock.close()

    failures += test_disconnect_reconnect(args.host, args.port)

    print(f"\n{'='*55}")
    if failures == 0:
        print("All tests passed.")
    else:
        print(f"{failures} test(s) FAILED.")
        sys.exit(1)


if __name__ == "__main__":
    main()
