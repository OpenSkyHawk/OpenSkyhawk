# PanelGroup

CAN sub-node domain layer for OpenSkyhawk panel boards.

`PanelGroup` turns an STM32F103CBT6 into a CAN sub-node that receives the full DCS-BIOS  
output stream (wrapped as `ControlPacket` frames) from the master node and dispatches it  
to registered output objects. Switches and other inputs are polled, debounced, and sent  
back over CAN as input events. Heartbeats are sent every 500 ms.

The design mirrors DCS-BIOS exactly — output and input objects are declared at global scope  
in the sketch, self-register via static linked lists, and are dispatched by `PanelGroup::loop()`.

## Dependencies

- [STM32Board](../STM32Board/README.md)
- [CANProtocol](../CANProtocol/) — `ControlPacket`, CAN IDs, `DIAG_*` constants

## Node ID strapping

Node ID is read from **PA0** at boot using an internal pull-down:

| PA0 wiring | node_id | CAN IDs used |
|---|---|---|
| Tied to 3.3 V | 1 | HB: 0x100, EVT: 0x200, ECHO: 0x210 |
| Floating (pull-down reads LOW) | 2 | HB: 0x101, EVT: 0x201, ECHO: 0x211 |

## Usage

```cpp
#include <PanelGroup.h>

// Output: drive PB0 from the ARM_MASTER DCS-BIOS bit
OpenSkyhawk::LED armLed(A_4E_C_ARM_MASTER, 0x4000, PB0);

// Output: custom callback for non-standard logic
void onCanopyPos(uint16_t v) { /* drive motor etc. */ }
OpenSkyhawk::IntegerOutput canopy(A_4E_C_CANOPY_POS, onCanopyPos);

// Input: debounced switch → CAN event → DCS-BIOS via SimGateway
OpenSkyhawk::Switch2Pos ejSafe(A_4E_C_SEAT_EJECT_SAFE, PA1);

void setup() {
    STM32Board::setDebug(true);   // optional
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
```

## Output objects

Output objects receive `ControlPacket` frames broadcast by the master node and  
translate them to panel hardware actions.

| Class | DCS-BIOS equivalent | Behaviour |
|---|---|---|
| `OpenSkyhawk::LED` | `DcsBios::LED` | `(value & mask) != 0` → pin HIGH |
| `OpenSkyhawk::IntegerOutput` | `DcsBios::IntegerBuffer` | Calls user callback with raw value |

## Input objects

Input objects poll GPIO, debounce, and call `PanelGroup::sendEvent()` on state change.  
The matching `OpenSkyhawk::DCSInput` on the SimGateway translates the event to a  
DCS-BIOS message — the panel author does not write that translation.

| Class | DCS-BIOS equivalent | Behaviour |
|---|---|---|
| `OpenSkyhawk::Switch2Pos` | `DcsBios::Switch2Pos` | GPIO (INPUT_PULLUP) → CAN event (0/1) |

## controlId routing

`controlId` values in ControlPackets follow the CANProtocol namespace:

| Range | Type | Routing on SimGateway |
|---|---|---|
| `0x0010`–`0x00FF` | HID axis / hat / button / reserved HID expansion | → `Joystick.*()` when a SimGateway handler is registered |
| `0x8000`–`0xFFFF` | DCS-BIOS address | → `sendDcsBiosMessage()` |

For DCS-BIOS outputs (DCS → panel), the controlId **is** the DCS-BIOS address — no  
translation table needed.

## API

See [`PanelGroup.h`](PanelGroup.h) for full Doxygen documentation.

| Function | Description |
|---|---|
| `PanelGroup::setup()` | Init hardware, read PA0 strap, configure CAN filter, start CAN |
| `PanelGroup::loop()` | Update LED, drain CAN FIFO, poll inputs, send heartbeat |
| `PanelGroup::sendEvent(controlId, value)` | Send a CAN input event to the master |
| `PanelGroup::nodeId()` | Return node_id read at boot (1 or 2) |
