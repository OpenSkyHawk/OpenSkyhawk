# Compatibility & Versioning

Firmware 1.0 is a promise: **a sketch written for 1.x keeps compiling, and boards running any 1.x
release work together**, until 2.0. This page says exactly what that promise covers, what it
doesn't, and how each kind of change moves the version number.

All the firmware libraries share one version: they ship together and speak the same CAN wire
format, so a mixed set has no meaning. Every board reports the version it runs (see
[Mixing versions](#mixing-versions-on-one-bus)).

## What 1.x keeps stable

| Area | Covered |
|---|---|
| **Sketch API** | The calls a sketch makes: `PanelGroup::setup` / `loop` / `registerExpander` / `registerADC`, `PanelBridge::setup` / `loop` / `onNodeAlive` / `onNodeDead`, `SimGateway::setup` / `loop`, `PduNode::setup` / `loop`, and `STM32Board::begin` / `setDebug` / `isDebug` / `log` / `diagSerial`. |
| **Control classes** | Every class in `OpenSkyhawk.h`, plus `DrumDisplay`, `ShiftBus` and `I2cMux`: their constructors, config structs (`AnalogConfig`, `AngleConfig`, `StepperConfig`, `GaugeCal`, `DrumReadout`, …), enums, factory functions (`makeX27Config`) and `DEFAULT_*` constants. The `PinRef` constructors. |
| **Your own control classes** | What a class written against [the control-class interface](../guides/new-control-type.md) relies on: the `InputBase` / `OutputBase` methods and self-registration, the `PinRef` methods, `CANProtocol::sendBatched` with `ControlPacket` and the `canIdEvt*` frame IDs, and `FaultSource`. |
| **Build flags** | The flags [Build Flags](build-flags.md) tells you to set or opt out of (`NODE_ID`, `NODE_HEALTH_TELEM=0`, `PANELBRIDGE_NODE_STATUS=0`, …), and their meaning. |
| **CAN bus** | Every frame ID and its payload layout, so the [arbitration order](../architecture/can-bus.md) too, and the values those payloads carry (board kinds, fault codes, telemetry pages, flag bits). |
| **Gateway link** | The HID frame between the Gateway's two chips, and the request that sends the PanelBridge chip to its bootloader for an update (`BRIDGE_ENTER_EXPORT_ADDRESS` in namespace `Boot`, `0xFF02`, `BootProtocol.h:181`). |
| **PC side** | The `_NODE_STATUS`, `_NODE_STATUS_END`, `_NODE_TELEM` and `_BOOT_RSP` messages, the roster request (`NODE_STATUS_REQ_ADDR`, `0xFF00`, `NodeStatus.h:42`), the forget-node request (`NODE_FORGET_REQ_ADDR`, `0xFF04`, `NodeStatus.h:46`) and the bootloader relay addresses (`RELAY_EXPORT_ADDRESS` + `RELAY_EXPORT_WORDS` in namespace `Boot`, `0xFF10`–`0xFF4F`, `BootProtocol.h:162,163`); the USB calibration channel; the Gateway's USB identity (VID, PID, product and serial-port names), because DCS binds your controls to it. |
| **Control IDs** | `DCSIN_*` and `CTRL_*` ids never change. `A_4E_C_*` output addresses follow DCS-BIOS (see below). |

## What isn't covered

- **Library-internal functions**, even when a header makes them public. They say so in their
  comments: the CANProtocol runtime (apart from `sendBatched`), STM32Board's status-LED and health
  functions, PanelGroup's expander-cache functions.
- **The protected hooks of the built-in class families**: `MultiPosInput`'s position hook and
  `RotaryEncoder`'s family constructor. Derive your own classes from `InputBase` / `OutputBase`, not
  from a concrete class.
- **Test seams** (the `*_TEST` builds) and the specs under `Firmware/ScratchPad/`.
- **Debug output text** on DiagSerial.
- **Stored calibration.** It survives a normal reflash, but a release may reset it. The client
  offers to restore the Gateway's joystick calibration from its own copy (on the same PC); the PDU
  shows `UNCAL` and is recalibrated at the bench.
- **`A_4E_C_*` addresses**, which belong to DCS-BIOS. When the automatic DCS-BIOS update removes or
  moves one, its pull request lists it, and the maintainer decides whether that release needs a
  major version.

## How versions move

The version follows [Conventional Commits](https://www.conventionalcommits.org): the commit type of
each change decides the bump, and the release tooling applies it.

| Change | Bump | Example |
|---|---|---|
| Breaks a covered item: a sketch stops compiling, a field moves, a value changes meaning | **major** — 2.0 | changing a constructor's arguments; moving a byte in `HEALTH_n`; changing the roster-request address |
| Adds something, and everything written for 1.x still works | **minor** — 1.1 | a new control class; a new config field with a default; a new telemetry page; a new fault code |
| Fixes behaviour without changing a covered item | **patch** — 1.0.1 | a debounce bug; a wrong default corrected to what the docs promised |

A breaking commit is marked `!` (`feat(firmware)!: …`) or carries a `BREAKING CHANGE:` footer.
Raising a third-party library's version range is at least a minor, and a major if it changes the
sketch API.

## How the wire grows without breaking

Inside one major version, the wire format only **grows**, and every reader tolerates growth:

- **Existing fields never move, resize or change meaning.** A CAN frame never grows past its 8
  bytes; new data goes into a frame's reserved bytes, a new telemetry page, or a new frame in a
  free part of the ID map.
- **Reserved bits and bytes are sent as 0 and ignored on receive**, so a later minor can give them
  a meaning without confusing older boards.
- **Unknown values are kept, not rejected.** A board kind, fault code or telemetry page a reader
  doesn't know shows as "unknown".
- **PC messages only grow at the end.** A reader uses the fields it knows from a longer
  `_NODE_STATUS` line and ignores the rest; a shorter one means older firmware.
- **The calibration channel grows by new message types.** Each type has one fixed length, so an
  existing type never grows; `DIAG` keeps spare bytes for new counters.

The [bootloader](bootloader.md) uses CAN IDs `0x740`–`0x7C1`. Message types `0x10`–`0x1F` /
`0x90`–`0x9F` on the Gateway's USB channel are the firmware updater's (`0x10`–`0x15` and `0x90`–`0x92` in
use).

**Byte order:** multi-byte fields in CAN payloads, the Gateway's HID frame and calibration frames
are little-endian. The
numbers in `_NODE_STATUS` are written as hex text, most significant digit first; `_NODE_TELEM`
forwards the raw CAN bytes, so its fields stay little-endian.

## Mixing versions on one bus

**Every board on one bus must run the same major version.** Minor and patch versions can differ.

Each board announces its firmware version every time it starts, and again whenever the Gateway asks
it for its state, which it does as soon as it sees the board. The Gateway passes every board's
version to the client, its own included, so a board left on an older major can be spotted — even one
that was already running when the Gateway started.

To update a cockpit across a major version, reflash every board, the Gateway included.
