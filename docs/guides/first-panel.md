# Your First Panel

The fastest way to understand OpenSkyhawk is to run the reference example. `E2E_DCS_Test` is a
minimal, working PanelGroup sketch — **one LED and one switch**, wired to real A-4E-C DCS-BIOS
IDs. Get this blinking and you understand the whole input/output path.

## What it does

The example lives at `Firmware/Examples/E2E_DCS_Test/PanelGroup/`. The entire sketch:

```cpp
#include <OpenSkyhawk.h>

const PinRef PIN_LED(PC13);   // dev-board built-in LED (active-LOW) — use an external LED in production
const PinRef PIN_BTN(PB0);    // pushbutton to GND; 10 kΩ pull-up to 3.3V

OpenSkyhawk::LED        gearLight (A_4E_C_GEAR_LIGHT, A_4E_C_GEAR_LIGHT_AM, PIN_LED, /*reverse=*/true);
OpenSkyhawk::Switch2Pos masterTest(DCSIN_MASTER_TEST, PIN_BTN);

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
```

Two control objects, declared at global scope (they self-register), and a two-line
`setup()`/`loop()`. That's a panel.

- **Output:** `gearLight` watches the DCS-BIOS gear-light address (`A_4E_C_GEAR_LIGHT` + its mask
  `_AM`) and drives the LED. `reverse=true` because the dev-board LED is active-LOW.
- **Input:** `masterTest` reads the button and fires the compact `DCSIN_MASTER_TEST` command — in
  DCS, holding Master Test illuminates the gear light, so pressing the button lights the LED
  through the full round trip.

## Wiring

| Signal | Pin | Notes |
|--------|-----|-------|
| LED | PC13 | dev-board built-in, active-LOW (use an external LED + resistor on a real panel) |
| Button | PB0 | other side to GND; **10 kΩ pull-up to 3.3 V** |

## Run it

1. Open `Firmware/Examples/E2E_DCS_Test/PanelGroup/` in PlatformIO.
2. Confirm `-DNODE_ID=1` in `platformio.ini` (this example is NODE 1).
3. Flash over ST-Link — see [Flashing Firmware](flashing.md).
4. Watch DiagSerial at 115200 (`STM32Board::setDebug(true)` is on) — see
   [Debugging on STM32](../firmware/debugging.md).
5. With the full stack (PanelBridge + SimGateway) connected and DCS-BIOS running, press the
   button: the LED follows Master Test. See [Bring-Up & Testing](bring-up.md).

## Then go further

You've now seen the two halves — an `LED` output and a `Switch2Pos` input, routed by their IDs.
To build your own panel group from here, follow
[Adding a New Panel Group](new-panel-group.md). For what each control class can do, see
[Control Types](../firmware/control-types.md).
