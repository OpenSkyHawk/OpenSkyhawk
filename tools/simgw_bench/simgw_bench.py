#!/usr/bin/env python3
"""SimGateway link-health bench helper (#355).

Two subcommands, both talking to the Gateway's CDC port ("A-4E Skyhawk DCS-BIOS"):

  diag PORT            Send GET_DIAG and print the decoded DIAG reply.
  stall PORT [-s N]    Hold the port open with DTR asserted, keep asking PanelBridge for the
                       node roster (host-bound traffic), and never read for N seconds. Then
                       drain, check the device still answers, and print DIAG.

The DIAG field layout and the message types are read from Firmware/Libraries/SimGateway/
AxisCal.h at run time, so this script never restates a wire constant.

Close SkyHawkClient first: only one program can hold the port.
Requires pyserial (`pip install pyserial`).
"""

import argparse
import re
import sys
import time
from pathlib import Path

import serial

REPO = Path(__file__).resolve().parents[2]
AXISCAL_H = REPO / "Firmware/Libraries/SimGateway/AxisCal.h"
NODESTATUS_H = REPO / "Firmware/Libraries/NodeStatus/NodeStatus.h"


def _enum(src: str, name: str) -> dict:
    """Parse `enum <name> : uint8_t { A = 1, ... }` into {A: (value, comment)}."""
    body = re.search(r"enum\s+" + name + r"\s*:\s*uint8_t\s*\{(.*?)\};", src, re.S)
    if not body:
        sys.exit(f"{name} not found in {AXISCAL_H}")
    out = {}
    for m in re.finditer(r"(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*,?\s*(?://\s*(.*))?", body.group(1)):
        out[m.group(1)] = (int(m.group(2), 0), (m.group(3) or "").strip())
    return out


def _const(src: str, name: str) -> int:
    m = re.search(r"\b" + name + r"\s*=\s*(0x[0-9A-Fa-f]+|\d+)", src)
    if not m:
        sys.exit(f"{name} not found")
    return int(m.group(1), 0)


SRC = AXISCAL_H.read_text()
TYPES = {k: v for k, (v, _) in _enum(SRC, "CalType").items()}
DIAG_FIELDS = _enum(SRC, "CalDiagField")
DIAG_BYTES = _const(SRC, "CAL_DIAG_BYTES")
RESET_REASONS = {v: k for k, (v, _) in _enum(SRC, "CalDiagResetReason").items()}
MAGIC = bytes(int(b, 0) for b in re.search(r"CAL_FRAME_MAGIC\[4\]\s*=\s*\{([^}]*)\}", SRC).group(1).split(","))
ROSTER_ADDR = int(re.search(r"NODE_STATUS_REQ_ADDR\s+(0x[0-9A-Fa-f]+)", NODESTATUS_H.read_text()).group(1), 16)


def crc16(data: bytes) -> int:
    """CRC-16/CCITT-FALSE, matching calCrc16() in AxisCal.cpp."""
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if crc & 0x8000 else (crc << 1)
            crc &= 0xFFFF
    return crc


def build_frame(ftype: int, seq: int, payload: bytes = b"") -> bytes:
    body = bytes([ftype, seq, len(payload) & 0xFF, len(payload) >> 8]) + payload
    return MAGIC + body + crc16(body).to_bytes(2, "little")


def find_frame(buf: bytes, ftype: int, seq: int):
    """Return the payload of the first valid frame of `ftype`/`seq` in `buf`, else None."""
    i = buf.find(MAGIC)
    while i >= 0:
        if len(buf) >= i + 8:
            n = buf[i + 6] | (buf[i + 7] << 8)
            end = i + 10 + n
            if len(buf) >= end and buf[i + 4] == ftype and buf[i + 5] == seq:
                body = buf[i + 4:i + 8 + n]
                if crc16(body) == int.from_bytes(buf[end - 2:end], "little"):
                    return buf[i + 8:i + 8 + n]
        i = buf.find(MAGIC, i + 1)
    return None


def roster_request() -> bytes:
    """DCS-BIOS export frame writing 1 to NODE_STATUS_REQ_ADDR: PanelBridge replies with the roster."""
    return b"\x55\x55\x55\x55" + ROSTER_ADDR.to_bytes(2, "little") + (2).to_bytes(2, "little") + (1).to_bytes(2, "little")


def get_diag(port: serial.Serial, timeout: float = 2.0):
    seq = int(time.time()) & 0xFF
    port.reset_input_buffer()
    port.write(build_frame(TYPES["CAL_T_GET_DIAG"], seq))
    buf = b""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        buf += port.read(port.in_waiting or 1)
        pay = find_frame(buf, TYPES["CAL_T_DIAG"], seq)
        if pay is not None:
            return pay
    return None


def print_diag(pay: bytes) -> None:
    if len(pay) != DIAG_BYTES:
        print(f"DIAG payload is {len(pay)} bytes, header says {DIAG_BYTES}")
    fields = sorted(DIAG_FIELDS.items(), key=lambda kv: kv[1][0])
    for idx, (name, (off, comment)) in enumerate(fields):
        nxt = fields[idx + 1][1][0] if idx + 1 < len(fields) else DIAG_BYTES
        val = int.from_bytes(pay[off:nxt], "little")
        label = name.removeprefix("CAL_DIAG_")
        if label == "RESET_REASON":
            shown = RESET_REASONS.get(val, f"unknown ({val})").removeprefix("CAL_DIAG_RESET_")
        elif label.endswith("_MS"):
            shown = "never" if (val == 0 and label != "UPTIME_MS") else f"{val} ms ({val / 1000:.1f} s)"
        else:
            shown = str(val)
        print(f"  {label:<20} {shown:<24} {comment.split(':', 1)[-1].strip()}")


def cmd_diag(args) -> int:
    with serial.Serial(args.port, 115200, timeout=0.1) as port:
        pay = get_diag(port)
    if pay is None:
        print("No DIAG reply. Firmware older than #355, or the port is not the Gateway.")
        return 1
    print_diag(pay)
    return 0


def cmd_stall(args) -> int:
    with serial.Serial(args.port, 115200, timeout=0.1) as port:
        port.dtr = True
        print(f"Holding {args.port} with DTR, requesting the roster every 100 ms, not reading for {args.seconds} s…")
        t_end = time.monotonic() + args.seconds
        sent = 0
        while time.monotonic() < t_end:
            try:
                port.write(roster_request())
                sent += 1
            except serial.SerialTimeoutException:
                pass
            time.sleep(0.1)
        print(f"Sent {sent} roster requests. Draining…")
        drained = 0
        quiet_since = time.monotonic()
        while time.monotonic() - quiet_since < 1.0:
            chunk = port.read(4096)
            if chunk:
                drained += len(chunk)
                quiet_since = time.monotonic()
        print(f"Drained {drained} bytes buffered by the host.")
        t0 = time.monotonic()
        pay = get_diag(port)
    if pay is None:
        print("FAIL: device did not answer GET_DIAG after the stall.")
        return 1
    print(f"PASS: device answered in {(time.monotonic() - t0) * 1000:.0f} ms. DIAG:")
    print_diag(pay)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    d = sub.add_parser("diag", help="print the DIAG counters")
    d.add_argument("port", help="e.g. /dev/cu.usbmodem1101")
    d.set_defaults(func=cmd_diag)
    s = sub.add_parser("stall", help="hold DTR, stop reading, then check the device still answers")
    s.add_argument("port")
    s.add_argument("-s", "--seconds", type=int, default=60)
    s.set_defaults(func=cmd_stall)
    args = ap.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
