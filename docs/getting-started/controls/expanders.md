# Expanders — more pins

## Why you need them

Your PanelGroup board has **10 free pins** for controls. A real cockpit panel needs more than
that — the armament panel alone has over 30 switches, knobs and lights.

An **expander** is a small chip that adds more pins. Think of a power strip: the wall has one
socket, and the strip turns it into six.

```
 PanelGroup board ══ one cable ══▶ expander ──┬── switch
                                              ├── switch
                                              ├── switch
                                              └── … and more
```

- The expander usually sits on the panel's own circuit board, right behind the switches. For
  experimenting on your desk, a cheap ready-made module works too.
- The board talks to it over **one cable**, instead of one wire per switch.
- You wire a switch to an expander **exactly** like you wire it to the board.
- In your sketch, only the switch's [pin address](pin-addresses.md) changes.

## Which kind?

| | **Digital expander** | **Shift-register chain** | **Analog expander** |
|---|---|---|---|
| The chip | MCP23017 | 74HC165 (in) · 74HC595 (out) | ADS1115 |
| Plugs into | `J_I2C1` or `J_I2C2` | `J_SR` | `J_I2C1` or `J_I2C2` |
| Best for | switches, buttons, lights | **rotary encoders**, fast gauge needles, lots of switches or lights | **knobs and sliders** (potentiometers), angle sensors |
| Pins per chip | 16 | 8 | 4 |
| How many | up to 8 per board | up to 8 input chips and 8 output chips | up to 4 per cable, 8 per board |
| Board version | any | **0.2.0 or later** | any |

**The quick rule:** rotary encoders or fast-moving needles → shift registers. Knobs that turn a
potentiometer → analog expander. Everything else → digital expander.

You can mix them freely. One sketch can use the board's own pins, two digital expanders and a
shift-register chain all at once.

## Digital expander (MCP23017)

**1. Name it, with its address.** Up to 8 can share one cable, and each needs its own address,
from `0x20` to `0x27`. You set it with the chip's three address pins (`A0`–`A2`); with all three
tied to GND it is `0x20`. Put this above `setup()`:

```cpp
MCP23017 expander1(0x20, Wire);
```

**2. Introduce it to the board**, inside `setup()`:

```cpp
void setup() {
    Wire.begin();
    PanelGroup::registerExpander(expander1, PB12, PB13);
    PanelGroup::setup();
}
```

`PB12` and `PB13` are the expander's two **interrupt** wires — pins 6 and 7 of `J_I2C1`. The
expander uses them to tap the board on the shoulder the moment a switch changes. Several
expanders on the same cable can share them.

**3. Use its pins.** They are named `GPA0`–`GPA7` (port A) and `GPB0`–`GPB7` (port B):

```cpp
const PinRef GEAR_HANDLE_PIN = PinRef(expander1, PORT_B, 2);   // GPB2
```

!!! warning "GPA7 and GPB7 can't read switches"
    A fault in the chip itself means these two pins only work as outputs. Use them for lights.
    That leaves 14 pins per expander for switches.

!!! warning "Switches still need their pull-up resistor"
    The expander doesn't add one. Every switch needs its 10 kΩ resistor from the pin to 3.3V,
    just like on the board.

## Shift-register chain (74HC165 / 74HC595)

**There is nothing to set up.** Add one line at the top of your sketch, then write addresses:

```cpp
#include <Helpers/ShiftBus/ShiftBus.h>

const PinRef MASTER_ARM_PIN  = PinRef(ShiftBus1, 0, 3);   // input chip 0, pin 3
const PinRef CAUTION_LED_PIN = PinRef(ShiftBus1, 0, 3);   // output chip 0, pin 3
```

- **Count chips from the board outward**, starting at 0. Each chip has pins 0–7.
- **Input chips (74HC165) and output chips (74HC595) are counted separately.** The same numbers
  mean a different pin depending on whether a switch or a light uses them — the two lines above
  are two different wires.
- **No resistors to add.** Shift-register boards built to our standard already have the
  pull-up on every input.

## Analog expander (ADS1115)

**1. Name it** above `setup()`:

```cpp
ADS1115 adc1;
```

**2. Introduce it to the board** inside `setup()`, with its address — `0x48` to `0x4B`, set by
its `ADDR` pin, so up to 4 per cable:

```cpp
void setup() {
    Wire.begin();
    PanelGroup::registerADC(adc1, 0x48);
    PanelGroup::setup();
}
```

**3. Use its inputs**, `A0`–`A3`:

```cpp
const PinRef FLOOD_KNOB_PIN = PinRef(adc1, 0);   // input A0
```

??? info "Going further"
    - **Using the second cable (`J_I2C2`).** Declare the second bus above `setup()` —
      `TwoWire Wire1(PB11, PB10);` — call `Wire1.begin()` in `setup()`, and pass `Wire1`
      where the examples above say `Wire`. Its interrupt wires are `PA8` and `PA15` on board
      0.2.0, and `PB8` and `PB9` on board 0.1.0.
    - **No interrupt wires?** `PanelGroup::registerExpander(expander1);` on its own still works.
      The board then checks the expander every 20 ms instead of hearing about changes instantly.
    - The limits above are per **board**, not per cable — using both cables doesn't double them.
