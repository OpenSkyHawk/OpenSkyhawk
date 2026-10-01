# Pin addresses (`PinRef`)

Every switch, knob and light in your sketch needs to know **where its wire is plugged in**.
You tell it with a **pin address**, written `PinRef(...)`. You will write one for every wire,
so it is worth five minutes here before anything else.

## It's an address

Think of a postal address.

- A wire on the **board itself** is like a house — one name, one place: `PinRef(PA0)`
- A wire on an **expander** is like an apartment — building, floor, door:
  `PinRef(expander1, PORT_A, 0)`

The switch is the mail carrier. It delivers to whatever address you give it, and it does not
care whether that is a house or an apartment. That is why every control works the same way no
matter where it is plugged in.

## Every kind of address

| The wire goes to… | You write | Example |
|---|---|---|
| A pin on the board | `PinRef(pin name)` | `PinRef(PA0)` — the name printed next to the pin |
| A [digital expander](expanders.md#digital-expander-mcp23017) | `PinRef(expander, port, pin)` | `PinRef(expander1, PORT_B, 2)` — pin `GPB2` |
| A [shift-register chain](expanders.md#shift-register-chain-74hc165-74hc595) | `PinRef(ShiftBus1, chip, pin)` | `PinRef(ShiftBus1, 0, 3)` — first chip, pin 3 |
| An [analog expander](expanders.md#analog-expander-ads1115) | `PinRef(adc, input)` | `PinRef(adc1, 0)` — input `A0` |

## Moving a wire? Change one line

Here is the same switch, plugged in three different places. Only the address changes — the
`Switch2Pos` line is identical every time.

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

Give every address a name, and put them all together at the top of your sketch — one line per
wire, named after what is on the end of it:

```cpp
// ── Where everything is wired ──────────────────────────────
const PinRef MASTER_ARM_PIN  = PinRef(PA0);
const PinRef GEAR_HANDLE_PIN = PinRef(expander1, PORT_A, 1);
const PinRef CAUTION_LED_PIN = PinRef(ShiftBus1, 0, 4);
```

If a wire moves, you fix one line. And the block doubles as your wiring list when you build the
harness.

## Which controls work where

| | Board pin | Digital expander | Shift register | Analog expander |
|---|:-:|:-:|:-:|:-:|
| Switches and buttons | ✅ | ✅ | ✅ | — |
| Rotary encoders | ✅ | ✅ | ✅ best | — |
| Knobs and sliders | ✅ analog pins | — | — | ✅ |
| Lights | ✅ | ✅ | ✅ | — |
| Gauge needles | ✅ | ✅ | ✅ fastest | — |
| Dimmable backlights | ✅ PWM pins | — | — | — |

**Why the gaps?**

- An analog expander measures *how far* something is turned. It can't read an on/off switch.
- A digital expander or shift register only sees on or off. It can't tell where a knob points.
- Dimming a light needs a pin that switches thousands of times a second. Only some of the
  board's own pins can, and they are marked on the board.

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
    - `PIN_NC` means "nothing connected". Use it for an optional pin you are not using, such as
      a gauge's home sensor.
    - Some controls check their address when the board starts. A `Dimmer` given an expander
      pin prints a message on the debug port and stays off, rather than half-working.
    - Everything a `PinRef` can do is in the [API reference](../../api/class_pin_ref.md).
