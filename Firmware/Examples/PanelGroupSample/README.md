# PanelGroupSample — a working PanelGroup node

A small sketch that runs as a real PanelGroup node: one of each common control, wired straight to
the STM32's own pins, so it works on a bare PanelGroup base board with no expander or other parts
fitted. The controls are the ones the
[Getting Started → Controls](https://openskyhawk.github.io/OpenSkyhawk/getting-started/controls/)
pages walk through, put together in one sketch that compiles.

There is no prebuilt PanelGroup firmware in the release, because every panel group has its own
NODE_ID and its own controls. You build your own from source, and this sketch is a known-good place
to start. For a blank project with every class listed in comments, use
[`Firmware/Templates/PanelGroup`](../../Templates/PanelGroup/) instead.

**How it differs from [`E2E_DCS_Test`](../E2E_DCS_Test/):** that folder is the project's
live-DCS test rig, a set of sketches for three boards that drives every control class, including a
stepper gauge and OLED displays, and needs all of that hardware connected. This sample is one board,
the everyday control classes, and nothing beyond a few switches.

## Wiring

All pins are on the base board's `J_BREAKOUT` header. Each switch, the button and the encoder
connect their pin to GND when closed and need a **10 kΩ resistor from the pin to 3.3V**.

| Control | Class | Cockpit control | Pin (`J_BREAKOUT`) |
|---|---|---|---|
| Master arm toggle | `Switch2Pos` | `DCSIN_ARM_MASTER` | PB0 (3) |
| Bomb arm 3-position toggle | `Switch3Pos` | `DCSIN_ARM_BOMB` | PA3 (10), PA4 (12) |
| Taxi light push button | `ActionButton` | `DCSIN_LIGHT_EXT_TAXI` | PB1 (5) |
| ARC-51 volume potentiometer | `AnalogInput` | `DCSIN_ARC51_VOL` | PA0 (4), wiper. Ends to 3.3V and GND |
| Destination latitude encoder | `RotaryEncoder` | `DCSIN_DEST_LAT_KNB` | PA1 (6) = A, PA2 (8) = B, C to GND |
| Fire warning light | `LED` | `A_4E_C_D_GLARE_FIRE` | PA5 (9), through 330 Ω to the LED, to GND |

PB2 (7) and PC14 (11) are left free.

## Build and flash

Copy this folder to `Firmware/Panels/<YourPanel>/`, either in your clone or in the
**Source code** archive of a firmware release. The paths in `platformio.ini` work from there
unchanged. Then take the next free NODE_ID from `Firmware/NODE_IDS.md`, register it there, and set
it in `platformio.ini`. The `1` in the sample is reserved, and `tools/check_node_ids.py` fails on a
copy in `Panels/` whose ID isn't registered (Step 1 of
[Adding a New Panel Group](../../../docs/guides/new-panel-group.md)). With an ST-Link on the
board's SWD header:

```bash
pio run -t bootstrap   # a new board: the bootloader + this sketch, once
pio run -t upload      # after that: this sketch only
```

On a new board, after `bootstrap` the status LED blinks red only: the firmware is running and
looking for the bus. Red and green blinking together means the board is still in the bootloader.
See [Flashing](https://openskyhawk.github.io/OpenSkyhawk/guides/flashing/) for ST-Link
troubleshooting.
