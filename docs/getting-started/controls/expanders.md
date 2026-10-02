# Expanders — more pins

## Why you need them

Your PanelGroup board has 10 free pins for controls, which is plenty for a first experiment but
not for a real panel — the armament panel alone has more than 30 switches, knobs and lights. An
**expander** solves this. It's a small chip that adds more pins, a bit like a power strip that
turns one wall socket into six.

```
 PanelGroup board ══ one cable ══▶ expander ──┬── switch
                                              ├── switch
                                              ├── switch
                                              └── … and more
```

The expander usually sits on the panel's own circuit board, right behind the switches, and
connects back to the PanelGroup board with a single cable. For experimenting on your desk, a
cheap ready-made module works just as well.

The good news is that very little changes for you. You wire a switch to an expander exactly the
way you'd wire it to the board, and in your sketch only its [pin address](pin-addresses.md)
changes.

## Which kind?

There are three kinds of expander, and each is good at something different.

| | **Digital expander** | **Shift-register chain** | **Analog expander** |
|---|---|---|---|
| The chip | MCP23017 | 74HC165 (in) · 74HC595 (out) | ADS1115 |
| Plugs into | `J_I2C1` or `J_I2C2` | `J_SR` | `J_I2C1` or `J_I2C2` |
| Best for | switches, buttons, lights | encoders, fast needles | knobs and sliders |
| Pins per chip | 16 | 8 | 4 |
| How many | 8 per board | 8 in + 8 out | 4 per cable, 8 per board |
| Board version | any | 0.2.0 or later | any |

If you're not sure which to pick, a simple rule works for almost every panel. Rotary encoders and
fast-moving gauge needles fit best on **shift registers**, because a spinning encoder has to be
read quickly and a fast needle has to be stepped quickly. A digital expander can still read an
encoder as long as its interrupt wires are connected, but it moves a needle more slowly. Knobs
that turn a potentiometer go on an **analog expander**, because they need a position rather than
on or off. Everything else goes on a **digital expander**.

You can also mix them freely. One sketch can use the board's own pins, two digital expanders and
a shift-register chain all at the same time.

## Digital expander (MCP23017)

A digital expander needs three things in your sketch: a name, an introduction to the board, and
then pin addresses for each wire.

**Give it a name.** Up to eight can share one cable, so each one needs its own address between
`0x20` and `0x27`. You set it with the chip's three address pins, `A0` to `A2`; with all three
connected to GND, the address is `0x20`. Put this line above `setup()`:

```cpp
MCP23017 expander1(0x20, Wire);
```

**Introduce it to the board** inside `setup()`. This tells the board the expander exists and
which two wires it uses to signal a change:

```cpp
void setup() {
    Wire.begin();
    PanelGroup::registerExpander(expander1, PB12, PB13);
    PanelGroup::setup();
}
```

`PB12` and `PB13` are the expander's **interrupt** wires, on pins 6 and 7 of `J_I2C1`. The
expander uses them to tap the board on the shoulder the moment a switch changes, so the board
doesn't have to keep asking. Several expanders on the same cable can share the same two wires.

**Use its pins.** The expander's pins are named `GPA0` to `GPA7` (port A) and `GPB0` to `GPB7`
(port B), and you write them like this:

```cpp
const PinRef GEAR_HANDLE_PIN = PinRef(expander1, PORT_B, 2);   // GPB2
```

!!! warning "GPA7 and GPB7 can't read switches"
    Because of a fault in the chip itself, these two pins only work as outputs. Use them for
    lights, which leaves 14 pins on each expander for switches. Those switches still need their
    10 kΩ pull-up resistor, exactly as on the board — the expander doesn't add one for you.

## Shift-register chain (74HC165 / 74HC595)

Shift registers are the easiest kind to use, because there's nothing to set up at all — no name,
no introduction to the board. You just write pin addresses, using the chain's built-in name
`ShiftBus1`:

```cpp
const PinRef MASTER_ARM_PIN  = PinRef(ShiftBus1, 0, 3);   // input chip 0, pin 3
const PinRef CAUTION_LED_PIN = PinRef(ShiftBus1, 0, 3);   // output chip 0, pin 3
```

The chips are numbered from the board outward, starting at 0, and each chip has pins 0 to 7.
There are two kinds of chip — 74HC165s read switches and 74HC595s drive lights — and each kind is
counted separately. That's why the two lines above can use the same numbers: one is pin 3 on the
first input chip, the other is pin 3 on the first output chip, and they're two different wires.

You also don't need to add any resistors. Shift-register boards built to our standard already
have a pull-up on every input.

## Analog expander (ADS1115)

An analog expander follows the same pattern as a digital one. First give it a name above
`setup()`:

```cpp
ADS1115 adc1;
```

Then introduce it to the board inside `setup()`, along with its address. The address is `0x48`
to `0x4B`, set by the chip's `ADDR` pin, so up to four can share one cable:

```cpp
void setup() {
    Wire.begin();
    PanelGroup::registerADC(adc1, 0x48, Wire);
    PanelGroup::setup();
}
```

It has four inputs, `A0` to `A3`, and you use them like any other pin:

```cpp
const PinRef FLOOD_KNOB_PIN = PinRef(adc1, 0);   // input A0
```

One thing to know for now: a knob on the analog expander tops out at a reading of about 52800,
not the full 65535 a board pin reaches. Tell the class where the top is, as described in
[Setting the range](inputs/analoginput.md#setting-the-range), and the cockpit control will still
reach its end.

??? info "Going further"
    **Using the second cable.** To put expanders on `J_I2C2`, declare the second bus above
    `setup()` with `TwoWire Wire1(PB11, PB10);`, call `Wire1.begin()` in `setup()`, and pass
    `Wire1` wherever the examples above say `Wire`. Its interrupt wires are `PA8` and `PA15` on
    board 0.2.0, and `PB8` and `PB9` on board 0.1.0.

    **No interrupt wires?** Calling `PanelGroup::registerExpander(expander1);` on its own still
    works. The board then checks the expander every 20 ms instead of hearing about changes
    straight away.

    **The limits are per board.** Using both cables doesn't double them — a board can still have
    at most eight digital and eight analog expanders in total.
