# IntegerOutput — a value for your own code

`IntegerOutput` is for the parts no built-in class covers: a small screen, a custom readout, or
anything else you want to drive with your own code. It takes one value from the cockpit, such as
the fuel quantity, and hands it to a function you write. Every time the value changes in DCS, your
function runs again with the new number, and what it does with it is up to you.

!!! tip "Not quite the right part?"
    - **A warning lamp** that is simply on or off? Use [LED](led.md).
    - **A dimmable backlight?** Use [Dimmer](dimmer.md).
    - **A gauge needle?** Use [NeedleGauge](needlegauge.md).
    - **A number drum or a digit readout?** Use [DrumDisplay](drumdisplay.md).

## In your sketch

```cpp
void showFuel(uint16_t value) {
    if (STM32Board::isDebug()) {
        STM32Board::diagSerial().print("Fuel: ");
        STM32Board::diagSerial().println(value);
    }
}

OpenSkyhawk::IntegerOutput fuelReadout(A_4E_C_D_FUEL, showFuel);
```

This goes near the top of your sketch, above `setup()`. The first part is your own function:
here it simply prints the value on the board's [debug port](../../../firmware/debugging.md), which
is a good way to watch a value before you build anything to show it. The `if` line means it only
prints while debugging is turned on, with `STM32Board::setDebug(true);` in `setup()` — handy
while you build, but [turn it off again](../index.md#the-debug-stream) once your display works.

The last line connects the two. `fuelReadout` is a name you choose, `A_4E_C_D_FUEL` is the cockpit
value to follow (the fuel gauge), and `showFuel` is the function to hand it to. The function has to
come first in the sketch, so that the line below it knows what `showFuel` is. Every cockpit value
has a name like this; [DCS-BIOS Integration](../../../firmware/dcsbios-integration.md) explains
where to find them.

## Wiring

There's nothing to wire. `IntegerOutput` doesn't use a pin of its own — whatever your function
drives, such as a screen, is wired and set up the way that part's own instructions say.

## Troubleshooting

**Nothing is printed.**
Check that `STM32Board::setDebug(true);` is in `setup()` and that your serial monitor is open at
115200. Your function runs when the value first arrives and after that only when it changes, so
nothing new appears while the value holds steady.

**Other lights and gauges on the board are slow to respond.**
Your function is taking too long. Everything else on the board waits while it runs, so keep it
short and never use `delay()` inside it.

??? info "Going further"
    The value is a whole number from 0 to 65535. For a gauge like the fuel quantity, 0 is the
    needle at its lowest point and 65535 is the needle at full scale, so you'll usually scale it to
    whatever your display needs.

    A few values share their group with others, the way an [LED](led.md)'s lamps do — the ARC-51
    radio's frequency knobs, for example. For those, add the `_AM` name as a third argument and a
    shift as a fourth, which slides the value down so it starts from 0. The shift is the number of
    zeros at the low end of the `_AM` mask written in binary:

    ```cpp
    void showMhz(uint16_t value) {
        if (STM32Board::isDebug()) {
            STM32Board::diagSerial().print("MHz knob: ");
            STM32Board::diagSerial().println(value);
        }
    }

    // A_4E_C_ARC51_FREQ_1MHZ_AM is 0x3c00 = 0011 1100 0000 0000 → ten zeros at the end
    OpenSkyhawk::IntegerOutput mhzKnob(A_4E_C_ARC51_FREQ_1MHZ, showMhz,
                                       A_4E_C_ARC51_FREQ_1MHZ_AM, 10);
    ```

    `IntegerOutput` belongs to a small family of outputs that each take one cockpit value;
    [AnalogOutput](analogoutput.md) explains what they share.

    Every detail of the class is in the
    [API reference](../../../api/classOpenSkyhawk_1_1IntegerOutput.md).
