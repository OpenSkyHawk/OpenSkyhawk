# 03 — UART / USB-HID Protocol

**Owns:** UART multiplexing scheme, HID frame wire format, parser resync algorithm,
USB identity and HID device configuration, the USB CDC calibration protocol.
**Does not own:** HID axis/button class declarations (→ 07), calibration storage and the
transform itself (→ 07), CAN frame formats (→ 02), DCS-BIOS routing logic (→ 04),
boot sequencing (→ 09).

---

## UART Link (SimGateway ↔ PanelBridge)

**Baud rate:** 250 000 (matches DCS-BIOS protocol rate — no buffering mismatch).
**Physical pins:** See `08-hardware-firmware-contracts.md#uart-pin-assignments`.

The single UART carries two distinct byte streams in the **PanelBridge → SimGateway** direction:

1. **DCS-BIOS text commands** — raw ASCII from `sendDcsBiosMessage()`, e.g. `"ARM_MASTER 1\n"`.
   Forwarded by SimGateway to USB CDC verbatim, one whole line at a time (see *Host-bound Relay*).
2. **HID frames** — binary frames with a non-ASCII `HID_MAGIC` header, carrying
   `controlId < 0x8000` for HID dispatch.

In the **SimGateway → PanelBridge** direction, the UART carries only the raw DCS-BIOS binary
export stream forwarded from USB CDC. The host's requests to the bridge ride inside it as writes to
reserved export addresses — the roster request (`0xFF00`), the bridge's enter-bootloader request
(`0xFF02`, #367) and the bootloader relay (`0xFF10`–`0xFF4F`, #365) — see `04-dcs-bios-integration.md`.
While the Gateway's uploader updates the bridge itself (#367), the UART instead carries the bridge
bootloader's own `0xAA` frames, both ways (`12-bootloader.md`); nothing else crosses it then. Their replies come back as ordinary DCS-BIOS text lines
(`_NODE_STATUS`, `_BOOT_RSP`).

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

> The calibration protocol below *does* carry a CRC, which is not a contradiction. This frame is
> fixed-length on a two-node dedicated link with a provably disjoint alphabet, and it sits on the
> joystick latency path. Calibration frames are variable-length, cross the USB CDC boundary where
> they interleave with a third-party stream, and a commit is a deliberate once-per-session action
> nowhere near that path. Different risk, different answer.

---

## Parser Resync (SimGateway)

SimGateway demultiplexes the incoming UART byte stream as follows:

- Any byte other than `0xAA` → DCS-BIOS byte; append to the host-bound line.
- On byte `0xAA`: peek at the next byte.
  - If it is `0x55` → HID frame: read 4 more bytes, parse `controlId` + `value`, dispatch.
  - If it is not `0x55` → not a valid frame header: append `0xAA` + the mismatched byte to the
    line and resume scanning. `0xAA` is not ASCII, so that line is dropped as corrupt.

This ensures any framing error is self-healing at the next byte boundary.

**Consequence for anything else on the CDC stream:** since #355 the resync path no longer reaches
the host. The relay forwards only whole lines of printable ASCII, so the only high-bit bytes the
gateway emits outbound are its own calibration frames. A host-side demultiplexer must still
validate a `0xAA`-led candidate before acting (older firmware did emit a bare `0xAA`), see the
calibration protocol's CRC requirement below.

---

## Host-bound Relay (SimGateway → USB CDC)

Issue #355. Everything PanelBridge sends the host is a newline-framed DCS-BIOS message, so the
relay is line-granular: SimGateway assembles each line and writes it **whole** when the CDC FIFO
has room for all of it, or drops it and counts the drop. It never truncates a line, and it never
blocks: Adafruit TinyUSB's `write()` loops for as long as the host holds DTR but stops reading,
which would stall the whole gateway.

- A line is printable ASCII (`0x20`–`0x7E`) ending in `\n`, at most `RELAY_LINE_MAX` (128) bytes.
  Anything else — a control byte, `\r`, a high-bit byte, or a longer run — is dropped, not relayed.
- Calibration frames follow the same whole-or-drop rule, and since lines are buffered a frame always
  goes out *between* lines, never inside one.
- A host that stops reading loses lines; nothing else changes. The drops show in `DIAG`.

---

## USB Identity (SimGateway)

Set at runtime in `setup()` before the TinyUSB HID device begins.
`-DUSE_TINYUSB` replaces the default Pico USB stack with TinyUSB — use `TinyUSBDevice.*`
not `USB.*` (the latter belongs to the default stack):

```cpp
TinyUSBDevice.setID(0x2E8A, 0x4134);                    // VID = Raspberry Pi; PID = A-4E Skyhawk
TinyUSBDevice.setManufacturerDescriptor("OpenSkyhawk");
TinyUSBDevice.setProductDescriptor("A-4E Skyhawk");
Serial.setStringDescriptor("A-4E Skyhawk DCS-BIOS");    // CDC iInterface — names the serial port
```

The device-level `iProduct` ("A-4E Skyhawk") names the HID joystick, but the CDC serial
interface needs its own `iInterface` string to advertise a friendly port name (otherwise it
falls back to the library default "TinyUSB Serial"). Set it **after** `Serial.begin()`, which
initialises that default. This lets OpenSkyhawk Client identify the port by name, not just
VID/PID + CDC class.

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

The HID report is sent once after draining all HID frames each loop iteration to keep
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

---

## Calibration Protocol (USB CDC)

**Owner:** SimGateway. **Peer:** SkyHawkClient. **Issue:** #251.
**Storage and transform contract:** `07-simgateway-api.md#axis-calibration`.

Runtime axis calibration needs two things the joystick report cannot provide: raw
(pre-transform) values, which the transform clamps away exactly at the endpoints capture cares
about, and a way to write endpoints back to the device. Both ride the existing USB CDC
interface. No new USB interface is enumerated, and no HID slot is spent.

### The one rule everything else follows from

**The client stops relaying the DCS export stream before *any* exchange** — a 10 ms read as
much as a whole calibration session — and resumes afterwards.

That single act removes the hard problem. The gateway is not hunting for its frames inside a
live binary stream, because there is no stream: no collision risk, no length-and-CRC needed for
disambiguation, no buffer-don't-consume rule, no 7-bit encoding tax.

The reverse direction is **not** taken over. UART→CDC forwarding continues unchanged so
`_NODE_STATUS` and DCS-BIOS text keep flowing during a session; calibration frames are injected
alongside them. Health data must not stop just because someone opened a dialog.

Consequences worth stating plainly:

- The relay is never "hijacked" in both directions, despite that shorthand appearing in early
  design notes.
- An expired or abandoned session degrades to exactly normal operation. The only thing that
  stops is raw streaming.
- Because outbound is shared, the client must de-multiplex on the **raw byte chunk, before line
  assembly**. Since #355 the gateway buffers lines and never injects a frame mid-line, but older
  firmware did; de-multiplexing first keeps that case harmless too — removing the frame's bytes
  rejoins the interrupted line correctly.

### Frame format

Identical envelope in both directions.

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | `MAGIC` — `AA 53 4B 43` (`\xAA` `S` `K` `C`) |
| 4 | 1 | `TYPE` |
| 5 | 1 | `SEQ` — echoed in the response; on unsolicited `RAW`, a free-running counter (see below) |
| 6 | 2 | `LEN` — payload length, uint16 little-endian. Must match the type exactly (see below) |
| 8 | `LEN` | `PAYLOAD` |
| 8+`LEN` | 2 | `CRC16` — CRC-16/CCITT-FALSE over `TYPE`‖`SEQ`‖`LEN`‖`PAYLOAD` |

10 bytes of envelope. All multi-byte fields little-endian. Payloads are binary, not text: the
device is the constrained end and the client has the budget to unpack.

**`0xAA` leads all non-DCS data** on this link by project convention, matching the HID frame
magic. It is not collision-proof on the outbound direction — firmware before #355 let the parser
resync path emit a bare `0xAA` — so the **CRC is mandatory, and a client must re-emit CRC-failed candidate bytes
into its line assembler rather than dropping them.** Otherwise a corrupted DCS-BIOS line would be
silently swallowed. A false frame requires `AA 53 4B 43` to appear consecutively *and* the CRC to
pass.

### `LEN` must match the type exactly

**`LEN` is read before the CRC can be checked, so on a false frame it is noise.** A stray
`AA 53 4B 43` in DCS-BIOS text can decode a `LEN` of nearly 65535; a receiver that waits for
that many bytes before validating stalls its line assembly and makes its memory use depend on
whatever the noise said. That is not hypothetical here — the whole reason this section admits
`0xAA` can appear outbound is the parser resync path above.

So `LEN` is not merely bounded, it is **fixed by `TYPE`**: every type has exactly one legal
length, listed in the payload table. Anything else is not a frame. There are no
variable-length types and therefore no exception to this rule. The largest payload the device
**sends** is **82** (`CAL_DATA`), so the client's buffering is bounded by that. The one larger type,
`UPL_DATA` (260, #366), travels **client → device only** — the device's inbound buffer is sized for it
(`CAL_MAX_INBOUND_FRAME`, 270); nothing larger than 82 ever comes back.

A receiver checks `LEN` against the type **before buffering the payload**. A mismatch means the
candidate was never a frame: emit the consumed bytes into the line assembler and resume scanning
from the byte after the magic, exactly as for a CRC failure.

This is strictly stronger than a bare upper bound. A bare `LEN ≤ 82` rule would still let a
false frame consume 82 bytes before dying at the CRC; an exact-match rule usually rejects it on
the first comparison.

### `SEQ`

On a request, the client picks `SEQ` and the device echoes it in the response, which is what lets
a client match replies to requests.

`RAW` has no request to echo. It carries a **free-running per-session counter**, starting at 0 on
`SESSION_ACK` and wrapping at 256. This is deliberately not the selecting `SESSION_OPEN` /
`STREAM_SELECT` sequence, which would only duplicate the `idx` field `RAW` already carries.

A counter adds something neither of those does: `RAW` is explicitly droppable — the device
discards a whole frame rather than stall the relay when the CDC buffer is short — so a gap in the
counter is the only way a client can tell that samples were lost rather than that the axis stopped
moving. A client **may** use it to surface link saturation; it **must not** require contiguity, or
it will treat normal back-pressure as an error.

**CRC-16/CCITT-FALSE**: poly `0x1021`, init `0xFFFF`, no input or output reflection, no final
XOR. Canonical check: `"123456789"` → `0x29B1`. The magic is excluded from coverage — checksumming
constant bytes adds no detection power and doubles the chance of an implementation disagreeing.

### Message types

High bit of `TYPE` set = device→client, so direction is readable at a glance in a capture.

| Client → device | | Device → client | |
|---|---|---|---|
| `0x01` | `HELLO` | `0x81` | `HELLO_ACK` |
| `0x02` | `GET_CAL` | `0x82` | `CAL_DATA` |
| `0x03` | `SESSION_OPEN` | `0x83` | `SESSION_ACK` |
| `0x04` | `SESSION_CLOSE` | `0x84` | `ACK` |
| `0x05` | `COMMIT` | `0x85` | `NACK` |
| `0x06` | `RESET` | `0x86` | `RAW` |
| `0x07` | `KEEPALIVE` | | |
| `0x08` | `STREAM_SELECT` | `0x87` | `DIAG` |
| `0x09` | `GET_DIAG` | | |

**Firmware update (#295 reservation, assigned by #366):** types `0x10`–`0x1F` (client → device) and
`0x90`–`0x9F` (device → client), NACK reasons `0x09`–`0x0E` — see § Firmware upload below. `0x16`–`0x1F`
and `0x93`–`0x9F` are still free for it; nothing else may claim them.

| Client → device | | Device → client | |
|---|---|---|---|
| `0x10` | `UPL_INFO` | `0x90` | `UPL_INFO_ACK` |
| `0x11` | `UPL_BEGIN` | `0x91` | `UPL_STATUS` |
| `0x12` | `UPL_DATA` | `0x92` | `UPL_RESULT` |
| `0x13` | `UPL_END` | | |
| `0x14` | `UPL_FLASH` | | |
| `0x15` | `UPL_ABORT` | | |

These are numeric type codes, not strings. Contrast `_NODE_STATUS`, which *is* text because it
must imitate a DCS-BIOS message riding the ASCII stream; this channel is quiet during exchanges,
so binary is free.

They are also **not** `controlId`s. Those are the 16-bit routing values PanelBridge branches on
(`04-dcs-bios-integration.md`), a separate namespace. A `controlId` does appear *inside* a
`CAL_DATA` payload, so the client can label an axis "Roll" from `0x0010` rather than hardcoding
the mapping — but that is data in the message, not the message's identity.

### Payloads

| Type | `LEN` | Payload |
|---|---|---|
| `HELLO` | 0 | — |
| `HELLO_ACK` | 6 | `proto`:u8, `blobVersion`:u8, `axisSlots`:u8 (8), `fw`:u8×3 (major, minor, patch — the release version, `OPENSKYHAWK_FW_*` in `FirmwareVersion.h`) |
| `GET_CAL` | 0 | — |
| `CAL_DATA` | 82 | `presentMask`:u8, `calibratedMask`:u8, then **8 ×** { `controlId`:u16, `min`:u16, `centre`:u16, `max`:u16, `deadzone`:u16 } |
| `SESSION_OPEN` | 1 | `axisIdx`:u8 — axis to stream, or `0xFF` for none |
| `SESSION_ACK` | 5 | `timeoutMs`:u32 (30000), `axisIdx`:u8 — echoed selection |
| `SESSION_CLOSE` | 0 | — |
| `STREAM_SELECT` | 1 | `axisIdx`:u8 — change the streamed axis mid-session |
| `COMMIT` | 9 | `idx`:u8, `min`:u16, `centre`:u16, `max`:u16, `deadzone`:u16 — **exactly one axis** |
| `RESET` | 1 | `idx`:u8, or `0xFF` for all |
| `KEEPALIVE` | 0 | — |
| `ACK` | 1 | `type`:u8 being acknowledged |
| `NACK` | 3 | `type`:u8, `reason`:u8, `detail`:u8 (axis index, or `0xFF`) |
| `RAW` | 5 | `idx`:u8, `raw`:u16, `cal`:u16 |
| `GET_DIAG` | 0 | — |
| `DIAG` | 64 | link-health counters and event times — see below; bytes 47–63 reserved |
| `UPL_INFO` | 0 | — |
| `UPL_INFO_ACK` | 16 | `proto`:u8, `maxImageKB`:u8 (124), `chunk`:u16 (256), `state`:u8, `lastTarget`:u8, `lastResult`:u8, `lastFw`:u8×3, `bridgeFw`:u8×3 (bundled bridge image, #367; 0.0.0 = none), 3 reserved |
| `UPL_BEGIN` | 8 | `imageLength`:u32, `imageCrc`:u32 — both from the image's identity block |
| `UPL_DATA` | 260 | `offset`:u32 (a multiple of 256), `data`:u8×256 (last chunk `0xFF`-padded) — **client → device only** |
| `UPL_END` | 0 | — |
| `UPL_FLASH` | 2 | `target`:u8 (NODE_ID 1–63, `0xFE` = the unconfigured board, `0` = the bridge), `newNodeId`:u8 (`0xFF` = keep) |
| `UPL_ABORT` | 0 | — |
| `UPL_STATUS` | 8 | `phase`:u8, `target`:u8, `attempt`:u8, `block`:u16, `blocks`:u16, 1 reserved |
| `UPL_RESULT` | 8 | `result`:u8, `target`:u8, `fw`:u8×3, `attempts`:u8, `seconds`:u16 |

Field offsets are named in `AxisCal.h` — one enum per type with a payload (`CalHelloAckField`,
`CalDataField` + `CalDataRecField` and `CAL_DATA_REC_BYTES`, `CalSessionOpenField`,
`CalSessionAckField`, `CalStreamSelectField`, `CalCommitField`, `CalResetField`, `CalAckField`,
`CalNackField`, `CalRawField`, `CalDiagField`) — and SimGateway reads and writes payloads through them.

**Values on the wire are unsigned 0–65535**, matching storage and the node — `RAW`'s `raw` and
`cal`, the `CAL_DATA` endpoints, and what `COMMIT` writes. They are positions in the node's ADC
space, where zero carries no meaning, so a sign would say nothing extra.

**A client carries them unsigned everywhere they are used — wire, storage, capture and
`COMMIT` — and may convert to signed for display only.** The unsigned→signed offset belongs to
the HID report, not to this channel: it is a fixed 32768 applied once by `HIDAxis::dispatch()` on
the way into the report, never per-axis. All per-axis variation lives in the stored `centre`,
which is why an axis resting at raw 34728 still reads exactly 0 to DCS.

Display is the one place a client has a reason to differ, because it is showing the user a number
they will also see in DCS, and DCS shows ±32767. Converting at the render edge is therefore
expected. What must not happen is converting anywhere a value is stored, compared, or sent: a
displayed number is read by a person, while a committed one is read by the device.

Converting only some of these values is the case that hurts, and it fails quietly rather than
loudly: the offset applied twice, or zero times, still yields plausible in-range values, and
endpoints would then be displayed in units other than the ones being committed.

`CAL_DATA` is fixed-size with leading bitmasks rather than variable-length records, so each entry
is a clean 10 bytes and the whole thing reads with one typed-array view. `controlId` is `0x0000`
for an absent slot. `presentMask` bit *n* means a `HIDAxis` with that index is declared in the
gateway sketch; `calibratedMask` bit *n* means its stored endpoints are ordered. The client
displays only present axes — the device has 8 slots but a cockpit may populate fewer.

`RAW` carries both the pre-transform value and the same sample after the stored calibration, so
the dialog can show the sensor reading beside what DCS is currently receiving. It also carries
`idx` so a frame in flight across a `STREAM_SELECT` can be discarded.

`DIAG` (#355) is the gateway's link-health record: what went wrong on the PanelBridge UART and the
USB link, and what the firmware did about it (`07-simgateway-api.md#link-health-and-self-recovery`).
Times are `millis()` since this boot, `0` = never. Counters are u16 and saturate at `0xFFFF`.

| Offset | Field | Type | Meaning |
|---|---|---|---|
| 0 | `UPTIME_MS` | u32 | `millis()` when the reply was built — the reference for the times below |
| 4 | `FIRST_UART_ERR_MS` | u32 | first PL011 error this boot |
| 8 | `LAST_UART_ERR_MS` | u32 | latest PL011 error |
| 12 | `FIRST_USB_LOSS_MS` | u32 | first healthy → unhealthy host-link edge |
| 16 | `LAST_USB_LOSS_MS` | u32 | latest such edge |
| 20 | `UART_OVERRUN` | u16 | status ticks with RSR OE — the UART IRQ itself was held off |
| 22 | `UART_FRAMING` | u16 | … RSR FE — electrical |
| 24 | `UART_BREAK` | u16 | … RSR BE — RX held low (PanelBridge reset or off) |
| 26 | `UART_PARITY` | u16 | … RSR PE |
| 28 | `UART_RX_OVERFLOW` | u16 | software RX queue overflowed — `loop()` fell behind |
| 30 | `LINES_DROPPED` | u16 | line dropped whole, no CDC room (host not reading) |
| 32 | `LINES_CORRUPT` | u16 | line held a non-ASCII byte |
| 34 | `LINES_OVERLONG` | u16 | line exceeded `RELAY_LINE_MAX` |
| 36 | `FRAMES_DROPPED` | u16 | calibration frame dropped, no CDC room |
| 38 | `UART_RESTARTS` | u16 | FAULT self-clears |
| 40 | `USB_REATTACHES` | u16 | recovery-ladder detach/attach cycles |
| 42 | `CDC_STALLS` | u16 | host held the port, bytes pending, CDC IN endpoint stopped draining |
| 44 | `RECOVERY_REBOOTS` | u8 | recovery-ladder reboots — survives a warm reboot |
| 45 | `WATCHDOG_RESETS` | u8 | hardware-watchdog resets — survives a warm reboot |
| 46 | `RESET_REASON` | u8 | `CalDiagResetReason`: 0 power-on, 1 watchdog, 2 recovery reboot |
| 47–63 | `RESERVED` | — | transmit 0, ignore on receive (#295). `LEN` is fixed per type, so a new counter fills these bytes — a minor change — instead of growing `DIAG` |

The UART error counters count status ticks on which the bit was set, not characters: the core's
UART IRQ owns the data register. The two "survives" counters live in watchdog scratch registers and
reset on power-on, which on the Gateway means a USB replug.

**`CAL_PROTO_VERSION` stays 1.** `GET_DIAG` is purely additive: an older client never sends it,
and older firmware rejects the unknown type (`calLenValidForType()` fails), so its bytes go to
the relay and no reply comes. A client therefore treats "no `DIAG` within its request timeout"
as "not supported", not as a link fault. Bumping the version would make a client that requires
an exact match refuse calibration against new firmware until it synced, for no gain.

### Compatibility — how the channel grows (#295, D22)

The `_NODE_*` lines grow by appending fields (`04-dcs-bios-integration.md` § Compatibility). This
channel can't: `LEN` is fixed per type, so a longer payload on an existing type is "not a frame" to
an older reader. From v1.0 it follows its own rule:

1. **`HELLO` / `HELLO_ACK` never change**, in any version — they are how a client learns `proto`,
   so every client must always be able to read them.
2. **A new message type is a minor.** Older firmware fails `calLenValidForType()` on it and relays
   the bytes as DCS data, so no reply comes; the client treats a timeout as "not supported" and
   falls back (as `GET_DIAG` does). `CAL_PROTO_VERSION` stays.
3. **Any change to an existing type's `LEN`, field layout or meaning bumps `CAL_PROTO_VERSION`**,
   and is a major: a client refuses calibration against a `proto` it doesn't know.
### Firmware upload (#366)

The Gateway is the uploader: the client streams an image, the Gateway stages and verifies it, then
flashes a board's bootloader over CAN through PanelBridge (`12-bootloader.md`) — and finishes on its
own. Values: `UploadProtocol.h` (states, phases, results, field offsets) and `AxisCal.h` (type codes,
NACKs). Behaviour: `07-simgateway-api.md` § Uploader.

- **Session.** `UPL_BEGIN` opens it (`NACK BUSY` while calibrating or flashing; `TOO_LARGE` over 124 KB
  or not a whole number of words); `UPL_DATA` chunks at any offset, in any order, resendable
  (`BAD_OFFSET`); `UPL_END` checks the staged bytes against `UPL_BEGIN`'s CRC (`IMAGE_CRC`) and the
  identity block against both (`BAD_IMAGE`). `UPL_FLASH` needs a verified image (`NOT_STAGED`, or
  `NO_SESSION` with none open). Idle 30 s without a frame (staging / staged) → the session and the
  image are dropped. `UPL_ABORT` drops them (`BUSY` while flashing). `UPL_INFO` works any time.
- **One session at a time:** calibration `SESSION_OPEN` answers `BUSY` while an upload session is open
  or a flash runs, and `UPL_BEGIN` answers `BUSY` during a calibration session.
- **During a flash** the client sends `KEEPALIVE` (~1 s) and gets `UPL_STATUS` (each phase change, and
  every 250 ms while writing) then one `UPL_RESULT`. No client frame for 5 s → the Gateway stops sending
  them, finishes anyway, and keeps the result for the next `UPL_INFO_ACK` (`lastTarget`, `lastResult`,
  `lastFw`). The staged image stays staged after a result, so it can be flashed again.
- **Target `0`, the Gateway's own cockpit side (#367):** flashes the staged image, or — nothing staged —
  the bridge image the Gateway firmware carries (`bridgeFw`); neither → `NOT_STAGED`. A client that
  wants the bundled image sends `UPL_ABORT` first to drop a staged one. Behind when the node-0 row's
  version ≠ `bridgeFw`.
- **The Gateway owns the UART while flashing:** export bytes from the client are dropped (the client
  should have paused its relay anyway), and the bootloader's `_BOOT_RSP` lines are kept from the
  client. Idle, `_BOOT_RSP` lines are relayed as before.
- **Results:** `OK` (the app runs and reports the image's version), `NO_RESPONSE`, `NO_BOOTLOADER`,
  `ID_MISMATCH`, `KIND_MISMATCH`, `TOO_BIG_FOR_BOARD` (all decided before anything is erased),
  `WRITE_FAILED`, `VERIFY_FAILED` (after `UPL_RETRIES` = 2 whole-flash retries), `VERSION_MISMATCH`,
  `TIMEOUT`. (`UNSUPPORTED` is no longer sent since #367.) For an unconfigured board, `target` in the result
  is the NODE_ID it adopted.
- Older Gateway firmware relays unknown types, so a `UPL_INFO` timeout reads as "updates not supported".

### Session model

`HELLO`, `GET_CAL` and `GET_DIAG` work **outside** a session. This is what lets the client show
calibrated/uncalibrated badges and an uncalibrated count as soon as the port opens, without
opening a dialog. `COMMIT`, `RESET` and `STREAM_SELECT` require an open session.

**Only the axis under calibration streams.** The dialog calibrates one axis at a time, so
`SESSION_OPEN` names it and `STREAM_SELECT` changes it as the user moves along the rail — one
11-byte message per switch, rather than streaming seven axes nobody is looking at. Every other
axis dispatches to HID exactly as normal; it simply emits no `RAW`.

A session ends on `SESSION_CLOSE` or after **30 s** with no inbound frame. `KEEPALIVE` exists
because a user dragging a slider sends nothing inbound for long stretches while `RAW` flows
outbound; send it roughly every 10 s.

**The frame parser runs at all times, not only during a session.** It has to: `HELLO` and
`GET_CAL` are answered outside a session, so there is no point at which the gateway can stop
looking. One parser, always on.

That fixes what happens to a session-requiring command that arrives without one. It has already
been consumed by the time the session check runs, so relaying it is not an option:

- A candidate with valid magic, a `LEN` matching its type, and a passing CRC is a **frame**. It is
  consumed and never reaches the UART.
- `COMMIT`, `RESET` and `STREAM_SELECT` without an open session are consumed and answered
  `NACK NO_SESSION`. `HELLO`, `GET_CAL`, `GET_DIAG` and `KEEPALIVE` are always accepted.
- Anything failing magic, `LEN` or CRC is **not** a frame. Its bytes are re-emitted into the relay
  untouched, exactly as today.

The last rule is what keeps the always-on parser honest: DCS-BIOS traffic can never be silently
eaten, because being eaten requires passing all three checks.

### Validation and failure

`COMMIT` is validated in order: frame CRC → `LEN == 9` → `idx < 8` → `deadzone == 0` →
`min < centre < max`. Only then is the axis applied and persisted.

Because a `COMMIT` carries exactly one axis, a rejection is unambiguous — `detail` names the
axis, and no other axis is affected by it.

| `reason` | Name | Meaning | Client remedy |
|---|---|---|---|
| `0x01` | `BAD_CRC` | frame checksum mismatch | retransmit |
| `0x02` | `BAD_LENGTH` | `LEN` inconsistent with its type | client bug — log it |
| `0x03` | `BAD_TYPE` | unknown type code | never sent: an unknown type fails `calLenValidForType()` first and is relayed, so the client sees a timeout (see § Compatibility) |
| `0x04` | `BAD_INDEX` | `idx` out of range | client bug |
| `0x05` | `BAD_ORDER` | endpoints not ordered; `detail` names the axis | "Axis N: endpoints out of order" |
| `0x06` | `NO_SESSION` | command requires an open session | client bug |
| `0x07` | `NO_STORAGE` | the flash write failed | device fault — advise reflash |
| `0x08` | `BAD_DEADZONE` | non-zero deadzone; reserved in this version | client bug |

**The two failure modes are distinct and need different messaging.** A **timeout** means no frame
arrived: the client keeps the dialog open, retains its dirty state, and advises checking the
connection. A **NACK** means the device answered and refused, naming the axis and reason.

"Nothing was written" is literally true on a failed commit, not a simplification — the write is a
single sector erase and program.

**`RESET` persists immediately** — it clears the named axis (or all of them) and writes flash by
the same path as `COMMIT`, returning `ACK` or `NACK`. It does **not** need a following `COMMIT`,
and there is no pending state for a later `SESSION_CLOSE` to discard. "Reset and save" is one
message, not two.

**The live calibration only ever holds committed values.** On a failed write the device answers
`NACK NO_STORAGE` and its in-RAM calibration is unchanged, so the read-back the client is told
to perform agrees with the refusal. This matters most for `RESET 0xFF`, where the alternative
would be every axis silently dropping to uncalibrated while the device reported that nothing
happened.

**Read back after committing.** `ACK` means "received and applied"; a subsequent `GET_CAL` is what
proves "stored, and here is what I hold". Badges should be driven from that re-read rather than
from client optimism.

### Flow

```
port opens    → HELLO          / ← HELLO_ACK
              → GET_CAL        / ← CAL_DATA          badges; client resumes relaying

Calibrate     client stops relaying FIRST
              → SESSION_OPEN {axis} / ← SESSION_ACK
              ← RAW …                                capture, dwell, spike rejection
              → KEEPALIVE                            ~every 10 s
next axis     → STREAM_SELECT {axis} / ← ACK         edits accumulate client-side

Save          → COMMIT {axis} / ← ACK                one sector erase, ~28 ms
              repeat per edited axis — each is independent
              → GET_CAL        / ← CAL_DATA          read-back drives the badges
              → SESSION_CLOSE  / ← ACK               client resumes relaying

Cancel        → SESSION_CLOSE  / ← ACK               nothing was ever written
Client dies   30 s timeout → streaming stops, session closes
```

There is **no calibration loop and no pending state on the device.** Capture, dwell timing, spike
rejection and centre selection are entirely client-side; the device streams and stores.

**There is therefore no preview of pending endpoints, and this needs stating plainly because it
is easy to assume otherwise.** `RAW`'s `cal` field is the sample through the calibration the
device *currently holds* — the old one, or none — for the whole of a capture. It shows what DCS
is receiving right now, not what the endpoints being captured would produce. The effect of new
endpoints becomes visible only after `COMMIT` and the read-back that follows it.

A client that wants to draw the pending curve must compute it, which is a deliberate
non-requirement of this design: the client is not asked to reimplement the transform, and doing so
reintroduces the integer-truncation divergence the split avoids. If a device-computed pending
preview is ever wanted, it needs a RAM-only write reinstated — a protocol change, not a client
change.

### Notes for the client implementer

**The stream is event-driven and genuinely silent at rest.** `AnalogInput` gates emission on a
128-count hysteresis, so a settled axis sends nothing at all — measured on the bench rig, a
stationary stick produced about one report per second, versus ~61/s while moving. Dwell detection
must therefore mean "the last received value has not moved for N ms", never "N consecutive samples
agreed", which would never complete on a stick held still.

**Do not assume an update rate.** `AnalogInput`'s read throttle is **per instance** — the `pollMs`
field of its `AnalogConfig`, default `AnalogConfig::DEFAULT_POLL_MS` = 8 ms — so it is a per-axis build choice, not a
constant you can read off the class, and two axes on one node may differ. Whatever it is set to
bounds only that axis's ADC sampling; hysteresis, `PanelGroup::loop()` timing, CAN, and PanelBridge
all reduce it further. Measure it.

**The near-rail case needs care.** `shouldEmit()` has a rule for reaching 0 or 65535 that cannot
fire while an axis spans only the middle portion of the ADC range, so the last few counts before
each mechanical stop can be swallowed. Capture should treat a value that stops changing as the
endpoint rather than waiting for a rail it will never see.

**Calibration is not permanent.** Measured on an undisturbed bench rig within a single day, a
hall axis's rest position moved ~1160 raw counts — roughly 5.4% of its lower segment — from
thermal null shift and magnet-mount movement. Re-running calibration should be cheap and obvious
in the UI, not framed as one-time setup.

**A commit stalls the relay.** The erase runs with interrupts disabled for ~28 ms, during which
inbound DCS-BIOS bytes are lost. This is acceptable for a deliberate once-per-session action, but
it is another reason the client should have stopped relaying first.
