# CAN_Test_RP2040 — MicroPython on RP2040
#
# Replaces CAN_Test_Arduino in the OpenSkyhawk CAN bus integration test.
# See Firmware/Prototype/README.md for experiment procedures.
#
# UART0 (GP0 TX / GP1 RX) @ 250000 → STM32 master PA3/PA2  (direct, no divider)
# USB serial (REPL)               → operator console: commands in, analytics out
# I2C0  (GP4 SDA / GP5 SCL)      → SSD1306 OLED 128×64, address 0x3C
#
# Commands — send one character via any serial terminal connected to USB:
#   D — DCS capture: relay USB→UART, measure byte stream, analytics every 1 s
#   I — Idle (stop injection)
#   S — Slow sweep: one ControlPacket per 100 ms (10 pkts/sec)
#   F — Fast burst: one ControlPacket per 2 ms (~500 pkts/sec)
#   X — Extreme: back-to-back, no delay
#   T — Throughput: 1000 TEST_SEQ, RTT histogram (ECHO_1 only)

import sys
import select
import struct
import time
from machine import Pin, I2C, UART

try:
    import ssd1306
    _i2c = I2C(0, sda=Pin(4), scl=Pin(5), freq=400_000)
    oled = ssd1306.SSD1306_I2C(128, 64, _i2c)
except Exception:
    oled = None  # OLED absent or ssd1306.py not installed — continue without it

uart = UART(0, baudrate=250000, tx=Pin(0), rx=Pin(1), rxbuf=512)

# ── protocol constants ────────────────────────────────────────────────────────
DIAG_MAGIC    = 0xAA
DIAG_RTT      = 0x01
DIAG_HB       = 0x02
DIAG_ERR      = 0x03
CTRL_TEST_SEQ = 0xFFFF

# ── modes ─────────────────────────────────────────────────────────────────────
MODE_IDLE, MODE_SLOW, MODE_FAST, MODE_EXTREME, MODE_THROUGHPUT, MODE_DCS = range(6)
MODE_NAMES = {0:'IDLE', 1:'SLOW', 2:'FAST', 3:'XBURST', 4:'THRUPUT', 5:'DCS'}

mode = MODE_IDLE

# ── sweep table ───────────────────────────────────────────────────────────────
SWEEP = [
    (0x0001, 1), (0x0001, 0),                                           # MASTER_ARM_SW
    (0x0002, 1), (0x0002, 2), (0x0002, 0),                              # BLEED_AIR_KNOB
    (0x0003, 0x0000), (0x0003, 0x8000), (0x0003, 0xFFFF), (0x0003, 0x8000),  # HUD_VIDEO_BRT
    (0x0004, 1), (0x0004, 0),                                           # APU_CONTROL_SW
]
sweep_idx = 0

# ── throughput test ───────────────────────────────────────────────────────────
T_COUNT      = 1000
rtt_send_ms  = [0] * T_COUNT
rtt_buckets  = [0] * 8
t_sent       = 0
t_echoed     = 0
t_start      = 0
RTT_LABELS   = ['<1ms','1-2ms','2-3ms','3-6ms','6-11ms','11-21ms','21-51ms','>51ms']

def _rtt_bucket(rtt_ms):
    for i, threshold in enumerate([1, 2, 3, 6, 11, 21, 51]):
        if rtt_ms < threshold:
            return i
    return 7

# ── node state (populated by DIAG_HB frames from master) ─────────────────────
node_alive   = [False, False]
node_rx      = [0, 0]
node_flags   = [0, 0]
node_esr     = [0, 0]
node_last_ms = [0, 0]
HB_TIMEOUT_MS = 3000

can_tec   = 0
can_rec   = 0
can_flags = 0
last_rtt  = -1

# ── DCS capture state ─────────────────────────────────────────────────────────
DCS_CAPTURE_MS  = 5 * 60 * 1000   # 5 minutes
DCS_FRAME_GAP_MS = 5

dcs_start_ms      = 0
dcs_total_bytes   = 0
dcs_sec_bytes     = 0
dcs_last_byte_ms  = 0
dcs_cur_frame     = 0
dcs_peak_100ms    = 0
dcs_cur_100_bytes = 0
dcs_cur_100_start = 0
last_sec_report   = 0

# per-second summary accumulators
_sum_min_rate  = 0xFFFFFFFF; _sum_max_rate  = 0; _sum_rate_n  = 0; _sum_rate_tot  = 0
_sum_min_frame = 0xFFFFFFFF; _sum_max_frame = 0; _sum_frame_n = 0; _sum_frame_tot = 0

def _dcs_reset(now):
    global dcs_start_ms, dcs_total_bytes, dcs_sec_bytes, dcs_last_byte_ms
    global dcs_cur_frame, dcs_peak_100ms, dcs_cur_100_bytes, dcs_cur_100_start
    global last_sec_report
    global _sum_min_rate, _sum_max_rate, _sum_rate_n, _sum_rate_tot
    global _sum_min_frame, _sum_max_frame, _sum_frame_n, _sum_frame_tot
    dcs_start_ms = dcs_last_byte_ms = dcs_cur_100_start = last_sec_report = now
    dcs_total_bytes = dcs_sec_bytes = dcs_cur_frame = 0
    dcs_peak_100ms = dcs_cur_100_bytes = 0
    _sum_min_rate = _sum_min_frame = 0xFFFFFFFF
    _sum_max_rate = _sum_max_frame = _sum_rate_n = _sum_rate_tot = 0
    _sum_frame_n  = _sum_frame_tot = 0

# ── DIAG receive buffer ───────────────────────────────────────────────────────
_diag_buf = bytearray(8)
_diag_pos = 0

def _process_diag(p):
    global last_rtt, t_echoed, can_tec, can_rec, can_flags
    typ = p[1]
    if typ == DIAG_RTT:
        seq = struct.unpack_from('<H', p, 2)[0]
        if seq < T_COUNT:
            rtt = time.ticks_diff(time.ticks_ms(), rtt_send_ms[seq % T_COUNT])
            rtt_buckets[_rtt_bucket(rtt)] += 1
            last_rtt = rtt
            t_echoed += 1
    elif typ == DIAG_HB:
        nid = p[2] - 1
        if 0 <= nid <= 1:
            node_alive[nid]   = True
            node_flags[nid]   = p[3]
            node_rx[nid]      = struct.unpack_from('<H', p, 4)[0]
            node_esr[nid]     = struct.unpack_from('<H', p, 6)[0]
            node_last_ms[nid] = time.ticks_ms()
            print('[HB] node={} flags=0x{:02X} rx={} ESR=0x{:04X}'.format(
                p[2], p[3], node_rx[nid], node_esr[nid]))
    elif typ == DIAG_ERR:
        can_tec = p[2]; can_rec = p[3]; can_flags = p[4]
        print('[ERR] TEC={} REC={} flags=0x{:02X}'.format(can_tec, can_rec, can_flags))

def _drain_uart():
    global _diag_pos
    while uart.any():
        b = uart.read(1)[0]
        if _diag_pos == 0 and b != DIAG_MAGIC:
            continue
        _diag_buf[_diag_pos] = b
        _diag_pos += 1
        if _diag_pos == 8:
            _process_diag(_diag_buf)
            _diag_pos = 0

# ── packet transmit helpers ───────────────────────────────────────────────────
def _send_packet(ctrl_id, value):
    uart.write(struct.pack('<HH', ctrl_id, value))

def _send_seq(seq):
    rtt_send_ms[seq % T_COUNT] = time.ticks_ms()
    _send_packet(CTRL_TEST_SEQ, seq)

# ── OLED update ───────────────────────────────────────────────────────────────
_last_oled_ms = 0

def _update_oled(now):
    global _last_oled_ms
    if oled is None or time.ticks_diff(now, _last_oled_ms) < 250:
        return
    _last_oled_ms = now
    oled.fill(0)
    for i in range(2):
        s = 'N{}: OK rx:{}'.format(i+1, node_rx[i]) if node_alive[i] else 'N{}: DEAD'.format(i+1)
        oled.text(s, 0, i * 10)
    flag_s = ('BOFF ' if can_flags & 1 else '') + ('EPVF' if can_flags & 2 else '')
    oled.text('TEC:{} REC:{} {}'.format(can_tec, can_rec, flag_s).strip(), 0, 22)
    oled.text(('RTT:{}ms'.format(last_rtt) if last_rtt >= 0 else 'RTT:--'), 0, 34)
    oled.text('Mode:{}'.format(MODE_NAMES.get(mode, '?')), 0, 46)
    oled.show()

# ── command input ─────────────────────────────────────────────────────────────
_spoll = select.poll()
_spoll.register(sys.stdin, select.POLLIN)

def _check_cmd(now):
    global mode, sweep_idx, t_sent, t_echoed, t_start
    if not _spoll.poll(0):
        return
    c = sys.stdin.read(1).upper()
    if   c == 'D':
        mode = MODE_DCS;        _dcs_reset(now);  print('[D] DCS capture. USB serial is 250000 from boot.')
    elif c == 'I':
        mode = MODE_IDLE;                          print('[I] Idle.')
    elif c == 'S':
        mode = MODE_SLOW;   sweep_idx = 0;         print('[S] Slow sweep (100 ms/pkt).')
    elif c == 'F':
        mode = MODE_FAST;   sweep_idx = 0;         print('[F] Fast burst (2 ms/pkt, ~500/sec).')
    elif c == 'X':
        mode = MODE_EXTREME; sweep_idx = 0;        print('[X] Extreme burst (no delay).')
    elif c == 'T':
        mode = MODE_THROUGHPUT
        t_sent = t_echoed = 0; t_start = now
        for i in range(8): rtt_buckets[i] = 0
        print('[T] Throughput test — 1000 SEQ, counting ECHO_1 only.')

# ── DCS capture ───────────────────────────────────────────────────────────────
def _run_dcs(now):
    global dcs_total_bytes, dcs_sec_bytes, dcs_last_byte_ms, dcs_cur_frame
    global dcs_peak_100ms, dcs_cur_100_bytes, dcs_cur_100_start, last_sec_report
    global _sum_min_rate, _sum_max_rate, _sum_rate_n, _sum_rate_tot
    global _sum_min_frame, _sum_max_frame, _sum_frame_n, _sum_frame_tot

    if _spoll.poll(0):
        data = sys.stdin.buffer.read(256)
        if data:
            uart.write(data)
            n = len(data)
            dcs_total_bytes   += n
            dcs_sec_bytes     += n
            dcs_cur_100_bytes += n

            gap = time.ticks_diff(now, dcs_last_byte_ms)
            if gap > DCS_FRAME_GAP_MS and dcs_cur_frame > 0:
                # frame boundary: record completed frame size
                if dcs_cur_frame < _sum_min_frame: _sum_min_frame = dcs_cur_frame
                if dcs_cur_frame > _sum_max_frame: _sum_max_frame = dcs_cur_frame
                _sum_frame_n += 1; _sum_frame_tot += dcs_cur_frame
                dcs_cur_frame = 0
            dcs_cur_frame     += n
            dcs_last_byte_ms   = now

    if time.ticks_diff(now, dcs_cur_100_start) >= 100:
        if dcs_cur_100_bytes > dcs_peak_100ms: dcs_peak_100ms = dcs_cur_100_bytes
        dcs_cur_100_bytes = 0; dcs_cur_100_start = now

    if time.ticks_diff(now, last_sec_report) >= 1000:
        r = dcs_sec_bytes
        if r < _sum_min_rate: _sum_min_rate = r
        if r > _sum_max_rate: _sum_max_rate = r
        _sum_rate_n += 1; _sum_rate_tot += r
        print('DCS rate={}B/s total={}B peak100ms={}B'.format(r, dcs_total_bytes, dcs_peak_100ms))
        dcs_sec_bytes = 0; last_sec_report = now

    if time.ticks_diff(now, dcs_start_ms) >= DCS_CAPTURE_MS:
        print('=== DCS Capture Summary (5 min) ===')
        print('Total bytes : {}'.format(dcs_total_bytes))
        print('Peak 100 ms : {} bytes  ← min STM32 UART RX buffer'.format(dcs_peak_100ms))
        if _sum_rate_n:
            print('Byte rate   : min={} avg={} max={} B/s'.format(
                _sum_min_rate, _sum_rate_tot // _sum_rate_n, _sum_max_rate))
        if _sum_frame_n:
            print('Frame size  : min={} avg={} max={} bytes'.format(
                _sum_min_frame, _sum_frame_tot // _sum_frame_n, _sum_max_frame))
        print('===================================')
        _dcs_reset(now)

# ── injection ─────────────────────────────────────────────────────────────────
_last_inject = 0

def _run_injection(now):
    global sweep_idx, _last_inject, mode, t_sent, t_echoed

    if mode == MODE_THROUGHPUT:
        if t_sent < T_COUNT:
            _send_seq(t_sent); t_sent += 1
        elif t_echoed >= T_COUNT or time.ticks_diff(now, t_start) > 10000:
            print('=== RTT Histogram ===')
            for i, lbl in enumerate(RTT_LABELS):
                print('{}: {}'.format(lbl, rtt_buckets[i]))
            print('Sent: {}  Echoed: {}'.format(t_sent, t_echoed))
            mode = MODE_IDLE
        return

    interval = 100 if mode == MODE_SLOW else 2 if mode == MODE_FAST else 0
    if interval == 0 or time.ticks_diff(now, _last_inject) >= interval:
        ctrl_id, value = SWEEP[sweep_idx]
        _send_packet(ctrl_id, value)
        sweep_idx = (sweep_idx + 1) % len(SWEEP)
        _last_inject = now

# ── node watchdog ─────────────────────────────────────────────────────────────
_last_watchdog = 0

def _check_timeouts(now):
    global _last_watchdog
    if time.ticks_diff(now, _last_watchdog) < 500:
        return
    _last_watchdog = now
    for i in range(2):
        if node_alive[i] and time.ticks_diff(now, node_last_ms[i]) > HB_TIMEOUT_MS:
            node_alive[i] = False
            print('[DEAD] node={}'.format(i + 1))

# ── startup ───────────────────────────────────────────────────────────────────
print('CAN_Test_RP2040 ready.')
print('Commands: D=DCS I=idle S=slow F=fast X=extreme T=throughput')

if oled:
    oled.fill(0)
    oled.text('CAN Test RP2040', 0, 20)
    oled.text('Ready', 0, 34)
    oled.show()

# ── main loop ─────────────────────────────────────────────────────────────────
while True:
    now = time.ticks_ms()
    _check_cmd(now)
    _drain_uart()
    if mode == MODE_DCS:
        _run_dcs(now)
    elif mode != MODE_IDLE:
        _run_injection(now)
    _check_timeouts(now)
    _update_oled(now)
