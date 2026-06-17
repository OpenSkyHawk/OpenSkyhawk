# SimGateway

The SimGateway is the one device the PC plugs into. It's an off-the-shelf RP2040 module that
presents a single composite USB device to DCS and relays everything to the rest of the
cockpit over a UART. This page covers *how* it works. For *why* a separate gateway exists at
all — the STM32 USB failure that forced it — see
[Design Decisions D1](design-decisions.md#d1-simgateway-exists-because-stm32-usb-cant-take-the-load).

## Hardware

- **MCU:** an off-the-shelf RP2040 module (e.g. Raspberry Pi Pico). No custom PCB.
- **USB:** the single connection to the PC.
- **UART:** RP2040 `Serial1` / UART0 — **GP0** (TX → PanelBridge RX/PA3) and **GP1**
  (RX ← PanelBridge TX/PA2), at **250000 baud**. Both sides are 3.3 V; no level shifter.

`SimGateway::setup(Serial1)` configures the pins, starts the UART at 250000, sets the USB
identity, and enumerates the TinyUSB HID device. The baud rate matches the DCS-BIOS protocol
rate exactly, so there's no buffering mismatch on the relay.

## USB composite device

SimGateway enumerates as a **composite CDC + HID device** on one USB port:

- **CDC (serial)** carries the DCS-BIOS stream, both directions.
- **HID (joystick)** presents flight-control axes and buttons to the PC.

USB identity is set at runtime before the HID device starts (TinyUSB replaces the default
Pico USB stack):

```cpp
TinyUSBDevice.setID(0x2E8A, 0x4134);   // VID = Raspberry Pi, PID = A-4E Skyhawk
TinyUSBDevice.setManufacturerDescriptor("OpenSkyhawk");
TinyUSBDevice.setProductDescriptor("A-4E Skyhawk");
Serial.setStringDescriptor("A-4E Skyhawk DCS-BIOS"); // CDC interface name (iInterface)
```

The product string names the HID joystick, but the CDC serial interface advertises its own
name (`iInterface`) — `A-4E Skyhawk DCS-BIOS` — so host tooling such as OpenSkyhawk Client can
match the port by name, not just VID/PID + CDC class.

## HID device profile

The HID descriptor is built on the Adafruit TinyUSB library and frozen:

| Capability | Value |
|------------|-------|
| Axes | 8 — X, Y, Z, Rx, Ry, Rz, Slider, Dial (16-bit signed, ±32767) |
| Buttons | 128 (1 bit each) |
| Hat switches | 4 (4-bit nibble each; `0xF` = centered) |
| Report size | **34 bytes** |

This covers all 7 planned OpenSkyhawk axes (Roll, Pitch, Throttle, Rudder, Brake L, Brake R,
Zoom) with one spare, plus headroom on buttons and hats. Validated against DCS World
(DirectInput) as a full `DIJOYSTATE2` match — 8 axes, 128 buttons, 4 POV hats, no limits
exceeded.

!!! warning "Never grow the HID report past 64 bytes"
    TinyUSB on the RP2040 hardcodes `CFG_TUD_HID_EP_BUFSIZE = 64`; build-flag overrides are
    silently ignored. A report over 64 bytes makes `sendReport()` return `false` and drop
    silently — no error, no crash. The production report is 34 bytes; keep it well under 64.

## The relay contract

This is the heart of SimGateway, and it's deliberately dumb:

- Every byte from USB CDC is forwarded to the UART **immediately**.
- Every UART byte that is **not** part of a HID frame is forwarded to USB CDC **immediately**.
- SimGateway never buffers, parses, or delays DCS-BIOS bytes, and never runs the DCS-BIOS
  library. PanelBridge therefore sees the stream with USB-CDC latency only.

The two streams share one UART because they can't collide: DCS-BIOS text is pure printable
ASCII + LF (every byte ≤ `0x7F`), while HID frames start with the magic bytes `0xAA 0x55`
(both have bit 7 set).

This "dumb relay" is also why **node-status reporting** ([#86](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/86))
needs no SimGateway change: PanelBridge reports connected PanelGroup nodes to the host over
DCS-BIOS itself — `_NODE_STATUS` ASCII command messages downstream, and a roster request as a
DCS-BIOS export write to reserved address `0x86FE` upstream. Both ride the existing streams and
transit SimGateway transparently; PanelBridge owns the feature.

## controlId routing logic

Incoming HID frames (PanelBridge → SimGateway) are fixed 6-byte little-endian:

| Bytes | Field |
|-------|-------|
| 0–1 | Magic `0xAA 0x55` |
| 2–3 | `controlId` (uint16 LE) |
| 4–5 | `value` (uint16 LE) |

SimGateway demultiplexes the UART byte stream:

- Byte ≤ `0x7F` → DCS-BIOS byte → forward to USB CDC.
- Byte `0xAA` followed by `0x55` → HID frame: read 4 more bytes, dispatch by `controlId`.
- Byte `0xAA` *not* followed by `0x55` → not a frame: forward both bytes and resume scanning
  (self-healing resync at the next byte boundary).

HID controls are owned entirely here — PanelBridge wraps them but never interprets them. The
`controlId` ranges SimGateway dispatches:

| Range | Type | Handler |
|-------|------|---------|
| `0x0010`–`0x001F` | Axes | `HIDAxis` |
| `0x0020`–`0x002F` | Hat switches | `HIDHatSwitch` |
| `0x0030`–`0x00AF` | Buttons | `HIDButton` |

`HIDAxis` maps the incoming 0–65535 value to ±32767 internally; `HIDButton` treats
`value != 0` as pressed; `HIDHatSwitch` reads a direction nibble (0 = centered, 1–8 = N…NW).

## Dispatch and startup behaviour

Each `SimGateway::loop()` iteration drains all pending HID frames, applies them to the axis /
button / hat objects, and then calls `OsJoystick.send()` **once** to flush the HID report.
Sending once per drain cycle — not once per frame — keeps the HID report rate predictable and
avoids redundant USB packets.

At startup, `SimGateway::setup()` owns all of it: USB identity, GP0/GP1 pin configuration,
`Serial1.begin(250000)`, and TinyUSB HID enumeration. The sketch only declares its `HIDAxis`
/ `HIDButton` / `HIDHatSwitch` objects and calls `setup()` / `loop()` — DCS-BIOS relay is
automatic and needs no declarations.

## Status LEDs

The gateway carries two board-mounted status LEDs on the `Gateway_Bridge` board — **GREEN on
GP2, RED on GP3** — so you can read its USB / data / fault state at a glance without a host. They
are active-high (HIGH = on) and driven by a non-blocking `millis()` animator ticked from
`SimGateway::loop()`; the RP2040 module's onboard WS2812 is not used. State priority runs highest
to lowest — the topmost active row wins:

| Priority | State | What it means | LED |
|---|---|---|---|
| 1 (highest) | `FAULT` | A UART hardware error on the PanelBridge link (overrun / framing / parity) | **Red, fast blink** (4 Hz) |
| 2 | `NO_HOST` | USB not enumerated — no PC, or the cable was unplugged | **Red, solid** |
| 3 | `STREAMING` | DCS-BIOS data flowing from the PC (seen within the last ~500 ms) | **Green, solid** |
| 4 | `USB_IDLE` | USB connected, but no recent DCS-BIOS data | **Green, slow blink** (1 Hz) |
| 5 (lowest) | `INIT` | Just booted, USB not yet mounted (brief) | **Red, slow blink** |

!!! note "When the fault light clears"
    `FAULT` is latched, not momentary: it stays on for at least ~2 s (so a single glitch is
    visible) and then clears **only once clean PanelBridge data resumes** on the UART — an
    error-free byte received *after* the fault. A fault on an otherwise-silent link therefore
    stays lit until traffic returns error-free, rather than timing out on its own. The error is
    read straight from the RP2040's `uart0` PL011 error register (`RSR`), because the
    arduino-pico `SerialUART` driver doesn't surface overrun/framing flags any other way.

!!! warning "Active-high, separate power domain"
    Both LEDs are active-high (HIGH = lit) and sit on the RP2040 module's own 3V3 LDO, isolated
    from the STM32 rail — so they reflect the *gateway's* state even if the STM32 side is
    unpowered or held in reset. The two `BRIDGE` LEDs next to them (PB14/PB15) are driven
    separately by the STM32 firmware.
