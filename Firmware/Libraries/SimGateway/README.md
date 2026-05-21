# SimGateway

RP2040 USB HID + DCS-BIOS gateway domain layer for OpenSkyhawk.

`SimGateway` is the infrastructure layer for the RP2040 gateway board. It owns:

- **USB identity** — sets VID/PID and product strings before the composite device stack starts.
- **Joystick HID** — initialises full 16-bit axis range and manual-send mode.
- **UART link** — starts the serial port to PanelBridge at 250000 baud, drains incoming  
  bytes, reassembles 8-byte DIAG frames, and dispatches them to registered callbacks.

DCS-BIOS `setup()`/`loop()` and all application code (IntegerBuffer callbacks, NeoPixel,  
custom HID button wiring) stay in the sketch — SimGateway is infrastructure only.

## Dependencies

- [CANProtocol](../CANProtocol/) — `ControlPacket`, `CTRL_TEST_SEQ`, `DIAG_*` constants
- `earlephilhower/arduino-pico` platform
- `dcs-skunkworks/DCS-BIOS` library (managed by the sketch, not this library)

## Usage

`#define DCSBIOS_DEFAULT_SERIAL` must appear before `#include <DcsBios.h>` in the  
sketch's translation unit. Include DcsBios.h first, then SimGateway.h:

```cpp
#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>        // DCSBIOS_DEFAULT_SERIAL must be set before this
#include <SimGateway.h>     // pulls in Joystick.h transitively

// DCS-BIOS output callbacks — application code, stays in the sketch
void onRpmChange(unsigned int v) { /* ... */ }
DcsBios::IntegerBuffer rpmBuf(A_4E_C_RPM, onRpmChange);

void onRtt(uint16_t seq, uint32_t sentMs) { /* compute RTT */ }

void setup() {
    SimGateway::onDiagRtt(onRtt);   // register callbacks before setup()
    SimGateway::setup(Serial1);     // GP0 TX / GP1 RX @ 250000
    DcsBios::setup();               // start CDC serial after USB identity is set
}

void loop() {
    DcsBios::loop();
    SimGateway::loop();
}
```

## USB identity

| Field | Value |
|---|---|
| Manufacturer | OpenSkyhawk |
| Product | A-4E Skyhawk |
| VID | 0x2E8A (Raspberry Pi) |
| PID | 0x4134 |

See `architecture.md` for the full PID allocation table.

## Sending to PanelBridge

Use `SimGateway::send()` to forward a ControlPacket to PanelBridge over UART:

```cpp
// Trigger the built-in ping-pong RTT test
SimGateway::send(CTRL_TEST_SEQ, pingSeq);

// Forward a DCS-BIOS output state change to a panel board
SimGateway::send(A_4E_C_ARM_MASTER, newValue);
```

## DIAG frame callbacks

PanelBridge sends 8-byte DIAG frames to the RP2040 in response to sub-node activity.  
Register callbacks before `setup()` to receive them:

| Callback | Trigger | Parameters |
|---|---|---|
| `onDiagRtt` | Sub-node echoed a TEST_SEQ frame | seq, sentMs |
| `onDiagHb` | Sub-node sent a heartbeat | nodeId, rxCount |
| `onDiagErr` | CAN error counters non-zero on master | tec, rec, flags |

## API

See [`SimGateway.h`](SimGateway.h) for full Doxygen documentation.

| Function | Description |
|---|---|
| `SimGateway::setup(uartPort)` | Init USB identity, Joystick, UART |
| `SimGateway::loop()` | Drain UART, dispatch DIAG frames |
| `SimGateway::send(controlId, value)` | Send a ControlPacket to PanelBridge |
| `SimGateway::onDiagRtt(cb)` | Register RTT frame callback |
| `SimGateway::onDiagHb(cb)` | Register heartbeat frame callback |
| `SimGateway::onDiagErr(cb)` | Register error frame callback |
