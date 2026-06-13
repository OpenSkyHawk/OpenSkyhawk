# Adding a New Control Type

Most input and output [control types](../firmware/control-types.md) are specified but not yet
implemented (Phase 4 inputs, Phase 5 outputs). If your panel needs one that doesn't exist, you
may be the one writing it. This guide is the pattern; the per-class spec lives in the
FirmwarePlan TechSpec.

!!! note "Read the spec first"
    Each control type has a detailed specification in the project's TechSpec (value encoding,
    debounce, polling rate, edge cases). Implement to the spec — don't improvise behaviour.
    `AngleSensor`'s base class is a documented gap with no spec yet; don't invent it.

## Where it goes

Control classes live in the **`PanelGroup`** library (`Firmware/Libraries/PanelGroup/`). Inputs
derive from the input base, outputs from the output base.

## The pattern

- **Self-registration.** Constructors register the object into a static linked list — the
  sketch just declares globals; `PanelGroup::loop()` walks the list. No manual registration.
- **Take a `PinRef`, not a raw pin.** Support GPIO / MCP23017 / ADS1115 backends through
  `PinRef`. PWM/servo outputs must check `isGpio()` (only direct GPIO can do PWM).
- **Inputs fire a `ControlPacket`** via `CANProtocol::sendBatched(canIdEvt(NODE_ID), pkt)` — the
  `controlId` (passed to your constructor) decides routing, not the class. Honour the batch
  flush rules (slot B `0x0000`; flush a half-full batch within two `loop()` iterations).
- **Outputs receive `CTRL_BCAST` packets** dispatched by `PanelGroup::loop()`; pick out your
  value by address + mask.
- **Normalise analog to 16-bit (0–65535)** at the source — no rescaling downstream.

## Value encoding and PanelBridge dispatch

Inputs send a numeric `value`; PanelBridge maps it to a DCS-BIOS argument by **dispatch type**
(`SWITCH`, `ACTION`, `ENCODER`, `ACCEL_ENCODER`, `MULTIPOS`). When you add an input class, decide
which dispatch type it uses and make sure the generator/input-map handles it — see
[DCS-BIOS Integration](../firmware/dcsbios-integration.md).

## Tests

Every control type ships with a test project under `Firmware/Tests/`. Add one that exercises the
value semantics and edge cases from the spec, on real hardware where the type needs it.

## Then document it

Update [Control Types](../firmware/control-types.md): move the type from "not started" to
implemented, and add an example. Honesty in the status table matters.
