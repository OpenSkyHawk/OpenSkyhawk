# AnalogOutput — the family behind Dimmer and IntegerOutput

`AnalogOutput` isn't a part you wire up, and you'll never write it in your sketch. It's the name
of a small family of outputs that all do the same kind of job: take one value from the cockpit,
such as how far the lighting knob is turned, and do something with it. You'll see the name in the
API reference and in error messages, so this page explains where it fits.

!!! tip "Looking for a part?"
    - **A dimmable backlight?** Use [Dimmer](dimmer.md).
    - **A value for your own code**, like a custom screen? Use [IntegerOutput](integeroutput.md).
    - **A warning lamp** that is simply on or off? Use [LED](led.md).
    - **A gauge needle?** Use [NeedleGauge](needlegauge.md).

## The family

There are two members, and each one does something different with the value:

| Member | What it does with the value |
|---|---|
| [Dimmer](dimmer.md) | Sets the brightness of a backlight |
| [IntegerOutput](integeroutput.md) | Hands it to a function you write |

Everything else about them is the same, so they share it. Both listen for their cockpit value,
and both ignore a value that hasn't changed. That means a dimmer doesn't reset its brightness, and
your function doesn't run again, just because DCS sent the same number twice. The one difference
is that an `IntegerOutput` can pick its value out of a shared group, the way an [LED](led.md)
picks out its lamp, while a `Dimmer` always takes the whole value.

You don't need to know any of this to use them. Write `Dimmer` or `IntegerOutput` in your sketch,
exactly as their own pages show, and the shared part comes along automatically.

## Troubleshooting

**The build fails with an error about `AnalogOutput`.**
The sketch is trying to create an `AnalogOutput` directly, which isn't possible. Change it to
[Dimmer](dimmer.md) or [IntegerOutput](integeroutput.md), whichever fits your part.

??? info "Going further"
    Gauge needles aren't in the family, even though they also follow one cockpit value. A needle
    needs calibration and smooth movement on top, so it has its own class,
    [NeedleGauge](needlegauge.md).

    The family picks a value out of its group using an address, a mask and a shift — the same three
    numbers DCS-BIOS itself uses — so values packed several to a group work as well as ones that
    fill a whole group. [IntegerOutput](integeroutput.md) shows how to use the mask and shift.

    Every detail of the class is in the
    [API reference](../../../api/classOpenSkyhawk_1_1AnalogOutput.md).
