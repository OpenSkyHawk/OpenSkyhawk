# Example — PanelGroup Sketch: Center_Armament

> **Note on identifiers:** All `DCSIN_*` compact command IDs and `A_4E_C_*_A` address constants
> shown here are representative. `DCSIN_*` constants come from the generated `A4EC_CmdIds.h`
> header; `A_4E_C_*_A` constants come from the DCS-BIOS `Addresses.h` header. Always verify
> against the generated headers — do not hard-code values from this example.

Runs on the STM32 node for the Center Armament panel. Emits and receives `{controlId, value}`
packets over CAN. No DCS-BIOS or Joystick knowledge — routing is determined entirely by the
`controlId` range: generated `DCSIN_*` IDs (`0x8000`-`0x86FF`) → DCS-BIOS path;
`< 0x8000` → HID path.

Companion `platformio.ini` excerpt:

```ini
[env:center_armament]
platform = ststm32
board = genericSTM32F103CB
framework = arduino
build_flags = -DNODE_ID=3
lib_deps =
    file://../Libraries/PanelGroup
```

```cpp
// NODE_ID is set in platformio.ini, not here:
//   build_flags = -DNODE_ID=3
// A #define in main.cpp would be invisible to library translation units.

#include <A4EC_CmdIds.h>     // generated: #define DCSIN_* constants
#include <Addresses.h>       // DCS-BIOS: A_4E_C_*_A output address constants
#include <PanelGroup.h>

// --- Hardware ---
AS5600Sensor rollSensor(Wire);          // AS5600 angle sensor on I2C1
MCP23017     exp1(0x20, Wire);          // GPIO expander — I2C address 0x20

// --- Wiring map (mirrors schematic net labels — no magic numbers below) ---
//
// Each bit position appears ONCE here, directly traceable to the schematic.
// All control declarations below use these names only.

// Output pins (DCS → hardware)
const PinRef PIN_MASTER_CAUT (PB0);              // master caution LED — direct GPIO
const PinRef PIN_INSTR_BRT   (PB9);              // instrument backlight PWM — direct GPIO

// Switch inputs (hardware → DCS, DCSIN_* control IDs)
const PinRef PIN_MASTER_ARM  (exp1, PORT_A, 0);  // master arm switch
const PinRef PIN_FUEL_SEL_0  (exp1, PORT_A, 1);  // 3-pos fuel selector: position 0
const PinRef PIN_FUEL_SEL_1  (exp1, PORT_A, 2);  // 3-pos fuel selector: position 2

// HID button inputs (hardware → HID, controlId < 0x8000)
const PinRef PIN_TRIGGER     (exp1, PORT_B, 0);  // trigger button
const PinRef PIN_NWS         (exp1, PORT_B, 1);  // nose wheel steering button

// LED mask
constexpr uint16_t MASK_MASTER_CAUT = 0x4000;   // bit inside DCS value that drives this LED

// --- Outputs (DCS → hardware) ---
// CTRL_BCAST arrives over CAN with a DCS-BIOS address → PanelGroup drives hardware.
// SimGateway needs no declarations for outputs — broadcast automatically.
OpenSkyhawk::LED          masterCaution(A_4E_C_MASTER_CAUTION_A,  MASK_MASTER_CAUT, PIN_MASTER_CAUT);
OpenSkyhawk::AnalogOutput instrBrt     (A_4E_C_LIGHT_INT_INSTR_A,                   PIN_INSTR_BRT);

// --- Inputs (hardware → DCS-BIOS, DCSIN_* control IDs) ---
// PanelBridge receives CAN EVT → binary search in A4EC_InputMap → sendDcsBiosMessage()
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PIN_MASTER_ARM);
OpenSkyhawk::Switch3Pos fuelSel  (DCSIN_FUEL_SEL,   PIN_FUEL_SEL_0, PIN_FUEL_SEL_1);

// --- Inputs (hardware → HID, controlId < 0x8000) ---
// PanelBridge receives CAN EVT → HID frame → SimGateway → Joystick setter
// Same input classes as DCS-BIOS controls — routing determined by controlId only.

// Axis input via hall-effect sensor:
// centerDeg = AS5600 angle at neutral stick position; travelDeg = ± mechanical travel
OpenSkyhawk::AngleSensorInput rollAxis(CTRL_ROLL, rollSensor, 180.0f, 90.0f);

// Button inputs — same Switch2Pos class, CTRL_* controlId routes to HID instead of DCS-BIOS:
OpenSkyhawk::Switch2Pos trigger  (CTRL_TRIGGER,  PIN_TRIGGER);   // trigger → Joystick.button(0)
OpenSkyhawk::Switch2Pos nwsBtn   (CTRL_NWS,      PIN_NWS);       // nose wheel steering button

void setup() {
    STM32Board::setDebug(true);   // remove in production
    Wire.begin();                 // init I2C1 before PanelGroup::setup()
    PanelGroup::registerExpander(exp1, PA4, PA5);    // INTA=PA4, INTB=PA5
    PanelGroup::setup();
    // PanelGroup::setup() drives ~SLEEP HIGH and homes any registered SwitecX25Output motors.
    // It also reads full port state of all expanders (baseline) and sends initial CAN EVTs.
}

void loop() {
    PanelGroup::loop();
    // Polls interrupt flags, dispatches input events, updates stepper motors, sends heartbeat.
}
```

## Data Flow (this node)

```
MASTER CAUTION LED  (DCS → this node)
  DCS state changes → DCS-BIOS stream → USB CDC → SimGateway
  → byte relay → UART → PanelBridge
  → ExportStreamListener(A_4E_C_MASTER_CAUTION_A, value) fires
  → PanelBridge broadcasts CTRL_BCAST over CAN
  → this node receives → masterCaution.onValue(value) → drive PB0 HIGH/LOW

MASTER ARM switch  (this node → DCS)
  Switch toggles → MCP23017 INT fires → PanelGroup reads INTCAP
  → Switch2Pos emits CAN EVT {DCSIN_ARM_MASTER, 0 or 1}
  → PanelBridge receives → binary search in A4EC_InputMap
  → sendDcsBiosMessage("ARM_MASTER", "0"/"1") → ASCII on UART
  → SimGateway relays → USB CDC → DCS receives command

Roll axis  (this node → HID)
  AS5600 angle changes → AngleSensorInput polls every 8 ms
  → emits CAN EVT {CTRL_ROLL, 0–65535}
  → PanelBridge receives → controlId 0x0010 < 0x8000 → HID frame → UART
  → SimGateway sees 0xAA 0x55 → parse → walk HIDAxis list
  → roll.dispatch(value) → Joystick.X(value) → Joystick.send()
  → USB HID report → DCS receives roll axis input
```
