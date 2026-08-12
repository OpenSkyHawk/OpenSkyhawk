# Example — SimGateway Sketch

SimGateway does not run the DCS-BIOS library. It relays the raw DCS-BIOS byte stream between
USB CDC and UART, and intercepts HID frames for HID dispatch. The only declarations in the
sketch are `HIDAxis` and `HIDButton` objects.

| Control | `controlId` | Direction | Handled by |
|---------|-------------|-----------|------------|
| MASTER CAUTION light | `A_4E_C_MASTER_CAUTION_A` (`0x8000`-`0x86FF`) | DCS → hardware | PanelBridge ExportStreamListener (automatic) |
| MASTER ARM switch | `DCSIN_ARM_MASTER` (`0x8000`-`0x86FF`) | hardware → DCS | PanelBridge input map (automatic) |
| Roll axis | `CTRL_ROLL` (0x0010, < 0x8000) | hardware → HID | SimGateway HIDAxis (declared in sketch) |

```cpp
#include <SimGateway.h>
#include <HIDControls.h>

// --- DCS ↔ Cockpit ---
// No declarations needed. SimGateway relays the raw DCS-BIOS stream between USB CDC
// and UART transparently. PanelBridge handles all DCS-BIOS parsing and dispatch.

// --- HID (controlId < 0x8000, arrives as HID frame from PanelBridge) ---
// Constructor takes (controlId, axisIndex 0–7). The sketch names no HID backend and does
// no scaling: the library applies the stored calibration and the 0–65535 → ±32767 mapping
// inside HIDAxis::dispatch(). Declare at file scope so the objects self-register before
// setup() runs.
OpenSkyhawk::HIDAxis roll    (CTRL_ROLL,     0);
OpenSkyhawk::HIDAxis pitch   (CTRL_PITCH,    1);
OpenSkyhawk::HIDAxis throttle(CTRL_THROTTLE, 2);
OpenSkyhawk::HIDAxis rudder  (CTRL_RUDDER,   3);
OpenSkyhawk::HIDAxis brakeL  (CTRL_BRAKE_L,  4);
// OpenSkyhawk::HIDAxis   <name>(controlId, axisIndex);    // 0–7
// OpenSkyhawk::HIDButton <name>(controlId, buttonIndex);  // 0–127

void setup() {
    // SimGateway::setup() owns everything: USB identity (VID/PID, manufacturer, product, and
    // the CDC interface name "A-4E Skyhawk DCS-BIOS"), the HID descriptor, the UART, and
    // loading axis calibration from flash. The sketch configures none of it.
    SimGateway::setup(Serial1);     // UART link to PanelBridge @ 250000 baud
}

void loop() {
    SimGateway::loop();
    // Each iteration:
    //   1. Forward all available bytes from USB CDC → UART (raw DCS-BIOS stream to PanelBridge)
    //   2. Read all available bytes from UART:
    //        byte <= 0x7F → forward to USB CDC (DCS-BIOS stream from PanelBridge)
    //        byte == 0xAA, next == 0x55 → HID frame: read 4 more bytes, parse controlId+value,
    //                                      dispatch to HIDAxis/HIDButton linked list
    //        byte == 0xAA, next != 0x55 → forward both bytes to USB CDC, resume scanning
    //   3. If any HID setter fired this iteration → send one HID report
}
```

> The canonical, compiling version of this sketch is `Firmware/Templates/SimGateway/src/main.cpp`.
> Prefer copying that; this page exists to show the sketch alongside the data flow below.

## Full Data Flow

```
MASTER CAUTION LED  (DCS → cockpit)
  DCS state changes
  → DCS-BIOS binary stream → USB CDC
  → SimGateway relays raw bytes → UART
  → PanelBridge: DCS-BIOS library fires ExportStreamListener(A_4E_C_MASTER_CAUTION_A, value)
  → PanelBridge broadcasts CTRL_BCAST over CAN
  → Center_Armament PanelGroup node receives → masterCaution.onValue(value) → drive PB0

MASTER ARM switch  (cockpit → DCS)
  Switch toggles → MCP23017 INT fires → PanelGroup reads INTCAP
  → Switch2Pos emits CAN EVT {DCSIN_ARM_MASTER, 0 or 1}
  → PanelBridge receives CAN → binary search in A4EC_InputMap
  → sendDcsBiosMessage("ARM_MASTER", "0"/"1") → raw ASCII on UART
  → SimGateway: byte <= 0x7F → relay → USB CDC
  → DCS receives command

Roll axis  (cockpit → HID)
  AS5600 angle changes → AngleSensorInput polls every 8 ms
  → emits CAN EVT {CTRL_ROLL, 0–65535}
  → PanelBridge receives CAN → controlId 0x0010 < 0x8000
  → wrap in HID frame: 0xAA 0x55 0x10 0x00 <value_lo> <value_hi> → UART
  → SimGateway: 0xAA detected, next byte 0x55 confirmed → parse frame
  → walk HIDAxis list → CTRL_ROLL match → roll.dispatch(value)
  → apply stored calibration, then value − 32768 → write axis 0 of the HID report
  → after draining UART: send the report once
  → USB HID report → DCS receives roll axis input
```
