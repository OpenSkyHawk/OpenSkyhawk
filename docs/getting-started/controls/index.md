# Controls

A PanelGroup sketch is mostly a list of controls: one line for every switch, knob, light and
gauge on your panel. Each line says what the part is, which cockpit control it belongs to, and
where it is wired. Once a control is in your sketch, the board takes care of the rest — it reads
your switches and tells DCS, and it listens to DCS and drives your lights and gauges.

These pages walk through every control, one page each, starting from the physical part in your
hand. If you're new, read [Pin addresses](pin-addresses.md) first, since every control uses
them, and [Expanders](expanders.md) once you run out of pins on the board.

## Which control do I need?

Find the part you're holding in the left column.

### Inputs — parts you touch

| Your part | What it does in the cockpit | Use |
|---|---|---|
| Toggle switch, slide switch, or push-button | Copies your part exactly | [Switch2Pos](inputs/switch2pos.md) |
| Three-position toggle (ON–OFF–ON) | Copies your part exactly | [Switch3Pos](inputs/switch3pos.md) |
| Rotary selector switch, one wire per position | Turns to the same position | [SwitchMultiPos](inputs/switchmultipos.md) |
| Rotary selector on a resistor ladder (one wire) | Turns to the same position | [AnalogMultiPos](inputs/analogmultipos.md) |
| Push-button, for a cockpit switch that stays put | Each press flips the switch | [ActionButton](inputs/actionbutton.md) |
| Switch, for a cockpit switch with a guard cover | Opens the cover, moves the switch | [SwitchWithCover2Pos](inputs/switchwithcover2pos.md) |
| Rotary encoder (turns forever, clicks as it turns) | Steps a selector or nudges a knob | [RotaryEncoder](inputs/rotaryencoder.md) |
| Rotary encoder that you spin quickly | Same, with bigger steps when spun fast | [RotaryAcceleratedEncoder](inputs/rotaryacceleratedencoder.md) |
| Potentiometer knob or slider | Follows your knob smoothly | [AnalogInput](inputs/analoginput.md) |
| Magnetic angle sensor under a knob | Follows your knob smoothly | [AngleSensorInput](inputs/anglesensorinput.md) |

!!! tip "Selector switch or rotary encoder?"
    Ask whether your knob **knows where it is pointing**. A selector switch with a pointer or marked
    positions always does, so when the sim starts up the board tells DCS exactly where it is and the
    cockpit matches it — use [SwitchMultiPos](inputs/switchmultipos.md) for up to 12 positions, or
    [AnalogMultiPos](inputs/analogmultipos.md) for more. A rotary encoder doesn't know where it is
    pointing at all; it can only say "one step up" or "one step down", so there is nothing to match
    at startup. Use an encoder only for cockpit knobs that have no marked position.

    For example, the UHF radio's mode selector has a pointer, so it's a `SwitchMultiPos`. The
    radio's frequency knobs turn continuously with nothing to show where they are — the frequency
    is read off the display — so they're rotary encoders.

### Outputs — parts the sim drives

| Your part | What the sim does with it | Use |
|---|---|---|
| Indicator lamp or LED | Turns it on and off | [LED](outputs/led.md) |
| Backlight LED strip | Dims it with the cockpit lighting knob | [Dimmer](outputs/dimmer.md) |
| Gauge with a stepper-motor needle | Moves the needle | [NeedleGauge](outputs/needlegauge.md) |
| Small OLED screen | Shows a rolling-drum number readout | [DrumDisplay](outputs/drumdisplay.md) |
| Anything else — your own display or gadget | Hands the value to your own code | [IntegerOutput](outputs/integeroutput.md) |

`Dimmer` and `IntegerOutput` belong to one family; [AnalogOutput](outputs/analogoutput.md)
explains what they share.

## The debug stream

While you're getting a panel working, the board can print what it's doing — every switch flip and
every knob reading — to a serial monitor on your computer. You turn it on with
`STM32Board::setDebug(true);` in `setup()`, and [Debugging](../../firmware/debugging.md) explains
how to connect. It's the easiest way to check your wiring and to measure the numbers some pages
ask you for.

Turn it off again once your panel works, by deleting that line. Each printed line takes the board
a moment to send, and while lots is happening it holds everything else up. Switches don't notice,
and knobs barely do, but a joystick axis loses the quick response it's tuned for, and a busy
board can fall behind.

??? info "Coming from DCS-BIOS?"
    The class names follow the DCS-BIOS Arduino library wherever it has an equivalent, so a
    DCS-BIOS sketch translates almost line for line. The one deliberate gap is `RotarySwitch`:
    use a [RotaryEncoder](inputs/rotaryencoder.md) in DIR mode instead, which can't jump the sim
    away from a mission's preset position when the panel starts up.
