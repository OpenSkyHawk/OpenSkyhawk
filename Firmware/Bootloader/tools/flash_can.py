#!/usr/bin/env python3
"""Bench host for the CAN bootloader (#363) — sends an app image to a board's bootloader, either
straight onto the bus through a CANable (slcan, python-can) or through the Gateway's USB port and the
PanelBridge relay (--gateway, #365), before the Gateway uploader (#366) exists.

  python3 flash_can.py PORT firmware.bin --node N          # flash node N (waits for its bootloader)
  python3 flash_can.py PORT --listen                       # print bootloader announcements
  python3 flash_can.py PORT firmware.bin --node N --stop-after 40   # stop mid-image, as if power went
  python3 flash_can.py PORT firmware.bin --delay-ms 100   # stretch the write for a power-pull test
  python3 flash_can.py PORT firmware.bin --unconfigured   # a board with no NODE_ID yet (one on the bus)
  python3 flash_can.py PORT --node N --set-node-id M      # move node N to NODE_ID M (invalidates its app)
  python3 flash_can.py --gateway GW_PORT firmware.bin     # through the Gateway + bridge relay instead

The image is a production firmware.bin (app at 0x08001000, identity block stamped by
Firmware/shared/osh_image.py). Its identity block gives the length BEGIN announces and the CRC END
checks. Frame IDs and codes are read from Firmware/Libraries/BootProtocol/BootProtocol.h, so this
script can't drift from the bootloader. Protocol: Firmware/ScratchPad/FirmwarePlan/12-bootloader.md.

A board enters its bootloader when it has no valid app, on request (#365), when its app keeps
resetting, or when a CONNECT arrives in the 200 ms after a reset — this script sends CONNECT every
100 ms while it waits, so resetting the board catches it. On the bench without #365, request it over
ST-Link instead: write 0xB007 to backup register DR1 (0x40006C04) and reset.
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
BOOT_RSP = re.search(r'#define\s+BOOT_RSP_MSG_NAME\s+"([^"]+)"', HEADER.read_text())[1]
BLK, DATA_BYTES, DATA_FRAMES = P["BLOCK_SIZE"], P["DATA_BYTES"], P["DATA_FRAMES"]
STATES = {P["REQUESTED"]: "requested", P["APP_INVALID"]: "app invalid", P["UNCONFIGURED"]: "unconfigured",
          P["APP_UNSTABLE"]: "app unstable", P["CAUGHT"]: "caught at boot"}
STATUS = {v: k for k, v in P.items() if k in ("OK", "BAD_CRC", "FLASH_ERROR", "OUT_OF_RANGE", "OUT_OF_ORDER", "NO_APP",
                                               "BAD_IMAGE", "WRONG_NODE", "WRONG_KIND")}


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
    uid = f" · uid {d[6] | (d[7] << 8):04X}" if len(d) >= 8 else ""
    return (f"bootloader v{d[1]} · node {d[2] if d[2] != 0xFF else '—'} · kind {d[3] if d[3] != 0xFF else '—'} · "
            f"{STATES.get(d[4], hex(d[4]))} · flash {d[5]} KB{uid}")


class Bus:
    def __init__(self, port):
        import can
        self.can = can
        self.bus = can.Bus(interface="slcan", channel=port, bitrate=500000)

    def send(self, can_id, data):
        self.bus.send(self.can.Message(arbitration_id=can_id, data=bytes(data), is_extended_id=False))

    def send_many(self, frames):
        for can_id, data in frames:
            self.send(can_id, data)

    def recv(self, timeout):
        m = self.bus.recv(max(timeout, 0))
        return (m.arbitration_id, bytes(m.data)) if m else None

    def close(self):
        self.bus.shutdown()


def relay_frame(can_id, data):
    """One CAN frame in the relay's stream encoding (BootProtocol.h): mark|len, id, data, 0x55/0x56 escaped."""
    out = bytearray([P["RELAY_FRAME_MARK"] | len(data)])
    for b in bytes([can_id & 0xFF, can_id >> 8]) + bytes(data):
        if b == 0x55:
            out += bytes([P["RELAY_ESCAPE"], P["RELAY_ESC_55"]])
        elif b == 0x56:
            out += bytes([P["RELAY_ESCAPE"], P["RELAY_ESC_56"]])
        else:
            out.append(b)
    return bytes(out)


class GatewayBus:
    """The same send / recv through the Gateway's USB port (#365, #366): frames go down packed into
    DCS-BIOS export records at RELAY_EXPORT_ADDRESS (whole frames per record, stream-encoded), a
    bootloader frame comes back as a `_BOOT_RSP <id><len><data>` line. Close the client first — it
    owns the port."""
    def __init__(self, port):
        import serial
        self.s = serial.Serial(port, 250000, timeout=0.02)
        self.buf = b""

    def send(self, can_id, data):
        self.send_many([(can_id, data)])

    def send_many(self, frames):
        cap = 2 * P["RELAY_EXPORT_WORDS"]
        out, rec = bytearray(b"\x55\x55\x55\x55"), bytearray()   # sync first: parse from a clean state

        def flush():
            if rec:
                body = rec + b"\x00" * (len(rec) % 2)   # whole words; 0x00 is padding
                out.extend(struct.pack("<HH", P["RELAY_EXPORT_ADDRESS"], len(body)) + body)
                rec.clear()
        for can_id, data in frames:
            f = relay_frame(can_id, data)
            if len(rec) + len(f) > cap:
                flush()                                    # a record starts on a frame boundary
            rec.extend(f)
        flush()
        self.s.write(bytes(out))

    def recv(self, timeout):
        end = time.monotonic() + max(timeout, 0)
        while True:
            while b"\n" in self.buf:
                line, self.buf = self.buf.split(b"\n", 1)
                text = line.decode(errors="replace").strip()
                if text.startswith(BOOT_RSP + " "):
                    h = text.split(" ", 1)[1]
                    try:
                        n = int(h[3], 16)
                        return int(h[:3], 16), bytes.fromhex(h[4:4 + 2 * n])
                    except (ValueError, IndexError):
                        continue
            if time.monotonic() >= end:
                return None
            self.buf += self.s.read(256)

    def close(self):
        self.s.close()


def listen(bus):
    rsp = range(P["RSP_ID_BASE"], P["RSP_ID_BASE"] + 64)
    print("listening for bootloader announcements (Ctrl-C to stop) …")
    while True:
        r = bus.recv(1.0)
        if r and (r[0] in rsp or r[0] == P["UNCONFIGURED_RSP_ID"]) and r[1] and r[1][0] == P["ANNOUNCE"]:
            print(f"{time.strftime('%H:%M:%S')}  0x{r[0]:03X}  {describe(r[1])}")


def unconfigured_uid(bus):
    """The UID16 of the (one) unconfigured bootloader on the bus, from its announcement."""
    print("waiting for an unconfigured bootloader's announcement …")
    while True:
        r = bus.recv(1.0)
        if r and r[0] == P["UNCONFIGURED_RSP_ID"] and len(r[1]) >= 8 and r[1][0] == P["ANNOUNCE"]:
            return r[1][6] | (r[1][7] << 8)


def connect(bus, node, uid=None):
    """Open a session: (ask, info). node=None → the unconfigured board named by uid."""
    if node is None:
        cmd, rsp, hello = P["UNCONFIGURED_CMD_ID"], P["UNCONFIGURED_RSP_ID"], [P["CONNECT"], uid & 0xFF, uid >> 8]
    else:
        cmd, rsp, hello = P["CMD_ID_BASE"] + node, P["RSP_ID_BASE"] + node, [P["CONNECT"]]

    def ask(data, want, timeout=0.5, tries=3):
        for _ in range(tries):
            bus.send(cmd, data)
            end = time.monotonic() + timeout
            while time.monotonic() < end:
                r = bus.recv(end - time.monotonic())
                if r and r[0] == rsp and r[1] and r[1][0] == want:
                    return r[1]
        return None

    print(f"waiting for {'the unconfigured board' if node is None else f'node {node}'}'s bootloader …")
    info = None
    while not info:              # every 100 ms — also catches a board in its 200 ms window after a reset
        info = ask(hello, P["CONNECTED"], timeout=0.1, tries=1)
    print(describe(info))
    return ask, cmd, rsp


def set_node_id(bus, node, new_id):
    ask, _, _ = connect(bus, node)
    r = ask([P["SET_NODE_ID"], new_id], P["NODE_SET"], timeout=1.0)
    if not r or r[1]:
        sys.exit(f"SET_NODE_ID refused: {STATUS.get(r[1], r[1]) if r else 'no reply'}")
    print(f"node {node} is now node {new_id} — its app is invalid until an image for node {new_id} is flashed")


def flash(bus, img, node, stop_after, no_go, delay_ms=0, uid=None, window=None):
    length, image_crc, img_node, img_kind, version = identity(img)
    print(f"image: node {img_node} · kind {img_kind} · v{version} · {length} B · CRC {image_crc:08X}")
    img = img[:length]
    padded = img + b"\xFF" * (-len(img) % BLK)
    ask, cmd, rsp = connect(bus, node, uid)

    t0 = time.monotonic()
    # BEGIN erases every page the image needs before it answers (#366): ~25 ms per KB.
    r = ask([P["BEGIN"], *struct.pack("<I", length)], P["BEGUN"], timeout=6.0, tries=1)
    if not r or r[1]:
        sys.exit(f"BEGIN refused: {STATUS.get(r[1], r[1]) if r else 'no reply'}")
    t_erase = time.monotonic() - t0
    blocks = len(padded) // BLK
    limit = blocks if stop_after is None else min(blocks, stop_after)

    def block_frames(b):
        chunk = padded[b * BLK:(b + 1) * BLK]
        return [(cmd, [P["BLOCK"], *struct.pack("<HH", b, crc16(chunk))])] + \
               [(cmd, [P["DATA_0"] + k, *chunk[k * DATA_BYTES:(k + 1) * DATA_BYTES]]) for k in range(DATA_FRAMES)]

    # Windowed (#366): up to WINDOW_BLOCKS ahead of the board's `next`; a loss is resent from `next`.
    window = 1 if delay_ms else window or P["WINDOW_BLOCKS"]
    acked, sent, goback, retries, stalls = 0, 0, -1, 0, 0
    deadline = time.monotonic() + 0.5
    while acked < limit:
        while sent < limit and sent - acked < window:
            bus.send_many(block_frames(sent))
            sent += 1
        x = bus.recv(max(0.0, deadline - time.monotonic()))
        if time.monotonic() >= deadline:                # no ack for 0.5 s (other traffic doesn't count)
            stalls += 1
            if stalls > 6:
                sys.exit(f"\nno ack — the board stopped answering at block {acked}")
            sent, retries = acked, retries + 1          # resend from the first unacknowledged block
            deadline = time.monotonic() + 0.5
            continue
        if not x or x[0] != rsp or not x[1] or x[1][0] != P["BLOCK_ACK"] or len(x[1]) < 6:
            continue
        st, nxt = x[1][3], x[1][4] | (x[1][5] << 8)
        if nxt < acked:                                 # the board reset and lost the session
            sys.exit(f"\nthe board lost the session at block {acked} (it restarted) — flash it again")
        stalls, deadline = 0, time.monotonic() + 0.5
        if st == P["OK"]:
            acked = max(acked, nxt)
        elif st in (P["OUT_OF_ORDER"], P["BAD_CRC"]):
            if nxt != goback:                           # one go-back per loss, not one per stale ack
                goback, sent, retries = nxt, nxt, retries + 1
            acked = max(acked, nxt)
        else:
            sys.exit(f"\nblock {x[1][1] | (x[1][2] << 8)} failed: {STATUS.get(st, st)}")
        if delay_ms:
            time.sleep(delay_ms / 1000)
        if acked % 32 == 0 or delay_ms:
            print(f"  block {acked}/{blocks}", end="\r", flush=True)
    if stop_after is not None and limit < blocks:
        print(f"\nstopped after {limit} blocks — the board must stay in its bootloader, flashable")
        return
    r = ask([P["END"], *struct.pack("<I", image_crc)], P["ENDED"], timeout=2.0)
    dt = time.monotonic() - t0
    if not r or r[1]:
        sys.exit(f"\nEND refused: {STATUS.get(r[1], r[1]) if r else 'no reply'}" + (f" · board CRC {r[2:6][::-1].hex()}" if r else ""))
    print(f"written {length} B in {dt:.2f} s = {length / dt / 1024:.1f} KB/s (erase {t_erase:.2f} s) · {retries} resends")
    if not no_go:
        r = ask([P["GO"]], P["GOING"], timeout=0.5)
        print("GO — the board resets into the new app" if r and r[1] == P["OK"] else f"GO refused: {r and STATUS.get(r[1])}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("port", nargs="?", help="the CANable's serial port")
    ap.add_argument("--gateway", metavar="GW_PORT", help="go through the Gateway's USB port and the bridge relay instead")
    ap.add_argument("image", nargs="?", help="production firmware.bin")
    ap.add_argument("--node", type=int, help="target NODE_ID (default: the image's)")
    ap.add_argument("--listen", action="store_true", help="print bootloader announcements")
    ap.add_argument("--stop-after", type=int, help="stop after N blocks (simulated power loss)")
    ap.add_argument("--no-go", action="store_true", help="don't start the app after a good END")
    ap.add_argument("--delay-ms", type=int, default=0, help="pause after each block — stretches the write for a power-pull test")
    ap.add_argument("--window", type=int, help=f"blocks in flight (default {P['WINDOW_BLOCKS']}; 1 = stop-and-wait)")
    ap.add_argument("--unconfigured", action="store_true", help="flash the one unconfigured board on the bus (no NODE_ID yet)")
    ap.add_argument("--set-node-id", type=int, metavar="M", help="move --node N to NODE_ID M (invalidates its app)")
    a = ap.parse_args()
    if a.gateway:
        if a.port and not a.image:      # one positional given: it is the image
            a.image, a.port = a.port, None
        bus = GatewayBus(a.gateway)
    elif a.port:
        bus = Bus(a.port)
    else:
        ap.error("a CANable port, or --gateway GW_PORT")
    try:
        if a.listen:
            listen(bus)
        elif a.set_node_id is not None:
            if a.node is None:
                ap.error("--set-node-id needs --node")
            set_node_id(bus, a.node, a.set_node_id)
        elif not a.image:
            ap.error("an image is needed unless --listen or --set-node-id")
        else:
            img = pathlib.Path(a.image).read_bytes()
            if a.unconfigured:
                flash(bus, img, None, a.stop_after, a.no_go, a.delay_ms, unconfigured_uid(bus), a.window)
            else:
                flash(bus, img, a.node if a.node is not None else identity(img)[2], a.stop_after, a.no_go, a.delay_ms,
                      window=a.window)
    except KeyboardInterrupt:
        pass  # Ctrl-C is how --listen (or a stuck wait) ends — close the bus quietly, no traceback
    finally:
        bus.close()


if __name__ == "__main__":
    main()
