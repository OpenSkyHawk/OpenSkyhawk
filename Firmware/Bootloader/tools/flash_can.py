#!/usr/bin/env python3
"""Bench host for the CAN bootloader (#363) — sends an app image straight onto the bus through a
CANable (slcan, python-can), before the Gateway uploader (#366) exists.

  python3 flash_can.py PORT firmware.bin --node N          # flash node N (waits for its bootloader)
  python3 flash_can.py PORT --listen                       # print bootloader announcements
  python3 flash_can.py PORT firmware.bin --node N --stop-after 40   # stop mid-image, as if power went

The image is a production firmware.bin (app at 0x08001000, identity block stamped by
Firmware/shared/osh_image.py). Its identity block gives the length BEGIN announces and the CRC END
checks. Frame IDs and codes are read from Firmware/Libraries/BootProtocol/BootProtocol.h, so this
script can't drift from the bootloader. Protocol: Firmware/ScratchPad/FirmwarePlan/12-bootloader.md.

A board enters its bootloader when it has no valid app, or on request (#365). On the bench without
#365, request it over ST-Link: write 0xB007 to backup register DR1 (0x40006C04) and reset.
"""
import argparse
import pathlib
import re
import struct
import sys
import time

HEADER = pathlib.Path(__file__).resolve().parents[2] / "Libraries" / "BootProtocol" / "BootProtocol.h"


def protocol():
    """Every `NAME = 0x..` / `NAME = n` in BootProtocol.h, by name."""
    text = HEADER.read_text()
    return {m[1]: int(m[2], 0) for m in re.finditer(r"\b([A-Z][A-Z0-9_]*)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\b", text)}


P = protocol()
BLK, DATA_BYTES, DATA_FRAMES = P["BLOCK_SIZE"], P["DATA_BYTES"], P["DATA_FRAMES"]
STATES = {P["REQUESTED"]: "requested", P["APP_INVALID"]: "app invalid", P["UNCONFIGURED"]: "unconfigured"}
STATUS = {v: k for k, v in P.items() if k in ("OK", "BAD_CRC", "FLASH_ERROR", "OUT_OF_RANGE", "OUT_OF_ORDER", "NO_APP", "BAD_IMAGE")}


def crc16(d):  # CRC-16/CCITT-FALSE — the block check
    c = 0xFFFF
    for b in d:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c


def identity(img):
    """(imageLength, imageCrc, nodeId, nodeKind, version) from the stamped identity block at +0x200."""
    magic, _, _, length, crc, node, kind, ma, mi, pa = struct.unpack_from("<IHHIIBBBBB", img, 0x200)
    if magic != 0x594B534F or length == 0xFFFFFFFF:
        sys.exit("no stamped identity block at +0x200 — not a production image")
    return length, crc, node, kind, f"{ma}.{mi}.{pa}"


def describe(d):
    return (f"bootloader v{d[1]} · node {d[2] if d[2] != 0xFF else '—'} · kind {d[3] if d[3] != 0xFF else '—'} · "
            f"{STATES.get(d[4], hex(d[4]))} · flash {d[5]} KB")


class Bus:
    def __init__(self, port):
        import can
        self.can = can
        self.bus = can.Bus(interface="slcan", channel=port, bitrate=500000)

    def send(self, can_id, data):
        self.bus.send(self.can.Message(arbitration_id=can_id, data=bytes(data), is_extended_id=False))

    def recv(self, timeout):
        m = self.bus.recv(max(timeout, 0))
        return (m.arbitration_id, bytes(m.data)) if m else None

    def close(self):
        self.bus.shutdown()


def listen(bus):
    rsp = range(P["RSP_ID_BASE"], P["RSP_ID_BASE"] + 64)
    print("listening for bootloader announcements (Ctrl-C to stop) …")
    while True:
        r = bus.recv(1.0)
        if r and (r[0] in rsp or r[0] == P["UNCONFIGURED_RSP_ID"]) and r[1] and r[1][0] == P["ANNOUNCE"]:
            print(f"{time.strftime('%H:%M:%S')}  0x{r[0]:03X}  {describe(r[1])}")


def flash(bus, img, node, stop_after, no_go):
    length, image_crc, img_node, img_kind, version = identity(img)
    print(f"image: node {img_node} · kind {img_kind} · v{version} · {length} B · CRC {image_crc:08X}")
    img = img[:length]
    padded = img + b"\xFF" * (-len(img) % BLK)
    cmd, rsp = P["CMD_ID_BASE"] + node, P["RSP_ID_BASE"] + node

    def ask(data, want, timeout=0.5, tries=3):
        for _ in range(tries):
            bus.send(cmd, data)
            end = time.monotonic() + timeout
            while time.monotonic() < end:
                r = bus.recv(end - time.monotonic())
                if r and r[0] == rsp and r[1] and r[1][0] == want:
                    return r[1]
        return None

    print(f"waiting for node {node}'s bootloader …")
    info = None
    while not info:
        info = ask([P["CONNECT"]], P["CONNECTED"], timeout=0.1, tries=1)
    print(describe(info))

    t0 = time.monotonic()
    r = ask([P["BEGIN"], *struct.pack("<I", length)], P["BEGUN"], timeout=1.0)
    if not r or r[1]:
        sys.exit(f"BEGIN refused: {STATUS.get(r[1], r[1]) if r else 'no reply'}")
    blocks = len(padded) // BLK
    retries = 0
    for b in range(blocks):
        if stop_after is not None and b >= stop_after:
            print(f"\nstopped after {b} blocks — the board must stay in its bootloader, flashable")
            return
        chunk = padded[b * BLK:(b + 1) * BLK]
        for _ in range(3):
            bus.send(cmd, [P["BLOCK"], *struct.pack("<HH", b, crc16(chunk))])
            for k in range(DATA_FRAMES):
                bus.send(cmd, [P["DATA_0"] + k, *chunk[k * DATA_BYTES:(k + 1) * DATA_BYTES]])
            r = None
            end = time.monotonic() + 0.3
            while time.monotonic() < end:
                x = bus.recv(end - time.monotonic())
                if x and x[0] == rsp and x[1] and x[1][0] == P["BLOCK_ACK"] and (x[1][1] | (x[1][2] << 8)) == b:
                    r = x[1]
                    break
            if r and r[3] == P["OK"]:
                break
            retries += 1
        else:
            sys.exit(f"\nblock {b} failed: {STATUS.get(r[3], r[3]) if r else 'no ack'}")
        if b % 32 == 0:
            print(f"  block {b}/{blocks}", end="\r", flush=True)
    r = ask([P["END"], *struct.pack("<I", image_crc)], P["ENDED"], timeout=2.0)
    dt = time.monotonic() - t0
    if not r or r[1]:
        sys.exit(f"\nEND refused: {STATUS.get(r[1], r[1]) if r else 'no reply'}" + (f" · board CRC {r[2:6][::-1].hex()}" if r else ""))
    print(f"written {length} B in {dt:.2f} s = {length / dt / 1024:.1f} KB/s · {retries} block retries")
    if not no_go:
        r = ask([P["GO"]], P["GOING"], timeout=0.5)
        print("GO — the board resets into the new app" if r and r[1] == P["OK"] else f"GO refused: {r and STATUS.get(r[1])}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("port", help="the CANable's serial port")
    ap.add_argument("image", nargs="?", help="production firmware.bin")
    ap.add_argument("--node", type=int, help="target NODE_ID (default: the image's)")
    ap.add_argument("--listen", action="store_true", help="print bootloader announcements")
    ap.add_argument("--stop-after", type=int, help="stop after N blocks (simulated power loss)")
    ap.add_argument("--no-go", action="store_true", help="don't start the app after a good END")
    a = ap.parse_args()
    bus = Bus(a.port)
    try:
        if a.listen:
            listen(bus)
        elif not a.image:
            ap.error("an image is needed unless --listen")
        else:
            img = pathlib.Path(a.image).read_bytes()
            flash(bus, img, a.node if a.node is not None else identity(img)[2], a.stop_after, a.no_go)
    except KeyboardInterrupt:
        pass
    finally:
        bus.close()


if __name__ == "__main__":
    main()
