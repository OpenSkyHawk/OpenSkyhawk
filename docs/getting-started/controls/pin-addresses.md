# Pin addresses (`PinRef`)

Every switch, knob and light in your sketch needs to know where its wire is plugged in. You tell
it with a **pin address**, written `PinRef(...)`. You'll write one for every wire on your panel,
so it's worth a few minutes to understand how they work before anything else.

## It's an address

The easiest way to think of a pin address is as a postal address. A wire on the board itself is
like a house: it has one name and one place, so its address is short — `PinRef(PA0)`. A wire on
an [expander](expanders.md) is more like an apartment, where you need the building, the floor and
the door — `PinRef(expander1, PORT_A, 0)`.

The control is the mail carrier. It delivers to whatever address you give it, and it doesn't care
whether that address is a house or an apartment. That's why every switch, knob and light works
the same way no matter where you plug it in: you only ever change the address.

## Every kind of address

| The wire goes to… | You write | Example |
|---|---|---|
| A pin on the board | `PinRef(pin name)` | `PinRef(PA0)` |
| A [digital expander](expanders.md#digital-expander-mcp23017) | `PinRef(expander, port, pin)` | `PinRef(expander1, PORT_B, 2)` |
| A [shift-register chain](expanders.md#shift-register-chain-74hc165-74hc595) | `PinRef(ShiftBus1, chip, pin)` | `PinRef(ShiftBus1, 0, 3)` |
| An [analog expander](expanders.md#analog-expander-ads1115) | `PinRef(adc, input)` | `PinRef(adc1, 0)` |

For a pin on the board, use the name printed next to it, such as `PA0`. On an expander, the
numbers count from 0, so `PinRef(expander1, PORT_B, 2)` is the expander's pin `GPB2`, and
`PinRef(ShiftBus1, 0, 3)` is pin 3 on the first chip in the chain.

## Moving a wire? Change one line

Here's the same switch plugged in three different places. Click through the tabs and you'll see
that only the address changes — the `Switch2Pos` line is identical every time.

=== "On the board"

    ```cpp
    const PinRef MASTER_ARM_PIN = PinRef(PA0);
    Switch2Pos masterArm(DCSIN_ARM_MASTER, MASTER_ARM_PIN);
    ```

=== "On a digital expander"

    ```cpp
    const PinRef MASTER_ARM_PIN = PinRef(expander1, PORT_A, 0);
    Switch2Pos masterArm(DCSIN_ARM_MASTER, MASTER_ARM_PIN);
    ```

=== "On a shift register"

    ```cpp
    const PinRef MASTER_ARM_PIN = PinRef(ShiftBus1, 0, 0);
    Switch2Pos masterArm(DCSIN_ARM_MASTER, MASTER_ARM_PIN);
    ```

## Name your addresses at the top

It's a good habit to give every address a name and keep them all together at the top of your
sketch, one line per wire, named after whatever is on the end of it:

```cpp
// ── Where everything is wired ──────────────────────────────
const PinRef MASTER_ARM_PIN  = PinRef(PA0);
const PinRef GEAR_HANDLE_PIN = PinRef(expander1, PORT_A, 1);
const PinRef CAUTION_LED_PIN = PinRef(ShiftBus1, 0, 4);
```

That way, if you ever move a wire, there's exactly one line to fix. The block also doubles as
your wiring list when you come to build the harness.

## Which controls work where

| | Board pin | Digital expander | Shift register | Analog expander |
|---|:-:|:-:|:-:|:-:|
| Switches and buttons | ✅ | ✅ | ✅ | — |
| Rotary encoders | ✅ | ✅ | ✅ best | — |
| Knobs and sliders | ✅ analog pins | — | — | ✅ |
| Lights | ✅ | ✅ | ✅ | — |
| Gauge needles | ✅ | ✅ | ✅ fastest | — |
| Dimmable backlights | ✅ PWM pins | — | — | — |

The gaps come down to what each kind of pin can actually sense. An analog expander measures how
far something is turned, so it can't reliably read a switch that is simply on or off. A digital
expander or a shift register is the opposite: it only sees on or off, so it can't tell where a
knob is pointing. Dimming a light needs a pin that can switch thousands of times a second, and
only some of the board's own pins can do that — they're marked on the board.

??? note "Which controls are in each row?"
    - **Switches and buttons:** `Switch2Pos`, `Switch3Pos`, `SwitchMultiPos`, `ActionButton`,
      `SwitchWithCover2Pos`
    - **Rotary encoders:** `RotaryEncoder`, `RotaryAcceleratedEncoder`
    - **Knobs and sliders:** `AnalogInput`, `AnalogMultiPos`, `AngleSensorInput`
    - **Lights:** `LED`
    - **Gauge needles:** `NeedleGauge`
    - **Dimmable backlights:** `Dimmer`

    Two outputs don't need a pin address at all. `DrumDisplay` plugs straight into an I²C
    cable, and `IntegerOutput` hands its value to your own code.

??? info "Going further"
    `PIN_NC` means "nothing connected". You'll use it for an optional pin you don't need, such
    as a gauge's home sensor.

    Some controls check their address when the board starts up. A `Dimmer` given an expander
    pin, for example, prints a message on the debug port and stays off, rather than half-working
    and leaving you guessing.

    Everything a `PinRef` can do is in the [API reference](../../api/class_pin_ref.md).
