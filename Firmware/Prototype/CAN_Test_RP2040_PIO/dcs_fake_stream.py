#!/usr/bin/env python3
"""
dcs_fake_stream.py — Fake DCS-BIOS export stream for A-4E-C bench testing.

Sends valid DCS-BIOS binary frames to the RP2040 over USB CDC, cycling through
realistic A-4E-C values. Listens for ACK and button events coming back.

Usage:
    python3 dcs_fake_stream.py              # auto-detect port
    python3 dcs_fake_stream.py /dev/ttyACM0
    python3 dcs_fake_stream.py COM5         # Windows

Press the USR button on the RP2040 to trigger a button event.
Press Ctrl+C to stop.

DCS-BIOS binary frame format:
    0x55 0x55 0x55 0x55   sync
    addr_lo addr_hi        address (uint16 LE)
    len_lo  len_hi         byte count (uint16 LE) — always 2 for integer controls
    val_lo  val_hi         value (uint16 LE)

Altitude encoding — each drum counter is 0-65535 representing 0-1 of its range:
    D_ALT_10K  0→65535 = 0→100,000 ft (ten-thousands drum)
    D_ALT_1K   0→65535 = 0→10,000 ft  (thousands drum)
    D_ALT_100S 0→65535 = 0→1,000 ft   (hundreds drum)
    Reconstructed: alt_feet = (10K/65535)*100000 + (1K/65535)*10000
"""

import sys
import math
import time
import struct
import threading
import serial
import serial.tools.list_ports

BAUD = 250_000

# ── A-4E-C DCS-BIOS addresses (mirrors Addresses.h constants) ─────────────────
# Firmware constant       Address   Mask    Shift
# A_4E_C_RPM              0x8400   0xFFFF  0
# A_4E_C_D_FLAPS_IND      0x840E   0xFFFF  0
# A_4E_C_D_RADAR_ALT      0x8428   0xFFFF  0
# A_4E_C_D_ALT_10K        0x8430   0xFFFF  0
# A_4E_C_D_ALT_1K         0x8432   0xFFFF  0
# A_4E_C_D_ALT_100S       0x8434   0xFFFF  0
# A_4E_C_ARM_MASTER        0x8500   0x4000  14
ADDR_RPM        = 0x8400
ADDR_FLAPS_IND  = 0x840E
ADDR_RADAR_ALT  = 0x8428
ADDR_ALT_10K    = 0x8430
ADDR_ALT_1K     = 0x8432
ADDR_ALT_100S   = 0x8434
ADDR_ARM_MASTER = 0x8500

# ── helpers ───────────────────────────────────────────────────────────────────
SYNC = bytes([0x55, 0x55, 0x55, 0x55])

def frame(address: int, value: int) -> bytes:
    """Build one DCS-BIOS binary frame for a single 16-bit word."""
    return SYNC + struct.pack('<HHH', address, 2, value & 0xFFFF)

def norm_to_u16(v: float) -> int:
    """Convert normalised float 0.0–1.0 to DCS-BIOS uint16 0–65535."""
    return int(max(0.0, min(1.0, v)) * 65535)

def alt_to_drums(alt_feet: float):
    """
    Convert altitude in feet to the three DCS-BIOS drum counter values.
    Each drum is a 0-65535 value representing 0-1 of its range.
      10K drum: 0-65535 = 0-100,000 ft
       1K drum: 0-65535 = 0-10,000 ft  (its position within the current 10k band)
      100s drum: 0-65535 = 0-1,000 ft  (its position within the current 1k band)
    """
    alt_feet = max(0.0, alt_feet)
    drum_10k  = norm_to_u16((alt_feet % 100000) / 100000)
    drum_1k   = norm_to_u16((alt_feet % 10000)  / 10000)
    drum_100s = norm_to_u16((alt_feet % 1000)   / 1000)
    return drum_10k, drum_1k, drum_100s

def find_port() -> str:
    """Auto-detect the RP2040 USB CDC port."""
    candidates = []
    for p in serial.tools.list_ports.comports():
        desc = (p.description or '').lower()
        if any(k in desc for k in ('pico', 'rp2040', 'cdc', 'tinyusb')):
            candidates.append(p.device)
    if len(candidates) == 1:
        print(f'[auto] Found port: {candidates[0]}')
        return candidates[0]
    if candidates:
        print('[auto] Multiple candidates:')
        for i, c in enumerate(candidates):
            print(f'  {i}: {c}')
        idx = int(input('Select: '))
        return candidates[idx]
    all_ports = [p.device for p in serial.tools.list_ports.comports()]
    print('No RP2040 port auto-detected. Available ports:')
    for i, p in enumerate(all_ports):
        print(f'  {i}: {p}')
    idx = int(input('Select: '))
    return all_ports[idx]

# ── receiver thread ───────────────────────────────────────────────────────────
def reader_thread(ser: serial.Serial) -> None:
    """Print every line received from the RP2040."""
    while True:
        try:
            raw = ser.readline()
            if raw:
                line = raw.decode('ascii', errors='replace').strip()
                if line.startswith('BTN:'):
                    print(f'\n  *** {line} ***')
                elif line.startswith('ACK:'):
                    print(f'  {line}')
        except Exception:
            break

# ── main ──────────────────────────────────────────────────────────────────────
def main() -> None:
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()

    print(f'Opening {port} @ {BAUD} baud...')
    ser = serial.Serial(port, BAUD, timeout=0.5)
    time.sleep(0.5)

    threading.Thread(target=reader_thread, args=(ser,), daemon=True).start()

    print('Streaming fake A-4E-C DCS-BIOS data. Press USR button on board.')
    print('Altitude: 10s per 10k ft climb/descent, 5s hold at each 10k mark.')
    print('Ctrl+C to stop.\n')

    # ── Altitude profile: (target_ft, duration_s) waypoints ──────────────────
    # Climb 0→40k with 5s holds, then descend 40k→0 with 5s holds.
    ALT_PROFILE = [
        (     0,  5),   # hold at ground
        ( 10000, 10),   # climb to 10k
        ( 10000,  5),   # hold
        ( 20000, 10),   # climb to 20k
        ( 20000,  5),   # hold
        ( 30000, 10),   # climb to 30k
        ( 30000,  5),   # hold
        ( 40000, 10),   # climb to 40k
        ( 40000,  5),   # hold at ceiling
        ( 30000, 10),   # descend to 30k
        ( 30000,  5),   # hold
        ( 20000, 10),   # descend to 20k
        ( 20000,  5),   # hold
        ( 10000, 10),   # descend to 10k
        ( 10000,  5),   # hold
        (     0, 10),   # descend to ground
    ]
    # Pre-compute cumulative times and total cycle length
    _cum = []
    _t = 0.0
    for alt, dur in ALT_PROFILE:
        _cum.append((_t, alt, dur))
        _t += dur
    ALT_CYCLE = _t   # total cycle length in seconds

    def alt_at(elapsed: float) -> float:
        """Return interpolated altitude in feet for the given elapsed time."""
        t = elapsed % ALT_CYCLE
        for i, (seg_start, seg_alt, seg_dur) in enumerate(_cum):
            seg_end = seg_start + seg_dur
            if t < seg_end:
                if i == 0:
                    return float(seg_alt)
                prev_alt = _cum[i - 1][1]
                frac = (t - seg_start) / seg_dur
                return prev_alt + (seg_alt - prev_alt) * frac
        return float(ALT_PROFILE[-1][0])

    start     = time.monotonic()
    arm_state = 0
    flap_step = 0
    last_arm  = start
    last_flap = start
    FLAP_INTERVAL = 8.0
    ARM_INTERVAL  = 15.0

    ser.write(SYNC)  # initial sync so parser locks on immediately

    try:
        while True:
            now     = time.monotonic()
            elapsed = now - start

            # ── Altitude — stepped profile with holds ─────────────────────────
            alt_feet = alt_at(elapsed)

            drum_10k, drum_1k, drum_100s = alt_to_drums(alt_feet)
            ser.write(frame(ADDR_ALT_10K,  drum_10k))
            ser.write(frame(ADDR_ALT_1K,   drum_1k))
            ser.write(frame(ADDR_ALT_100S, drum_100s))

            # Radar alt tracks baro alt (simplified — same value)
            ser.write(frame(ADDR_RADAR_ALT, drum_1k))

            # ── RPM — ramp 0→100% over 40 s then hold ────────────────────────
            rpm_norm = min(elapsed / 40.0, 1.0)
            ser.write(frame(ADDR_RPM, norm_to_u16(rpm_norm)))

            # ── Flap indicator ────────────────────────────────────────────────
            ser.write(frame(ADDR_FLAPS_IND, norm_to_u16(flap_step / 2.0)))

            # ── ARM_MASTER toggle ─────────────────────────────────────────────
            if now - last_arm >= ARM_INTERVAL:
                arm_state ^= 1
                ser.write(frame(ADDR_ARM_MASTER, 0x4000 if arm_state else 0x0000))
                print(f'\n[PC] ARM_MASTER = {"ARMED" if arm_state else "SAFE"}')
                last_arm = now

            # ── Flap step change ──────────────────────────────────────────────
            if now - last_flap >= FLAP_INTERVAL:
                flap_step = (flap_step + 1) % 3
                print(f'[PC] Flaps = {["UP","HALF","FULL"][flap_step]}'
                      f'  |  Alt = {alt_feet:.0f} ft')
                last_flap = now

            time.sleep(0.2)  # ~5 Hz

    except KeyboardInterrupt:
        print('\nStopped.')
    finally:
        ser.close()

if __name__ == '__main__':
    main()
