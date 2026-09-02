#!/usr/bin/env python3
"""
Test script for DebugServer evaluate command.

Usage:
    1. Launch Sugarbox and wait for it to pause (breakpoint or halt)
    2. Run: python3 test_evaluate.py [--host 127.0.0.1] [--port 1234]
"""

import socket
import json
import argparse
import sys


def connect(host, port):
    s = socket.create_connection((host, port), timeout=5)
    return s, s.makefile("r")


def send(sock, reader, cmd):
    sock.sendall((json.dumps(cmd) + "\n").encode())
    line = reader.readline()
    if not line:
        raise EOFError("Connection closed by server")
    return json.loads(line)


def check(label, result, expected_key="text"):
    val = result.get(expected_key)
    ok = val is not None and val != "?"
    status = "OK" if ok else "FAIL"
    print(f"  [{status}] {label:30s} → {val}")
    return ok


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

    def evaluate(expr):
        return send(sock, reader, {"cmd": "evaluate", "expression": expr})

    # --- 16-bit registers ---
    print("=== 16-bit registers ===")
    for reg in ["AF", "BC", "DE", "HL", "IX", "IY", "SP", "PC"]:
        failures += 0 if check(reg, evaluate(reg)) else 1

    # --- Alternate registers ---
    print("\n=== Alternate registers ===")
    for reg in ["AF'", "BC'", "DE'", "HL'"]:
        failures += 0 if check(reg, evaluate(reg)) else 1

    # --- 8-bit registers ---
    print("\n=== 8-bit registers ===")
    for reg in ["A", "F", "B", "C", "D", "E", "H", "L"]:
        failures += 0 if check(reg, evaluate(reg)) else 1

    # --- Memory: default mapping (MEM_READ) ---
    print("\n=== Memory: default mapping ===")
    for addr in ["0x0000", "0x4000", "0xC000"]:
        failures += 0 if check(addr, evaluate(addr)) else 1

    # --- Memory: explicit modes ---
    print("\n=== Memory: explicit modes ===")
    cases = [
        ("read:0x0000",    "MEM_READ explicit"),
        ("write:0x4000",   "MEM_WRITE"),
        ("ram:0x0000",     "MEM_RAM_LOWER_BANK"),
        ("ram[0]:0x0000",  "MEM_RAM_BANK[0]"),
        ("rom:0x0000",     "MEM_LOWER_ROM"),
        ("rom[0]:0x0000",  "MEM_ROM_BANK[0]"),
        ("cart[0]:0x0000", "MEM_CART_SLOT[0]"),
    ]
    for expr, label in cases:
        failures += 0 if check(f"{expr} ({label})", evaluate(expr)) else 1

    # --- Error cases: must return "?" ---
    print("\n=== Error cases (expect '?') ===")
    for expr in ["FOOBAR", "unknown:0x1234", "ram[]:0x0000"]:
        r = evaluate(expr)
        val = r.get("text")
        ok = val == "?"
        status = "OK" if ok else "FAIL"
        print(f"  [{status}] {expr:30s} → {val}")
        failures += 0 if ok else 1

    print(f"\n{'='*50}")
    if failures == 0:
        print("All tests passed.")
    else:
        print(f"{failures} test(s) FAILED.")
        sys.exit(1)


if __name__ == "__main__":
    main()
