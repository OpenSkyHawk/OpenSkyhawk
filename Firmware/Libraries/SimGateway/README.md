# SimGateway

RP2040 USB HID + DCS-BIOS relay gateway library for OpenSkyhawk.

Owns:
- **USB CDC ↔ UART relay** — forwards the export stream to PanelBridge as-is, and PanelBridge's output to the PC line by line; a line that can't be written whole is dropped and counted, never truncated (#355)
- **HID frame demultiplexer** — intercepts `0xAA 0x55` binary frames on the UART RX path; dispatches `controlId` + `value` to registered `HIDAxis` / `HIDButton` / `HIDHatSwitch` objects
- **HID report batching** — one TinyUSB HID report sent per `loop()` iteration after draining all UART bytes
- **Status-LED state machine** — drives the two board LEDs (GREEN = GP2, RED = GP3, active-high) with a non-blocking `millis()` animator ticked from `loop()`
- **Calibration transport** — the client's axis-calibration frames on USB CDC, consumed before the relay; calibration stored in flash (see *Axis calibration*)
- **Link health and recovery** — per-bit UART error counters, FAULT self-clear, the USB recovery ladder, the hardware watchdog and the `GET_DIAG` reply (#355)
- **Firmware uploader** — updates boards over the bus and the Gateway's own PanelBridge over USART2 (`Uploader.h`, #366/#367)

Does **not** run DCS-BIOS. Does **not** parse DCS-BIOS addresses. The relay knows no CAN; the built-in
uploader (#366, #367) drives board bootloaders over CAN through PanelBridge, and reads `_NODE_STATUS` /
`_BOOT_RSP` lines to do it.

Platform: **RP2040 only.**

## Dependencies

- `HIDControls` — CTRL_* controlId constants (platform-agnostic, shared with STM32)
- `FirmwareVersion`, `FirmwareImage`, `BootProtocol`, `NodeStatus` — the version in `HELLO_ACK`, and the
  uploader's image checks, bootloader frames and roster parsing
- `earlephilhower/arduino-pico` platform — TinyUSB CDC, HardwareSerial, RP2040 Arduino core
- `adafruit/Adafruit TinyUSB Library` — custom HID descriptor with 8 axes, 128 buttons, 4 hat switches

## Usage

```cpp
#include <SimGateway.h>
#include <HIDControls.h>

// Declare HID controls at file scope — constructed before setup()
OpenSkyhawk::HIDAxis  roll(CTRL_ROLL,    0);
OpenSkyhawk::HIDAxis  pitch(CTRL_PITCH,  1);
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0);

void setup() {
    SimGateway::setup(Serial1); // USB identity + UART + TinyUSB HID enumeration
}

void loop() {
    SimGateway::loop(); // relay + dispatch + Send()
}
```

## USB identity

| Field | Value |
|---|---|
| Manufacturer | OpenSkyhawk |
| Product | A-4E Skyhawk |
| VID | 0x2E8A (Raspberry Pi) |
| PID | 0x4134 |
| CDC port (iInterface) | A-4E Skyhawk DCS-BIOS |

The `Product` string names the HID joystick; the CDC interface carries its own `iInterface`
string so the serial port is identifiable by name (not just VID/PID + CDC class).

## UART pins

The standard PanelBridge link uses RP2040 `Serial1` / UART0:

| Signal | RP2040 pin | PanelBridge STM32 pin |
|---|---|---|
| SimGateway TX -> PanelBridge RX | GP0 | PA3 / UART2 RX |
| SimGateway RX <- PanelBridge TX | GP1 | PA2 / UART2 TX |

`SimGateway::setup(Serial1)` configures GP0/GP1 before starting the UART. Board revisions with
different wiring can pass explicit `txPin` / `rxPin` arguments to `setup()`.

## HID frame wire format

Sent by PanelBridge over UART to SimGateway for HID axis/button updates:

```
Byte 0:   0xAA  (magic — bit 7 set, cannot appear in DCS-BIOS ASCII stream)
Byte 1:   0x55  (magic)
Byte 2:   controlId[7:0]
Byte 3:   controlId[15:8]
Byte 4:   value[7:0]
Byte 5:   value[15:8]
```

controlId ranges (from `HIDControls.h`):
- `0x0010–0x001F` — HID axes → `HIDAxis` dispatch → calibration + offset → HID axis report
- `0x0020–0x002F` — HID hat switches → `HIDHatSwitch` dispatch → HID hat nibble
- `0x0030–0x00AF` — HID buttons → `HIDButton` dispatch → HID button bit
- `0x00B0–0x00FF` — reserved HID expansion slots

Axis value mapping: 0–65535 (from PanelGroup) → calibration → ±32767 (`value − 32768`), both
applied internally by `HIDAxis::dispatch()`.

## Axis calibration

A hall stick spans only part of the ADC range — 57–62% on the bench rig — so an uncalibrated
axis reaches roughly ±20k instead of ±32767, and its neutral rarely sits at the midpoint of that
span. `AxisCal.h` holds the fix: three captured points per axis (`min`, `centre`, `max`) and two
linear segments, so centre lands on 32768 exactly however asymmetric the travel is. A single
`min..max` stretch fixes the range but leaves the stick resting off-centre; on the bench rig
that was +4669.

Calibration is stored in flash (`CalBlob`, one 4 KB EEPROM sector) and loaded once by
`SimGateway::setup()`. It is **never a constructor argument** — endpoints are per-unit, so
baking them in would mean a reflash per build. Sketch declarations are identical calibrated or
not, and an axis with no stored calibration passes values through unchanged.

`AxisCal.h`/`.cpp` are pure — no EEPROM, no `Serial`, no `millis()`, no globals — so the
transform is callable directly from a test sketch. That matters because the HID setters are
no-op stubs in `SIMGATEWAY_TEST` builds, which makes anything reachable only through
`dispatch()` unobservable.

Writing calibration at runtime is a separate concern; see `FirmwarePlan/07-simgateway-api.md`.

## Status LEDs

Two board-mounted LEDs on the `Gateway_Bridge` board — **GREEN = GP2, RED = GP3**, active-high —
report the gateway state. `SimGateway::setup()` configures the pins and `SimGateway::loop()` ticks
a non-blocking `millis()` animator; the sketch needs no LED code. The RP2040 module's onboard
WS2812 is not used.

| State | Trigger | LED |
|---|---|---|
| `FAULT` | uart0 RX hardware error (RSR overrun/framing/parity/break) | red fast (4 Hz) |
| `NO_HOST` | USB not enumerated, or unplugged after a mount | red solid |
| `STREAMING` | DCS-BIOS bytes from the PC within last ~500 ms | green solid |
| `USB_IDLE` | USB mounted, no recent DCS data | green slow (1 Hz) |
| `INIT` | booted, pre-USB-mount (brief) | red slow |

Priority is highest-first (`FAULT` wins). `FAULT` is read from the `uart0` PL011 `RSR` register
(the arduino-pico `SerialUART` surfaces no error flags), latched for ≥2 s, and clears when clean
PanelBridge data resumes after the fault — or, if still latched 3 s after it was set, the UART is
restarted and the latch cleared (#355); errors that persist latch it again. This is a SimGateway-local
engine; it shares the animation vocabulary with `STM32Board` (PB14/PB15) but not the code.

## API

| Function | Description |
|---|---|
| `SimGateway::setup(uart)` | Init USB identity, TinyUSB HID, UART, status LEDs |
| `SimGateway::loop()` | Relay CDC↔UART, parse HID frames, Send(), advance an upload, tick status LEDs, USB recovery. The first call arms a 3 s hardware watchdog: call `loop()` at least that often |
| `SimGateway::statusLedBegin()` | Configure GP2/GP3 outputs (called by `setup()`) |
| `SimGateway::statusTick()` | Advance the status-LED state machine (called by `loop()`) |
| `SimGateway::diagnostics()` | The live link-health counters (`GatewayDiag`) |
| `SimGateway::calibration()` | The axis calibration in use (`CalBlob`) |

See `SimGateway.h` for full Doxygen documentation.

## Platform support

Verified against all target platforms with the `hid_stress` test sketch. Production descriptor
(8 axes / 128 buttons / 4 hats / **34 bytes**) is within all platform limits.

| Platform | Axes | Buttons | Hats | Notes |
|---|---|---|---|---|
| DCS / DirectInput (Windows) | **8** ✓ | **128** ✓ | **4** ✓ | Full DIJOYSTATE2 match. Hats as `JOY_BTN_POVn_*` bindings. |
| Chrome Gamepad API (macOS) | 8 + 1 hat-as-axis | **32** | — | IOKit inserts hat 0 as axis at usage 0x39 position. 32-button cap is not a concern for DCS. |
| Windows joy.cpl | 8 | 128 | 4 | Same as DirectInput. |

**TinyUSB hard limit:** `CFG_TUD_HID_EP_BUFSIZE = 64` bytes, hardcoded in
`tusb_config_rp2040.h`. Build-flag overrides are silently ignored. `sendReport()` returns
`false` without sending if `sizeof(report) > 64`. Never expand the production report above
64 bytes.

## Testing

SimGateway tests run on an RP2040 device and inject synthetic HID-frame bytes, as if those bytes
arrived from PanelBridge over UART. They do **not** read local ADC pins, I2C ADCs, sensors, CAN,
or physical cockpit controls.

Parser/dispatch tests may use `SIMGATEWAY_TEST` hooks such as `feedByte()` to inject bytes
directly into the demultiplexer. UART integration tests may instead jumper `Serial1` TX to
`Serial1` RX and write the same frames through the hardware UART before calling
`SimGateway::loop()`. The default loopback jumper is GP0 -> GP1.

`cdc_forwarding` verifies the relay side of the parser contract: ordinary UART bytes are forwarded
to USB CDC in order, valid HID frames are consumed and not forwarded, and `0xAA` followed by a
non-`0x55` byte marks its line corrupt: that line is dropped and the next one forwards.

Test builds define `SIMGATEWAY_TEST`, activating no-op stubs for all TinyUSB HID helpers
directly inside `SimGateway.cpp`. No external shim file is needed. Production builds compile
the full `Adafruit_USBD_HID` descriptor and report struct in the same file.

`hid_parser`, `parser_resync` and `hid_dispatch` cover the HID frame parser, its resync after a
bad magic byte, and dispatch to the registered objects. `cdc_relay` covers the never-blocking
host-bound relay (#355): whole lines or nothing, corrupt and overlong lines dropped and counted.
`link_health` covers the USB recovery ladder, FAULT self-clear, the UART error counters, the CDC
stall detector and the `GET_DIAG` reply.

The `axis_cal_*` tests cover axis calibration: `axis_cal_transform` (the two-segment map and blob
checksum), `axis_cal_framing` (a frame is consumed only on magic + LEN + CRC; anything else reaches
PanelBridge byte-for-byte), `axis_cal_session` (session commands, replies checked off CDC) and
`axis_cal_raw_stream` (RAW streaming of the axis under calibration). `axis_cal_persist` builds in
production mode and is the one test that writes real flash, across a power cycle.

`hid_enumeration` builds in production mode with the real TinyUSB HID stack: flash it and check the
descriptor in joy.cpl or DCS. `hid_stress` uses its own beyond-production descriptor (16 axes,
128 buttons, 8 hats) without the SimGateway library, to see what each platform surfaces.

`uart_loopback` verifies the same parser and forwarding behavior through the configured physical
UART pins by writing bytes to `Serial1` TX and reading them back through `Serial1` RX.

`led_state` verifies the status-LED state machine — state→colour/animation mapping, priority
precedence, the STREAMING↔USB_IDLE 500 ms decay, the INIT→NO_HOST 2 s window, FAULT latch/recovery
(including the silent-bus hold), and animation phase timing — entirely through `SIMGATEWAY_TEST`
hooks, with no GPIO, TinyUSB, or register access.

ADC and sensor normalisation belongs in PanelGroup / AnalogInput tests. SimGateway receives only
already-normalised `uint16_t` HID payloads.

### Manual: USB CDC interface name (Mac + Windows)

Verifies the CDC `iInterface` string descriptor ([#85](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/85)).
No special test sketch — flash the base production template and inspect the host.

1. **Build & flash** the base template. `pio run -t upload` resets the board into its bootloader
   by itself (the 1200-baud touch, handled by the Adafruit TinyUSB library). Close anything
   holding the CDC port (client, serial monitor) first, or the reset can't reach the board.
   ```
   cd Firmware/Templates/SimGateway
   pio run -t upload        # board = rpipico by default; set `board` to match your module
   ```
2. Plug the RP2040 into the host; wait ~1 s for enumeration (TinyUSB drops HID reports until
   enumerated — see FirmwarePlan §09).
3. **macOS** — expect a CDC interface named `A-4E Skyhawk DCS-BIOS`:
   ```
   system_profiler SPUSBDataType | grep -A12 "A-4E Skyhawk"
   ls /dev/cu.usbmodem*                       # the serial node
   # alt: ioreg -p IOUSB -l -w0 | grep -i "DCS-BIOS"
   ```
4. **Windows** — Device Manager → Ports (COM & LPT) → the OpenSkyhawk port → Properties →
   Details → **Bus reported device description** → expect `A-4E Skyhawk DCS-BIOS`. Confirm the
   joystick still shows **A-4E Skyhawk** in `joy.cpl`.
5. Record the result below.

| Date | Host OS | Tool used | Observed CDC name | HID name (joy.cpl) | Pass/Fail | Notes |
|------|---------|-----------|-------------------|--------------------|-----------|-------|
|      |         |           |                   |                    |           |       |

Expected: CDC interface = `A-4E Skyhawk DCS-BIOS`; HID device = `A-4E Skyhawk`, on both OSes.
