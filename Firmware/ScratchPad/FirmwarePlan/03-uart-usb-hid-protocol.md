# 03 — UART / USB-HID Protocol

**Owns:** UART multiplexing scheme, HID frame wire format, parser resync algorithm,
USB identity and Joystick configuration, the USB CDC calibration protocol.
**Does not own:** HID axis/button class declarations (→ 07), calibration storage and the
transform itself (→ 07), CAN frame formats (→ 02), DCS-BIOS routing logic (→ 04),
boot sequencing (→ 09).

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

> The calibration protocol below *does* carry a CRC, which is not a contradiction. This frame is
> fixed-length on a two-node dedicated link with a provably disjoint alphabet, and it sits on the
> joystick latency path. Calibration frames are variable-length, cross the USB CDC boundary where
> they interleave with a third-party stream, and a commit is a deliberate once-per-session action
> nowhere near that path. Different risk, different answer.

---

## Parser Resync (SimGateway)

SimGateway demultiplexes the incoming UART byte stream as follows:

- Any byte ≤ `0x7F` → DCS-BIOS byte; forward to USB CDC immediately.
- On byte `0xAA`: peek at the next byte.
  - If it is `0x55` → HID frame: read 4 more bytes, parse `controlId` + `value`, dispatch.
  - If it is not `0x55` → not a valid frame header: forward `0xAA` + the mismatched byte to
    USB CDC and resume scanning.

This ensures any framing error is self-healing at the next byte boundary.

**Consequence for anything else on the CDC stream:** the resync path puts a bare `0xAA` on USB
CDC, followed by the mismatched byte. `0xAA` is therefore the one high-bit value the gateway is
known to emit outbound, and it does so precisely when the link is already degraded. Any host-side
demultiplexer watching for a `0xAA`-led frame must validate before acting — see the calibration
protocol's CRC requirement below.

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
  assembly**. A calibration frame injected mid-line is then harmless — removing its bytes
  rejoins the interrupted line correctly.

### Frame format

Identical envelope in both directions.

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | `MAGIC` — `AA 53 4B 43` (`\xAA` `S` `K` `C`) |
| 4 | 1 | `TYPE` |
| 5 | 1 | `SEQ` — echoed in the response |
| 6 | 2 | `LEN` — payload length, uint16 little-endian |
| 8 | `LEN` | `PAYLOAD` |
| 8+`LEN` | 2 | `CRC16` — CRC-16/CCITT-FALSE over `TYPE`‖`SEQ`‖`LEN`‖`PAYLOAD` |

10 bytes of envelope. All multi-byte fields little-endian. Payloads are binary, not text: the
device is the constrained end and the client has the budget to unpack.

**`0xAA` leads all non-DCS data** on this link by project convention, matching the HID frame
magic. It is not collision-proof on the outbound direction — the parser resync path above emits a
bare `0xAA` — so the **CRC is mandatory, and a client must re-emit CRC-failed candidate bytes
into its line assembler rather than dropping them.** Otherwise a corrupted DCS-BIOS line would be
silently swallowed. A false frame requires `AA 53 4B 43` to appear consecutively *and* the CRC to
pass.

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
| `0x08` | `STREAM_SELECT` | | |

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
| `HELLO_ACK` | 6 | `proto`:u8, `blobVersion`:u8, `axisSlots`:u8 (8), `fw`:u8×3 (major, minor, patch) |
| `GET_CAL` | 0 | — |
| `CAL_DATA` | 82 | `presentMask`:u8, `calibratedMask`:u8, then **8 ×** { `controlId`:u16, `min`:u16, `centre`:u16, `max`:u16, `deadzone`:u16 } |
| `SESSION_OPEN` | 1 | `axisIdx`:u8 — axis to stream, or `0xFF` for none |
| `SESSION_ACK` | 5 | `timeoutMs`:u32 (30000), `axisIdx`:u8 — echoed selection |
| `SESSION_CLOSE` | 0 | — |
| `STREAM_SELECT` | 1 | `axisIdx`:u8 — change the streamed axis mid-session |
| `COMMIT` | 1 + 9n | `count`:u8 (1–8), then n × { `idx`:u8, `min`:u16, `centre`:u16, `max`:u16, `deadzone`:u16 } — **dirty axes only** |
| `RESET` | 1 | `idx`:u8, or `0xFF` for all |
| `KEEPALIVE` | 0 | — |
| `ACK` | 1 | `type`:u8 being acknowledged |
| `NACK` | 3 | `type`:u8, `reason`:u8, `detail`:u8 (axis index, or `0xFF`) |
| `RAW` | 5 | `idx`:u8, `raw`:u16, `cal`:u16 |

**Values on the wire are unsigned 0–65535**, matching storage and the node. The client converts to
signed ±32767 once, at its display edge. Getting this wrong is quiet rather than loud: the offset
applied twice, or zero times, still yields plausible in-range values.

`CAL_DATA` is fixed-size with leading bitmasks rather than variable-length records, so each entry
is a clean 10 bytes and the whole thing reads with one typed-array view. `controlId` is `0x0000`
for an absent slot. `presentMask` bit *n* means a `HIDAxis` with that index is declared in the
gateway sketch; `calibratedMask` bit *n* means its stored endpoints are ordered. The client
displays only present axes — the device has 8 slots but a cockpit may populate fewer.

`RAW` carries both the pre-transform value and the same sample after the stored calibration, so
the dialog can show the sensor reading beside what DCS is currently receiving. It also carries
`idx` so a frame in flight across a `STREAM_SELECT` can be discarded.

### Session model

`HELLO` and `GET_CAL` work **outside** a session. This is what lets the client show
calibrated/uncalibrated badges and an uncalibrated count as soon as the port opens, without
opening a dialog. `COMMIT`, `RESET` and `STREAM_SELECT` require an open session.

**Only the axis under calibration streams.** The dialog calibrates one axis at a time, so
`SESSION_OPEN` names it and `STREAM_SELECT` changes it as the user moves along the rail — one
11-byte message per switch, rather than streaming seven axes nobody is looking at. Every other
axis dispatches to HID exactly as normal; it simply emits no `RAW`.

A session ends on `SESSION_CLOSE` or after **30 s** with no inbound frame. `KEEPALIVE` exists
because a user dragging a slider sends nothing inbound for long stretches while `RAW` flows
outbound; send it roughly every 10 s.

Commands arriving outside a session are **relayed to the UART verbatim, not rejected.** There is
deliberately no `NOT_IN_SESSION` response: a second always-on watcher would give DCS traffic a
path to trigger outbound frames. A client that missed a session close and keeps sending will put
frame bytes into PanelBridge's DCS-BIOS input, which self-heals at the next sync. Avoiding that is
the client's responsibility.

### Validation and failure

`COMMIT` is validated in order, and is **all-or-nothing per frame**: frame CRC → `LEN == 1 + 9 ×
count` and `1 ≤ count ≤ 8` → every `idx < 8` → every `deadzone == 0` → every record satisfies
`min < centre < max`. Only then is anything applied. A three-axis commit with one bad axis writes
none, otherwise the client's dirty-state bookkeeping desynchronises from the device.

| `reason` | Name | Meaning | Client remedy |
|---|---|---|---|
| `0x01` | `BAD_CRC` | frame checksum mismatch | retransmit |
| `0x02` | `BAD_LENGTH` | `LEN` inconsistent with type or `count` | client bug — log it |
| `0x03` | `BAD_TYPE` | unknown type code | protocol version mismatch |
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

Save          → COMMIT [dirty axes] / ← ACK          one sector erase, ~28 ms
              → GET_CAL        / ← CAL_DATA          read-back drives the badges
              → SESSION_CLOSE  / ← ACK               client resumes relaying

Cancel        → SESSION_CLOSE  / ← ACK               nothing was ever written
Client dies   30 s timeout → streaming stops, session closes
```

There is **no calibration loop and no pending state on the device.** Capture, dwell timing, spike
rejection and centre selection are entirely client-side; the device streams and stores. An earlier
design had a RAM-only write for live preview, which was dropped once the preview moved into the
`RAW` frame itself.

### Notes for the client implementer

**The stream is event-driven and genuinely silent at rest.** `AnalogInput` gates emission on a
128-count hysteresis, so a settled axis sends nothing at all — measured on the bench rig, a
stationary stick produced about one report per second, versus ~61/s while moving. Dwell detection
must therefore mean "the last received value has not moved for N ms", never "N consecutive samples
agreed", which would never complete on a stick held still.

**Do not assume an update rate.** `AnalogInput::POLL_MS = 8` bounds one axis's ADC sampling, but
hysteresis, `PanelGroup::loop()` timing, CAN, and PanelBridge all reduce it further. Measure it.

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
