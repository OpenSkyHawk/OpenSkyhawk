#!/usr/bin/env python3
"""Bench client for the Gateway uploader (#366) — what SkyHawkClient will do: stream an image to the
Gateway over USB, have it flash a board over CAN, and report.

  python3 upload_gateway.py GW_PORT firmware.bin --node N            # flash node N
  python3 upload_gateway.py GW_PORT firmware.bin --unconfigured      # the one board with no NODE_ID
  python3 upload_gateway.py GW_PORT firmware.bin --node N --new-id M # move N to M first (M = the image's)
  python3 upload_gateway.py GW_PORT firmware.bin --node N --leave-after 3   # close the port 3 s into the flash
  python3 upload_gateway.py GW_PORT --info                           # UPL_INFO: state and the last result

Close SkyHawkClient first — it owns the port. Message codes and layouts are read from
Firmware/Libraries/SimGateway/{AxisCal.h,UploadProtocol.h}, so this can't drift from the firmware.
"""
import argparse
import pathlib
import re
import struct
import sys
import time

LIB = pathlib.Path(__file__).resolve().parents[2] / "Libraries" / "SimGateway"


def constants(*names):
    out = {}
    for n in names:
        text = (LIB / n).read_text()
        out.update({m[1]: int(m[2], 0) for m in re.finditer(r"\b([A-Z][A-Z0-9_]*)\s*=\s*(0x[0-9A-Fa-f]+|\d+)u?\b", text)})
    return out


C = constants("AxisCal.h", "UploadProtocol.h")
MAGIC = b"\xAA\x53\x4B\x43"
PHASES = {v: k[len("UPL_PHASE_"):] for k, v in C.items() if k.startswith("UPL_PHASE_")}
RESULTS = {v: k[len("UPL_RESULT_"):] for k, v in C.items() if k.startswith("UPL_RESULT_") and k[len("UPL_RESULT_"):] in
           ("NONE", "OK", "NO_RESPONSE", "NO_BOOTLOADER", "ID_MISMATCH", "KIND_MISMATCH", "TOO_BIG_FOR_BOARD",
            "WRITE_FAILED", "VERIFY_FAILED", "VERSION_MISMATCH", "TIMEOUT", "UNSUPPORTED")}
NACKS = {v: k[len("CAL_NACK_"):] for k, v in C.items() if k.startswith("CAL_NACK_") and k.count("_") >= 2 and k not in
         ("CAL_NACK_TYPE", "CAL_NACK_REASON", "CAL_NACK_DETAIL")}


def crc16(d):
    c = 0xFFFF
    for b in d:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c


class Gateway:
    def __init__(self, port):
        import serial
        self.s = serial.Serial(port, 250000, timeout=0.02)
        self.buf = b""
        self.seq = 0

    def send(self, typ, payload=b""):
        self.seq = (self.seq + 1) & 0xFF
        body = bytes([typ, self.seq]) + struct.pack("<H", len(payload)) + payload
        self.s.write(MAGIC + body + struct.pack("<H", crc16(body)))
        return self.seq

    def frames(self, timeout):
        """Yield (type, seq, payload) for every valid frame within timeout; text lines are skipped."""
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            self.buf += self.s.read(512)
            while True:
                i = self.buf.find(MAGIC)
                if i < 0:
                    self.buf = self.buf[-3:]
                    break
                if len(self.buf) < i + 8:
                    self.buf = self.buf[i:]
                    break
                typ, seq, n = self.buf[i + 4], self.buf[i + 5], struct.unpack_from("<H", self.buf, i + 6)[0]
                if n > 300:
                    self.buf = self.buf[i + 1:]
                    continue
                if len(self.buf) < i + 10 + n:
                    self.buf = self.buf[i:]
                    break
                body = self.buf[i + 4:i + 8 + n]
                ok = crc16(body) == struct.unpack_from("<H", self.buf, i + 8 + n)[0]
                self.buf = self.buf[i + (10 + n if ok else 1):]
                if ok:
                    yield typ, seq, self.buf[0:0] + body[4:]

    def ask(self, typ, payload=b"", want=None, timeout=1.0):
        seq = self.send(typ, payload)
        for t, s, p in self.frames(timeout):
            if s == seq and t in (want or (C["CAL_T_ACK"], C["CAL_T_NACK"])):
                return t, p
        return None, None


def nack_text(p):
    return f"NACK {NACKS.get(p[1], hex(p[1]))}" if p and len(p) >= 2 else "no reply"


def info(gw):
    t, p = gw.ask(C["CAL_T_UPL_INFO"], want=(C["CAL_T_UPL_INFO_ACK"],))
    if t is None:
        sys.exit("no UPL_INFO_ACK — this Gateway firmware has no uploader")
    state = {v: k for k, v in C.items() if k.startswith("UPL_STATE_")}.get(p[C["UPL_INFO_ACK_STATE"]])
    print(f"uploader v{p[0]} · max {p[1]} KB · chunk {struct.unpack_from('<H', p, 2)[0]} · {state} · "
          f"last: target {p[C['UPL_INFO_ACK_LAST_TARGET']]} {RESULTS.get(p[C['UPL_INFO_ACK_LAST_RESULT']])} "
          f"v{p[7]}.{p[8]}.{p[9]}")


def upload(gw, img, target, new_id, leave_after):
    magic, _, _, length, crc, node, kind, ma, mi, pa = struct.unpack_from("<IHHIIBBBBB", img, 0x200)
    if magic != 0x594B534F:
        sys.exit("no identity block at +0x200 — not a production image")
    print(f"image: node {node} · kind {kind} · v{ma}.{mi}.{pa} · {length} B · CRC {crc:08X}")
    img = img[:length]
    t0 = time.monotonic()
    t, p = gw.ask(C["CAL_T_UPL_BEGIN"], struct.pack("<II", length, crc))
    if t != C["CAL_T_ACK"]:
        sys.exit(f"UPL_BEGIN refused: {nack_text(p)}")
    chunk = C["UPL_CHUNK_BYTES"]
    for off in range(0, length, chunk):
        data = img[off:off + chunk]
        data += b"\xFF" * (chunk - len(data))
        for _ in range(3):
            t, p = gw.ask(C["CAL_T_UPL_DATA"], struct.pack("<I", off) + data, timeout=0.5)
            if t == C["CAL_T_ACK"]:
                break
        else:
            sys.exit(f"UPL_DATA at {off} refused: {nack_text(p)}")
    t, p = gw.ask(C["CAL_T_UPL_END"], timeout=2.0)
    if t != C["CAL_T_ACK"]:
        sys.exit(f"UPL_END refused: {nack_text(p)}")
    print(f"staged and verified in {time.monotonic() - t0:.2f} s")

    t1 = time.monotonic()
    t, p = gw.ask(C["CAL_T_UPL_FLASH"], bytes([target, new_id]))
    if t != C["CAL_T_ACK"]:
        sys.exit(f"UPL_FLASH refused: {nack_text(p)}")
    last_ka, last_line = 0.0, ""
    while True:
        if leave_after is not None and time.monotonic() - t1 >= leave_after:
            print(f"\nleaving {leave_after} s into the flash — the Gateway finishes on its own; --info reads the result")
            return
        if time.monotonic() - last_ka >= 1.0:
            gw.send(C["CAL_T_KEEPALIVE"])        # we're still here: keep the UPL_STATUS coming
            last_ka = time.monotonic()
        for typ, _, p in gw.frames(0.2):
            if typ == C["CAL_T_UPL_STATUS"]:
                blk, blocks = struct.unpack_from("<HH", p, C["UPL_STATUS_BLOCK"])
                line = f"  {PHASES.get(p[0], p[0]):<14} node {p[1]:<3} try {p[2] + 1}  {blk}/{blocks}"
                if PHASES.get(p[0]) != "WRITING" or blk == 0:
                    line += f"  +{time.monotonic() - t1:.2f} s"
                if line != last_line:
                    print(line, end="\r" if PHASES.get(p[0]) == "WRITING" else "\n", flush=True)
                    last_line = line
            elif typ == C["CAL_T_UPL_RESULT"]:
                secs = struct.unpack_from("<H", p, C["UPL_RESULT_SECONDS"])[0]
                dt = time.monotonic() - t1
                print(f"\nresult: {RESULTS.get(p[0], hex(p[0]))} · node {p[1]} · v{p[2]}.{p[3]}.{p[4]} · "
                      f"{p[5]} attempt(s) · {dt:.1f} s, Gateway {secs} s "
                      f"({length / dt / 1024:.1f} KB/s incl. entering + confirming)")
                return


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("port", help="the Gateway's USB serial port")
    ap.add_argument("image", nargs="?", help="production firmware.bin")
    ap.add_argument("--node", type=int, help="target NODE_ID")
    ap.add_argument("--unconfigured", action="store_true", help="the one board on the bus with no NODE_ID")
    ap.add_argument("--new-id", type=int, help="SET_NODE_ID first (must be the image's NODE_ID)")
    ap.add_argument("--leave-after", type=float, help="close the port this many seconds into the flash")
    ap.add_argument("--info", action="store_true", help="UPL_INFO only")
    a = ap.parse_args()
    gw = Gateway(a.port)
    try:
        if a.info:
            info(gw)
            return
        if not a.image or (a.node is None and not a.unconfigured):
            ap.error("an image and --node N (or --unconfigured)")
        target = C["UPL_TARGET_UNCONFIGURED"] if a.unconfigured else a.node
        upload(gw, pathlib.Path(a.image).read_bytes(), target,
               a.new_id if a.new_id is not None else C["UPL_NODE_ID_KEEP"], a.leave_after)
    finally:
        gw.s.close()


if __name__ == "__main__":
    main()
