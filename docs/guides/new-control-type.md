# Adding a New Control Type

The [control types](../firmware/control-types.md) cover the switches, selectors, encoders, pots,
lamps, gauges and displays of the A-4E cockpit. If your panel needs something none of them does,
you can write your own class — in your own sketch, or added to the library for everyone.

The interface a control class is written against is part of the firmware's compatibility
promise: a class you write against v1 keeps compiling and working on every 1.x release.

!!! note "Check the family first"
    Before writing a new class, check whether yours only changes an existing class's *source*
    (inputs) or *sink* (outputs). Then it belongs in that class's family as a subclass, not as a
    new class. Names follow `dcs-bios-arduino-library` where an equivalent exists:
    `RotaryAcceleratedEncoder` extends `RotaryEncoder`, `Dimmer` and `IntegerOutput` extend
    `AnalogOutput`. Family subclasses use protected hooks that are *not* part of the promise, so
    they belong in the library, not a sketch.

## The interface

An input derives from `OpenSkyhawk::InputBase`, an output from `OpenSkyhawk::OutputBase`. Declaring
an object registers it — `PanelGroup::setup()` and `PanelGroup::loop()` find it on their own.

| Input (`InputBase`) | Called |
|---|---|
| `configure()` | once, at start-up, after the expanders are ready — set your pins up here, not in the constructor |
| `forceReport()` | at start-up and on every `SYNC_REQ` — send the current state, unconditionally |
| `poll()` | every `loop()` — send only on a confirmed change; never block |
| `sampleTick()` | optional; from a sampling interrupt on boards that run one — read cached pins only |

| Output (`OutputBase`) | Called |
|---|---|
| `configure()` | once, at start-up — set your pins up here |
| `onControlPacket(controlId, value)` | for every value DCS sends — keep the ones whose `controlId` is yours, store them, return |
| `update()` | optional; every `loop()` — do the continuous work (stepping, fading) here, not in `onControlPacket` |

Read and write hardware through the `PinRef` you were given (`read()`, `write()`,
`readAnalog()`, …), never a raw pin, so the class works on board pins, expanders and shift
registers alike. An output that can fail (a display that stops answering, say) also derives from
`FaultSource`, so the fault shows on the node's health.

## Sending a value

An input sends `CANProtocol::sendBatched(canIdEvt(NODE_ID), ControlPacket{ controlId, value })`.
PanelGroup batches and flushes the frames for you. The `controlId` you were given decides where the
value goes — a `DCSIN_*` id to DCS-BIOS, a `CTRL_*` id to the joystick — so the class never needs to
know. Analog values are 0–65535, already scaled.

The frame family decides what DCS-BIOS receives:

| Frame | Value | DCS-BIOS gets |
|---|---|---|
| `canIdEvt` | the position or level | that number (`set_state`) |
| `canIdEvtRel` | a signed step, ±n | `+n` / `-n` (`variable_step`) |
| `canIdEvtDir` | ±1 | `INC` / `DEC` (`fixed_step`) |
| `canIdEvtAction` | 0 | `TOGGLE` (`action`) |

The relative, direction and action frames only reach DCS-BIOS; a joystick input uses `canIdEvt`.

## An example

A pushbutton that sends 1 while held and 0 when released, written in a sketch:

```cpp
#include <Wire.h>
#include <OpenSkyhawk.h>

// A pushbutton that sends 1 while held and 0 when released.
// (No debounce — kept short. A real class should debounce, like Switch2Pos.)
class SimpleButton : public OpenSkyhawk::InputBase {
public:
    SimpleButton(uint16_t controlId, PinRef pin) : _id(controlId), _pin(pin) {}

    void configure() override { _pin.configureAsInput(); }   // pins are ready only now

    void forceReport() override {                             // boot and every SYNC_REQ
        _pressed = !_pin.read();                              // active-low
        send();
    }

    void poll() override {                                    // every loop(); never block
        const bool pressed = !_pin.read();
        if (pressed != _pressed) { _pressed = pressed; send(); }
    }

private:
    void send() {
        CANProtocol::sendBatched(canIdEvt(NODE_ID), ControlPacket{ _id, (uint16_t)(_pressed ? 1 : 0) });
    }

    uint16_t _id;
    PinRef   _pin;
    bool     _pressed = false;
};

SimpleButton windDir(DCSIN_ASN41_WINDDIR_BTN, PinRef(PB0));

void setup() {
    Wire.begin();
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
```

## Adding it to the library

A class added to the library lives under `Firmware/Libraries/PanelGroup/Inputs/` or `Outputs/`,
follows its TechSpec in `Firmware/ScratchPad/TechSpec/`, and ships with a test project under
`Firmware/Tests/` that exercises its values and edge cases, on real hardware where it needs it.
Then add it to the `OpenSkyhawk.h` umbrella header and to
[Control Types](../firmware/control-types.md), with an example.
