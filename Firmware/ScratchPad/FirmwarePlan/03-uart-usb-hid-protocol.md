# 03 — UART / USB-HID Protocol

**Owns:** UART multiplexing scheme, HID frame wire format, parser resync algorithm,
USB identity and Joystick configuration.
**Does not own:** HID axis/button class declarations (→ 07), CAN frame formats (→ 02),
DCS-BIOS routing logic (→ 04), boot sequencing (→ 09).

---

## UART Link (SimGateway ↔ PanelBridge)

**Baud rate:** 250 000 (matches DCS-BIOS protocol rate — no buffering mismatch).
**Physical pins:** See `08-hardware-firmware-contracts.md#uart-pin-assignments`.

The single UART carries two distinct byte streams in the **PanelBridge → SimGateway** direction:

1. **DCS-BIOS text commands** — raw ASCII from `sendDcsBiosMessage()`, e.g. `"ARM_MASTER 1\n"`.
   Forwarded by SimGateway to USB CDC verbatim.
2. **HID frames** — binary frames with a non-ASCII `HID_MAGIC` header, carrying
   `controlId < 0x8000` for HID dispatch.

In the **SimGateway → PanelBridge** direction, the UART carries only the raw DCS-BIOS binary
export stream forwarded from USB CDC.

---

## HID Frame Wire Format

Fixed 6 bytes, little-endian:

| Byte offset | Field | Value / Notes |
|-------------|-------|---------------|
| 0 | Magic byte 0 | `0xAA` |
| 1 | Magic byte 1 | `0x55` |
| 2–3 | `controlId` | uint16, little-endian |
| 4–5 | `value` | uint16, little-endian |

**Magic:** `0xAA 0x55` — both bytes have bit 7 set. `sendDcsBiosMessage()` output on the same
UART direction is pure printable ASCII + LF (all bytes ≤ 0x7F). **No collision is possible.**

**Endianness:** little-endian — matches native struct layout on both STM32 and RP2040.

**CRC/checksum:** none. Fixed 6-byte length plus unique non-ASCII magic is sufficient for this
short dedicated UART link. A checksum would add latency on HID-critical joystick updates.

---

## Parser Resync (SimGateway)

SimGateway demultiplexes the incoming UART byte stream as follows:

- Any byte ≤ `0x7F` → DCS-BIOS byte; forward to USB CDC immediately.
- On byte `0xAA`: peek at the next byte.
  - If it is `0x55` → HID frame: read 4 more bytes, parse `controlId` + `value`, dispatch.
  - If it is not `0x55` → not a valid frame header: forward `0xAA` + the mismatched byte to
    USB CDC and resume scanning.

This ensures any framing error is self-healing at the next byte boundary.

---

## USB Identity (SimGateway)

Set at runtime in `setup()` before the TinyUSB HID device begins.
`-DUSE_TINYUSB` replaces the default Pico USB stack with TinyUSB — use `TinyUSBDevice.*`
not `USB.*` (the latter belongs to the default stack):

```cpp
TinyUSBDevice.setID(0x2E8A, 0x4134);                    // VID = Raspberry Pi; PID = A-4E Skyhawk
TinyUSBDevice.setManufacturerDescriptor("OpenSkyhawk");
TinyUSBDevice.setProductDescriptor("A-4E Skyhawk");
```

---

## HID Backend

**Selected: Adafruit TinyUSB Library** (`adafruit/Adafruit TinyUSB Library`) with a custom HID
descriptor embedded in `SimGateway.cpp`.

| Capability | Value |
|------------|-------|
| Axes | 8 (X, Y, Z, Rx, Ry, Rz, Slider, Dial — 16-bit signed ±32767) |
| Buttons | 128 (bit-packed, 1-bit each) |
| Hat switches | 4 (4-bit nibble each; 0xF = centered/null) |
| Report size | 34 bytes (16 buttons + 2 hats + 16 axes) |
| Composite device | CDC (Serial) + HID on the same USB port |
| Platform | earlephilhower/arduino-pico, PlatformIO compatible |

This covers all 7 OpenSkyhawk axes (Roll, Pitch, Throttle, Rudder, Brake L, Brake R, Zoom)
with one spare, plus 128 buttons and 4 hat switches for future controls.

The HID descriptor and report struct are private to `SimGateway.cpp`. In production builds
(`#ifndef SIMGATEWAY_TEST`) the full Adafruit_TinyUSB setup and `Adafruit_USBD_HID` instance
live inside an anonymous namespace. In test builds the same internal helpers are no-ops,
keeping all 5 test environments free of USB enumeration side effects.

`OsJoystick.send()` is called once after draining all HID frames each loop iteration to keep
the HID report rate predictable and avoid redundant USB packets.

> **Validated 2026-06-11** via `hid_stress` test sketch (VID 0x2E8A / PID 0x4135) on macOS
> Chrome Gamepad API and DCS World Controls (Windows DirectInput):
>
> | Platform | Axes | Buttons | Hats | Result |
> |---|---|---|---|---|
> | DCS World (DirectInput) | 8 ✓ | 128 ✓ | 4 (as `JOY_BTN_POVn_*`) ✓ | Full DIJOYSTATE2 match |
> | Chrome Gamepad API (macOS) | 8 + hat as axis | 32 (API cap) | — | Not a concern; DCS uses DirectInput |
>
> Descriptor layout is frozen. See `07-simgateway-api.md#hid-platform-compatibility` for
> full platform limits and the TinyUSB `CFG_TUD_HID_EP_BUFSIZE=64` constraint.
