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

// --- DCS ↔ Cockpit ---
// No declarations needed. SimGateway relays the raw DCS-BIOS stream between USB CDC
// and UART transparently. PanelBridge handles all DCS-BIOS parsing and dispatch.

// --- HID (controlId < 0x8000, arrives as HID frame from PanelBridge) ---
// PanelGroup sends 16-bit (0–65535); Joystick.use16bit() expects exactly that — no scaling.
OpenSkyhawk::HIDAxis roll    (CTRL_ROLL,     [](uint16_t v){ Joystick.X(v); });
OpenSkyhawk::HIDAxis pitch   (CTRL_PITCH,    [](uint16_t v){ Joystick.Y(v); });
OpenSkyhawk::HIDAxis throttle(CTRL_THROTTLE, [](uint16_t v){ Joystick.sliderLeft(v); });
OpenSkyhawk::HIDAxis rudder  (CTRL_RUDDER,   [](uint16_t v){ Joystick.Zrotate(v); });
OpenSkyhawk::HIDAxis brakeL  (CTRL_BRAKE_L,  [](uint16_t v){ Joystick.sliderRight(v); });
// OpenSkyhawk::HIDAxis   <name>(controlId, [](uint16_t v){ Joystick.<axis>(v); });
// OpenSkyhawk::HIDButton <name>(controlId, buttonNumber);

void setup() {
    // USB identity — set before Joystick.begin()
    USB.setManufacturer("OpenSkyhawk");
    USB.setProduct("A-4E Skyhawk");
    USB.setVIDPID(0x2E8A, 0x4134);

    Joystick.use16bit(true);        // 0–65535 range
    Joystick.useManualSend(true);   // batch setters; call send() once per drain cycle

    SimGateway::setup(Serial1);     // UART1 link to PanelBridge @ 250000 baud
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
    //   3. If any HID setter fired this iteration → Joystick.send() once
}
```

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
  → walk HIDAxis list → CTRL_ROLL match → lambda(value) → Joystick.X(value)
  → after draining UART: Joystick.send()
  → USB HID report → DCS receives roll axis input
```
