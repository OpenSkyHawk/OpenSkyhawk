# 07 — SimGateway API

**Owns:** `HIDAxis`, `HIDButton`, and `HIDHatSwitch` class specs, SimGateway relay contract, HID
dispatch logic, HID report batching rule, USB identity, UART initialisation, TinyUSB HID
descriptor and report layout.
**Does not own:** HID frame wire format (→ 03), CAN protocol (→ 02), boot sequence (→ 09).

---

## Role

SimGateway is the single RP2040 module that bridges the PC to the CAN cluster:

- **USB CDC ↔ UART relay:** raw DCS-BIOS byte stream forwarded transparently in both
  directions. SimGateway does **not** run the DCS-BIOS library — it never parses or
  interprets the DCS-BIOS stream.
- **HID dispatch:** intercepts HID frames on UART RX (identified by `HID_MAGIC` — see
  `03-uart-usb-hid-protocol.md`); routes controlIds to `HIDAxis`, `HIDButton`, or `HIDHatSwitch`.

SimGateway has no knowledge of DCS-BIOS control names, addresses, or input map entries.

---

## HIDAxis

Declared in the SimGateway sketch. Receives 16-bit value (0–65535) from PanelGroup via
CAN → PanelBridge → HID frame → UART.

The constructor takes a `controlId` and a joystick **axis index** (0–7). The library applies the
stored calibration, maps the result to ±32767 (`value − 32768`), and writes it to the TinyUSB
HID report internally. The sketch has no knowledge of the HID backend, the calibration, or the
scaling.

`axisIndex()` exposes the index, which is also the axis's slot in the calibration blob.

```cpp
// SimGateway sketch only:
OpenSkyhawk::HIDAxis roll    (CTRL_ROLL,     0);
OpenSkyhawk::HIDAxis pitch   (CTRL_PITCH,    1);
OpenSkyhawk::HIDAxis throttle(CTRL_THROTTLE, 2);
OpenSkyhawk::HIDAxis rudder  (CTRL_RUDDER,   3);
OpenSkyhawk::HIDAxis brakeL  (CTRL_BRAKE_L,  4);
OpenSkyhawk::HIDAxis brakeR  (CTRL_BRAKE_R,  5);
OpenSkyhawk::HIDAxis zoom    (CTRL_ZOOM,     6);
```

### Axis Calibration

**Calibration is never a constructor argument.** Endpoints are per-unit — hall sensor spread,
magnet geometry, print tolerance — so baking them in would mean a reflash per build, which
rules out kit builders. They live in flash, are loaded once by `SimGateway::setup()`, and are
looked up by `axisIndex`. Sketch declarations are therefore identical calibrated or not.

This is the difference from `AnalogInput`, whose `minRaw`/`maxRaw` *are* constructor arguments
and therefore compile-time only. Nodes stay at their defaults so there is one transform in the
chain and the gateway can see raw values.

Storage is `OpenSkyhawk::CalBlob` — magic, version, eight `AxisCal` records, CRC — held in RAM
and applied by `HIDAxis::dispatch()`. Each `AxisCal` is three captured points:

| Field | Meaning |
|---|---|
| `min` | raw value at the low mechanical stop |
| `centre` | raw value at rest — **captured, never derived** |
| `max` | raw value at the high mechanical stop |
| `deadzone` | reserved, always 0 in version 1; nothing reads it |

**Two linear segments, not one.** A single `min..max` stretch fixes an axis's range but leaves
it resting off-centre whenever travel is asymmetric, which for a rotating-magnet hall axis is
the normal case — the normal field component goes as sin θ, so raw midpoint is not physical
midpoint. Measured on the bench rig, Roll's centre sits 7.1% above its midpoint and a single
linear map left the stick resting at +4669. Two segments put `centre` on 32768 exactly.

`centre` is **always provided data**. Neither the device nor the client substitutes
`(min + max) / 2`.

An axis is "calibrated" iff `min < centre < max`. There is no stored validity flag: a zeroed
blob (`.bss`, before load) and an erased one (all `0xFF`) both fail that predicate, so both
fail closed to identity passthrough. An uncalibrated gateway is byte-identical in behaviour to
a build without this feature.

Out of scope, deliberately: response curves and axis inversion (both belong in the sim, and a
firmware curve would stack with DCS's per-aircraft curves), and boot-time centre auto-zero (the
stick may not be at rest at power-on, and not every axis has a rest position).

**Not covered by the v1.0 compatibility policy (#295).** The stored blob survives a normal reflash,
but its layout (`CAL_VERSION`) — and the EEPROM-emulation sector, whose position comes from the
RP2040 core — may change in any release, and every axis then reads uncalibrated. The client keeps a
copy per board serial and offers to restore it (same PC); otherwise, recalibrate.

---

## HIDButton

Declared in the SimGateway sketch. `value != 0` → pressed; `value == 0` → released.
TinyUSB HID descriptor supports 128 buttons (index 0–127).

```cpp
// SimGateway sketch only:
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0);  // button index 0
```

---

## HIDHatSwitch

Declared in the SimGateway sketch. `value` encodes direction: 0 = centered, 1 = N, 2 = NE,
3 = E, 4 = SE, 5 = S, 6 = SW, 7 = W, 8 = NW. Values > 8 treated as centered.
TinyUSB HID descriptor supports 4 hat switches (index 0–3).

```cpp
// SimGateway sketch only:
OpenSkyhawk::HIDHatSwitch hat(CTRL_HAT_0, 0);  // hat index 0
```

---

## HID controlId Allocation

Defined in `HIDControls.h` (platform-agnostic library shared with CANProtocol):

| Range | Use | HID descriptor capacity |
|-------|-----|-------------------------|
| `0x0010`–`0x001F` | Axes (16 slots) | 8 axes (X/Y/Z/Rx/Ry/Rz/Slider/Dial) |
| `0x0020`–`0x002F` | Hat switches (16 slots) | 4 hat switches |
| `0x0030`–`0x00AF` | Buttons (128 slots) | 128 buttons |
| `0x00B0`–`0x00FF` | Reserved HID expansion | Not exposed by current USB report |

```cpp
// Axes
#define CTRL_ROLL     0x0010
#define CTRL_PITCH    0x0011
#define CTRL_THROTTLE 0x0012
#define CTRL_RUDDER   0x0013
#define CTRL_BRAKE_L  0x0014
#define CTRL_BRAKE_R  0x0015
#define CTRL_ZOOM     0x0016

// Hat switches (0x0020–0x002F reserved; no constants defined until needed)

// Buttons
#define CTRL_TRIGGER  0x0030   // first button slot
```

---

## HID Platform Compatibility

Verified via `hid_stress` test sketch (VID 0x2E8A / PID 0x4135). Production descriptor
(8 axes / 128 buttons / 4 hats / 34 bytes) is within all platform limits.

### TinyUSB report size hard limit

`CFG_TUD_HID_EP_BUFSIZE = 64` is hardcoded unconditionally in
`tusb_config_rp2040.h` (framework-arduinopico package). Build-flag overrides are silently
ignored. **Maximum HID report = 64 bytes.** Production report = 34 bytes.

Exceeding 64 bytes causes `Adafruit_USBD_HID::sendReport()` to return `false` without
sending — no error, no crash, just silent drop. Never expand the report struct above 64 bytes.

### Chrome Gamepad API (macOS)

| Control | Production descriptor | Chrome sees |
|---|---|---|
| GD axes (0x30–0x37) | 8 | 8 ✓ |
| Hats | 4 | 1 (hat 0 as virtual axis — see note) |
| Buttons | 128 | **32** (hard API cap) |
| Vendor axes (0xFF00 page) | 0 | 0 (ignored entirely) |

Hat insertion note: macOS IOKit inserts Hat Switch (usage 0x39) as a virtual axis at its
GD usage-number position — between Wheel (0x38) and Vx (0x40) — regardless of where the
hat appears in the descriptor. Hat null nibble (0xF = 15) normalises to
(15/7)×2−1 ≈ 3.286, outside [−1, +1]. Only hat 0 appears as a single axis.

32-button cap is not a concern: DCS uses DirectInput, not the Gamepad API.

### DirectInput / DCS (Windows)

DIJOYSTATE2 limits (confirmed in DCS A-4E-C Controls):

| Control | Production descriptor | DCS sees |
|---|---|---|
| GD axes (0x30–0x37) | 8 | **8** ✓ |
| Buttons | 128 | **128** ✓ |
| POV hats | 4 | **4** ✓ |
| Extended GD axes (0x38, 0x40–0x46) | 0 | 0 (ignored by DirectInput) |
| Vendor axes (0xFF00) | 0 | 0 |

DCS exposes each hat as 8 directional bindings: `JOY_BTN_POVn_U/D/L/R/UL/UR/DL/DR`.
Production descriptor is a full match for DIJOYSTATE2 — no limits are exceeded.

---

## Dispatch Logic

After draining all HID frames from UART in one `SimGateway::loop()` iteration:
1. Walk the `HIDAxis` linked list; for each matching `controlId` apply that axis's calibration, then write `value − 32768` to the TinyUSB HID axis report field.
2. Walk the `HIDButton` linked list; for each matching `controlId` set/clear the corresponding button bit.
3. Walk the `HIDHatSwitch` linked list; for each matching `controlId` write the direction nibble.
4. If any setter fired, send the HID report **once** to flush it.

Calling `Send()` once per drain cycle (not once per frame) keeps HID report rate predictable.

**Order within step 1 matters: calibrate, then offset.** The pipeline stays unsigned until the
final step, so the two-segment map never divides on signed values. The `− 32768` offset is
invariant, not calibration-dependent — two different centres are in play and must not be
conflated. `AxisCal::centre` is the axis's *physical* rest point, which the map absorbs;
`− 32768` is the re-centring of a 16-bit range, a property of the representation. Since the map
always returns 0–65535, making the offset variable would apply the centring twice.

```
raw 0..65535 ──calibration──▶ 0..65535 ──(v − 32768)──▶ int16_t ──▶ HID report
```

---

## Relay Contract

**Host → PanelBridge** (the DCS-BIOS export stream): `SimGateway::loop()` does not buffer,
parse, or delay these bytes. Every byte from USB CDC is forwarded to UART in the same call, up to
`CDC_TO_UART_BUDGET` (64) bytes per call. The cap exists because the UART write blocks once its
32-byte hardware FIFO is full, and an uncapped drain of an export burst would hold `loop()` — and
with it the PanelBridge→host drain — for tens of milliseconds. Throughput is still the UART's.

This means `DcsBios::loop()` on PanelBridge receives the byte stream with USB CDC latency
only — no additional SimGateway processing latency.

**PanelBridge → host is line-granular and never blocks (#355).** Everything PanelBridge sends the
host is a newline-framed DCS-BIOS message (`name arg\n`: commands, `_NODE_STATUS`,
`_NODE_TELEM`), so SimGateway assembles each line and writes it **whole** when the CDC FIFO has
room for all of it. Otherwise the line is dropped and counted — never truncated, because half a
command is a different command. The reason: Adafruit TinyUSB's `write()` loops for as long as the
host holds DTR but stops reading, and a `loop()` stalled there stops the UART drain, the HID path
and the status LEDs.

| Line | Fate | Counter (`DIAG`) |
|---|---|---|
| printable ASCII (`0x20`–`0x7E`) + `\n`, ≤ `RELAY_LINE_MAX` (128) bytes, CDC room | written whole | — |
| as above, no CDC room | dropped whole | `LINES_DROPPED` |
| contains any other byte (incl. `\r`, or an `0xAA` that began no HID frame) | dropped | `LINES_CORRUPT` |
| longer than `RELAY_LINE_MAX` | dropped | `LINES_OVERLONG` |

The longest real line is a `_NODE_STATUS` row at about 42 bytes. A healthy link never drops a
line; the counters exist to tell a host that stopped reading from line noise.

**One exception, inbound only: calibration frames are consumed** (see
`03-uart-usb-hid-protocol.md#calibration-protocol-usb-cdc`). The parser runs on every inbound
byte because `HELLO` and `GET_CAL` are answered outside a session, but it consumes a candidate
only once it has passed **all three** of magic, `LEN`-matches-`TYPE`, and CRC. A candidate
failing any of them is handed back to the relay byte-for-byte, in order — so DCS-BIOS traffic
can never be silently eaten, and the guarantee above still holds for everything that is not a
valid calibration frame.

The cost is one comparison per byte against a UART that is roughly three orders of magnitude
slower per byte, so the relay contract's latency claim is unaffected.

The outbound direction is **not** taken over: `_NODE_STATUS` and DCS-BIOS text keep flowing during
a calibration session, with calibration frames injected alongside them. Because lines are
buffered, a frame always goes out between two lines, never inside one. Frames follow the same
whole-or-drop rule as lines (`FRAMES_DROPPED`).

---

## Status LEDs

Two board-mounted LEDs on the `Gateway_Bridge` board report the gateway state: **GREEN = GP2,
RED = GP3**, active-high (HIGH = on). A non-blocking `millis()` animator is ticked from
`SimGateway::loop()`; `SimGateway::setup()` calls `statusLedBegin()` (pin config), so the sketch
needs no LED code. The RP2040 module's onboard WS2812 (GP16) is not used.

This is a **SimGateway-local** state machine (issue #94) — it shares only the animation
vocabulary with `STM32Board` (issue #93), not the engine; the MCU-agnostic `StatusLedAnimator`
was not adopted.

States, highest priority first (the topmost active state wins):

| Pri | State | Trigger | LED |
|---|---|---|---|
| 1 | `FAULT` | uart0 RX hardware error — RSR overrun/framing/parity/break (self-clears, see below) | RED fast (4 Hz) |
| 2 | `NO_HOST` | USB not enumerated (`TinyUSBDevice.mounted()` false), or unplugged after a mount | RED solid |
| 3 | `STREAMING` | inbound host bytes seen within the last 500 ms — forwarded **or consumed** | GREEN solid |
| 4 | `USB_IDLE` | USB mounted, no recent DCS data | GREEN slow (1 Hz) |
| 5 | `INIT` | booted, never mounted, within the 2 s init window | RED slow (brief) |

Animation vocabulary (shared with STM32Board): OFF / SOLID / SLOW 1 Hz / FAST 4 Hz / ALT 500 ms /
PULSE ~50 ms. PULSE-on-traffic is implemented but disabled by default (STREAMING is plain solid).
Non-blocking — `millis()`, never `delay()`.

### Signal sources

- **STREAMING vs USB_IDLE** — relay step 1 timestamps the last inbound byte from the host,
  whether it was forwarded to the cockpit or consumed as a calibration frame; STREAMING while
  `now − lastCdcRx ≤ 500 ms`. A calibration session counts as the host talking, which is why
  the light stays green with DCS closed.
- **NO_HOST / INIT** — `TinyUSBDevice.mounted()` polled each tick. A sticky "ever-mounted" flag
  distinguishes INIT (never mounted, `now < 2000 ms`) from NO_HOST (unplugged after a mount, or
  init window expired with no mount).
- **FAULT** — read directly from the `uart0` PL011 error register (`uart_get_hw(uart0)->rsr`,
  bits OE/BE/PE/FE), cleared on every read (write-to-clear). The arduino-pico `SerialUART` RX IRQ
  drops framing/parity bytes without ever touching RSR, so RSR is the only reliable error source.
  `Serial1 == uart0` on this board; a board wiring the link to uart1 must change the hardware
  pointer. Each bit is also counted separately for `DIAG` — see *Link Health* below.
- **Not FAULT: a starved `loop()`.** The RX IRQ empties the PL011 into a software queue
  (`UART_RX_QUEUE_BYTES` = 1024, ~40 ms at 250 000 baud; the core default is 32). If `loop()`
  falls behind, that queue overflows and bytes are lost **without** any RSR bit. It is counted
  (`UART_RX_OVERFLOW`) but does not light FAULT. RSR OE means the IRQ itself was held off.

### FAULT latch / recovery

FAULT is latched, not momentary. On any error bit it latches and re-stamps; it clears when
**both** (a) ≥ 2 s have elapsed since the last error (a visibility floor, ~8 fast flashes) **and**
(b) an error-free byte has arrived on uart0 RX *after* the fault.

**Self-clear (#355).** PanelBridge sends almost nothing with no DCS running, so (b) may never
happen, and every FAULT seen in the field came with the host link lost for good. So once FAULT has
been latched for `FAULT_RESTART_MS` (3 s, measured from the latch, not the latest error), the UART
is restarted (`end()`/`begin()`, RSR cleared, HID parser and host-bound line reset) and the latch
cleared. Errors that persist re-latch on the next tick, so a genuinely bad link keeps showing
FAULT and restarts every 3 s. Counted as `UART_RESTARTS`; the event itself stays in `DIAG`.

### Public API (additions)

```cpp
void SimGateway::statusLedBegin(); // configure GP2/GP3 outputs (both off); called by setup()
void SimGateway::statusTick();     // advance the state machine + animation; called by loop()
enum class SimGateway::LedState : uint8_t { FAULT, NO_HOST, STREAMING, USB_IDLE, INIT };
enum class SimGateway::Anim     : uint8_t { OFF, SOLID, SLOW, FAST, ALT, PULSE };
```

`SIMGATEWAY_TEST` builds add injection/readback hooks (`statusInject`, `statusResolve`,
`statusFaultStep`, `statusFaultRestartCheck`, `statusState`, `statusAnim`, `statusRedLevel`,
`statusGreenLevel`, `statusResetForTest`) that exercise the pure state/animation/latch logic with
no GPIO, TinyUSB, or register access. See `Firmware/Tests/SimGateway/tests/led_state/` and
`link_health/`.

---

## Link Health and Self-Recovery

Issue #355. The Gateway's RP2040 has twice dropped off the USB bus while its core kept running
(FAULT flashing, port gone, only a replug — which is a power cycle — recovered it). The trigger was
never reproduced. The firmware therefore recovers on its own, whatever the trigger, and records
enough to tell causes apart next time.

### Recovery ladder

Pure logic in `LinkRecovery.h` / `.cpp`; `SimGateway::loop()` samples the inputs and applies the
action.

**Healthy** = `TinyUSBDevice.mounted()` **and not** (`TinyUSBDevice.suspended()` with a UART
error in the last 30 s) **and not** a CDC stall. The RP2040 is powered only from USB, so
running-but-unmounted is abnormal. Suspend alone is a sleeping host and is left alone.

A **CDC stall** is the configured-but-dead case `mounted()` cannot see: the host holds DTR, bytes
are pending in the CDC TX FIFO, and nothing has drained for `CDC_STALL_MS` (1 s). "Drained" means
free space now plus what the relay wrote since the last sample exceeds the free space then, so a
link draining exactly as fast as it is refilled still counts as progress. The host OS keeps polling
an open port into its own buffer (≈57 KB on macOS) even when the application is not reading, so a
FIFO that stays full means the host's USB stack stopped polling, not that an app is slow. Without
this, every line would be dropped while `loop()` and the watchdog ran on, and the ladder would never
start. Idle (nothing pending) and no host (DTR low) are never a stall. Onsets are counted
(`CDC_STALLS`).

| Step | Condition | Action |
|---|---|---|
| 1 | unhealthy for `LINK_BAD_MS` (5 s) | `TinyUSBDevice.detach()`, then `attach()` `LINK_REATTACH_GAP_MS` (100 ms) later — the host sees a fresh plug-in |
| 2 | still unhealthy `LINK_REBOOT_AFTER_MS` (5 s) after the re-attach | reboot — resets the USB controller like a replug, calibration is in flash |
| 3 | booted by step 2 and still unhealthy | re-attach every `LINK_BACKOFF_MS` (60 s); never reboot again until the link has been healthy once |

Step 3 keeps a Gateway on a charger, or back-fed with no host, from reboot-looping. A detach is
always followed by its attach. Health at any point resets the ladder.

### Hardware watchdog

`WATCHDOG_MS` (3 s), armed by the first `SimGateway::loop()` call — not in `setup()`, so a sketch
may block in its own `setup()` — and fed by every call. Production builds only. A watchdog reset
is recorded and survives the reboot.

### USB is serviced from the main loop only (#389, D33)

The same first `loop()` call disables TinyUSB's task IRQ, so `tud_task()` runs only from `yield()`
after every `loop()`. From interrupt context it could wait forever on the endpoint mutex that
`yield()`'s CDC flush holds — under heavy host traffic the watchdog then rebooted the Gateway. The
IRQ is found at run time: the claimed, enabled user IRQ with a handler (SimGateway claims none of
its own). Production builds only.

### Diagnostics (`GET_DIAG` / `DIAG`)

`SimGateway::diagnostics()` returns a `GatewayDiag`; the host reads the same data with the
calibration protocol's `GET_DIAG` request (`03-uart-usb-hid-protocol.md#payloads`), answered at
any time. Counts: each RSR bit separately (OE / FE / BE / PE), software RX-queue overflow, the
three line-drop reasons, dropped calibration frames, UART restarts, USB re-attaches, CDC stalls. Times: first
and latest UART error, first and latest healthy→unhealthy USB edge — comparing the two answers
"which came first". Recovery reboots, watchdog resets and this boot's reason live in watchdog
scratch registers 0–3, so they survive a warm reboot and reset on power-on.

**Wire contract:** `DIAG` is host-visible. SkyHawkClient takes the layout from `AxisCal.h` via
`npm run sync`, never hand-copied. Bench reader: `tools/simgw_bench/simgw_bench.py`.

---

## Uploader (#366)

`Uploader` (`Uploader.h/.cpp`, library-internal) makes the Gateway the firmware uploader: it stages an
image the client streams (`UPL_*`, `03` § Firmware upload) and flashes a board's bootloader over CAN
through PanelBridge (`12-bootloader.md`, relay format `04`). Portable C++: SimGateway feeds it the
client's frames, every line from PanelBridge, and a tick from `loop()`; it answers through callbacks.

**Staging.** A 124 KB RAM buffer (`UPL_MAX_IMAGE_BYTES` — an app on a 128 KB board), `0xFF` past the
image so the last block's padding is right. `UPL_END` verifies the CRC-32/MPEG-2 (the identity block's
algorithm, CRC word skipped) and that the identity block matches `UPL_BEGIN`.

**The flash** — a non-blocking state machine, phases reported in `UPL_STATUS`:

| Phase | Does | Ends |
|---|---|---|
| ENTERING_BOOT | `CONNECT` every 100 ms on `BOOT_CMD_n` (a running app with a bootloader resets into it; a reset board is caught). Unconfigured target: waits for a `0x7C1` announcement for the UID hash, then `CONNECT [uid16]` on `0x7C0` | `CONNECTED`; 10 s → `NO_BOOTLOADER` (its `_NODE_STATUS` row shows bootloader 00) or `NO_RESPONSE` |
| CONNECTED | Checks the bootloader's answer: kind (`KIND_MISMATCH`), room = flash − 4 KB (`TOO_BIG_FOR_BOARD`), NODE_ID (`ID_MISMATCH` unless `newNodeId` = the image's) — **before anything is erased** | → SETTING_ID or ERASING |
| SETTING_ID | `SET_NODE_ID` → the board resets as the new node, app invalid | → ENTERING_BOOT at the new ID |
| ERASING | `BEGIN` (the bootloader erases the image's pages first) | `BEGUN`; 6 s → retry |
| WRITING | Up to `WINDOW_BLOCKS` (4) blocks in flight, **one block onto the UART per `loop()`** so HID and USB keep being served; go-back-N on `BLOCK_ACK.next`; 0.5 s without an ack → resend from the first unacked block (6 times → retry); `next` going backwards → the board restarted → retry | all acked |
| VERIFYING | `END` with the CRC | `OK` → REBOOTING; `WRONG_NODE` / `WRONG_KIND` → final; else retry |
| REBOOTING | `GO` | `GOING` (or 3 s: it reboots anyway) |
| CONFIRMING | Waits for the image's NODE_ID row in `_NODE_STATUS`, newer than GO, app running | version equal → `OK`, different → `VERSION_MISMATCH`; 10 s → `TIMEOUT` |

A transport failure retries the whole flash from ENTERING_BOOT (`UPL_RETRIES` = 2; the board is then in
its bootloader, app invalid, and answers at once); exhausted → `WRITE_FAILED` / `VERIFY_FAILED`. The
result goes out as `UPL_RESULT` if the client is still there (a frame within 5 s), and is always kept for
`UPL_INFO_ACK`.

**While flashing it owns the UART:** SimGateway drops the client's export bytes (CDC → UART) and the
uploader writes the relay records itself (sync + records, a few hundred bytes per block); `_BOOT_RSP`
lines are consumed, not relayed. `_NODE_STATUS` lines are always read (the confirmation) and always
relayed. Calibration and upload exclude each other (`NACK BUSY`).

**Target `0` — the Gateway's own cockpit side (#367).** PanelBridge runs the `bridge` bootloader
(`12` § The Gateway's cockpit side), which talks USART2 straight to the RP2040, so there is no relay:

- **The image:** the staged one if the client staged one, else the **bundled** bridge image — the
  SimGateway build compiles the PanelBridge project and links its `.bin` in
  (`Firmware/shared/bundle_bridge.py`, `custom_bridge_project` / `custom_bridge_env`; Uploader's weak
  `osh_bundled_bridge`). `UPL_INFO_ACK.bridgeFw` is its version (0.0.0 = none) — the client compares it
  with the node-0 row to tell the cockpit side is behind. Nothing staged and nothing bundled →
  `NACK NOT_STAGED`.
- **ENTERING_BOOT:** a write to `BRIDGE_ENTER_EXPORT_ADDRESS` (`0xFF02`) every 500 ms until the
  bootloader announces (2 s at most), then `CONNECT` every 100 ms — pressing the Gateway's reset button
  catches an app that ignores the request. 10 s → `NO_RESPONSE`.
- **The UART is the uploader's both ways** from ENTERING_BOOT to CONFIRMING: every byte PanelBridge sends
  goes to `Uploader::onUartByte()` (the `0xAA` serial frames), not the line / HID parsers. SimGateway
  resets both parsers at each hand-over, so a line cut off at the switch can't swallow the next one.
- **CONFIRMING:** asks for the roster (`NODE_STATUS_REQ_ADDR`) every 500 ms — the bridge's boot row can
  be lost in the hand-over from its bootloader — until the node-0 row shows the app running.

**Measured** (bench, 27.7 KB image): entering 0.12 s, erase 0.61 s, writing 2.22 s (12.5 KB/s), confirm
~0.2 s — 3.2 s update. A mid-transfer reset retried and finished (2 attempts, 4.4 s); a client that left
1 s in found `OK` in `UPL_INFO_ACK` afterwards. The bridge (33.5 KB, #367): **4.1 s** from the bundle
(writing 2.8 s); a bridge reset mid-write → retried, `OK` (6.4 s); the Gateway rebooted mid-write → the
bridge stayed in its bootloader and the next `UPL_FLASH 0` finished (4.8 s); an app that ignores the
request + the reset pressed during ENTERING_BOOT → caught, `OK`.

## UART Pin Contract

The standard SimGateway link uses RP2040 `Serial1` / UART0:

| Direction | RP2040 pin | PanelBridge STM32 pin |
|-----------|------------|-----------------------|
| SimGateway TX → PanelBridge RX | GP0 | PA3 / UART2 RX |
| SimGateway RX ← PanelBridge TX | GP1 | PA2 / UART2 TX |

`SimGateway::setup(Serial1)` configures these defaults before starting the UART at 250000 baud.
Board revisions that move the UART must pass explicit `txPin` / `rxPin` arguments to
`SimGateway::setup()`, and their tests must exercise the same configured pins.

---

## Testing Contract

SimGateway tests run on a real RP2040 device. They simulate PanelBridge-originated UART traffic
with synthetic HID-frame bytes; they do not read local ADC pins, I2C ADCs, CAN, or physical
cockpit controls.

Parser tests may call a `SIMGATEWAY_TEST`-only byte-feed helper to inject bytes directly into
the HID demultiplexer. UART integration tests may instead jumper `Serial1` TX to `Serial1` RX and
write the same synthetic frames through the hardware UART before calling `SimGateway::loop()`.
The default loopback jumper is RP2040 `GP0 -> GP1`, matching the production UART pins above.

The values under test are already-normalised `uint16_t` payloads carried in HID frames, exactly
as PanelBridge would send after receiving CAN EVTs from PanelGroup nodes. ADC and sensor
normalisation belongs to PanelGroup/AnalogInput tests, not SimGateway tests.

**The calibration transform is tested by direct call, never through `dispatch()`.** In
`SIMGATEWAY_TEST` builds the HID setters are no-op stubs, so nothing written to the report is
observable and the capture globals record the *pre-transform* value read off the wire. The
transform is therefore a free function taking its inputs as arguments — the same shape as the
status-LED state machine, and for the same reason.

**Link health is tested as decision logic.** `cdc_relay` injects the CDC FIFO's free space
(`cdcSetRoomForTest`) to reach the drop path; `link_health` drives the recovery ladder, FAULT
self-clear and counters with injected time (`linkStepForTest`, `diagUartErrorsForTest`). No test
detaches USB, restarts the UART, or reboots. Those are bench checks.

**`SIMGATEWAY_TEST` builds must never write flash.** The EEPROM path is stubbed out to a RAM
clear, so tests start from a known uncalibrated state and no test run erases a sector.
Persistence consequently cannot be covered by a `SIMGATEWAY_TEST` scenario; it needs a
production-mode environment (`test_axis_cal_persist`), which runs in two phases separated by a
power cycle and restores the sector when it finishes.

---

## Sketch Structure

The sketch declares `HIDAxis`, `HIDButton`, and `HIDHatSwitch` registrars and calls
`setup()`/`loop()`. All TinyUSB HID setup, USB identity, baud rate, and value scaling are owned
by the library — not visible in the sketch.

```cpp
#include <SimGateway.h>
#include <HIDControls.h>

// HID declarations — only these are needed; DCS-BIOS routing is automatic
OpenSkyhawk::HIDAxis roll    (CTRL_ROLL,     0);
OpenSkyhawk::HIDAxis pitch   (CTRL_PITCH,    1);
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0);

void setup() {
    SimGateway::setup(Serial1);
    // SimGateway::setup() owns: USB identity, GP0/GP1 pin config,
    // Serial1.begin(250000), TinyUSB HID enumeration
}

void loop() {
    SimGateway::loop();   // relay USB CDC ↔ UART; intercept HID frames; send one HID report
}
```

See `examples/simgateway.md` for the full sketch with data flow annotations.
