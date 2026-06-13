# Example — PanelBridge Sketch

PanelBridge runs the DCS-BIOS library on UART2 (`Serial`, PA2/PA3 → SimGateway). SimGateway
forwards the raw DCS-BIOS byte stream from USB CDC onto this UART transparently, so the library
operates exactly as if it were connected directly to the PC.

An `ExportStreamListener` built into the PanelBridge library broadcasts all DCS output values
over CAN automatically. CAN EVTs from PanelGroup nodes are routed via the generated input map —
**no per-control declarations are needed in the sketch.**

```cpp
#define DCSBIOS_DEFAULT_SERIAL   // DCS-BIOS reads/writes Serial (UART2 PA2/PA3 → SimGateway)
#include <DcsBios.h>
#include <PanelBridge.h>

// Optional callbacks — log node liveness. Both are optional; remove if not needed.
void onNodeAlive(uint8_t nodeId) {
    // fires when a node transitions dead/unseen → alive
    // use for: status LED, diagnostic log, etc.
}
void onNodeDead(uint8_t nodeId) {
    // fires if no HB seen from a known node for 3 seconds
    // use for: alert, fault logging, etc.
}

void setup() {
    PanelBridge::onNodeAlive(onNodeAlive);
    PanelBridge::onNodeDead(onNodeDead);
    PanelBridge::setup();
    // PanelBridge::setup() does:
    //   - CAN init with pass-all filter + software range validation
    //   - Registers ExportStreamListener (auto-broadcasts DCS outputs over CAN)
    //   - Broadcasts cold-boot SYNC_REQ (0x012) to re-poll any running PanelGroup nodes
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
    // Receives DCS-BIOS binary stream from SimGateway via Serial (UART2).
    // Fires ExportStreamListener for each address update.
    // Detects model_time backward → broadcasts SYNC_REQ.

    PanelBridge::loop();
    // Services CANProtocol batching deadlines.
    // Drains CAN RX: for each EVT frame:
    //   0x8000 <= controlId <= 0x86FF → binary search in A4EC_InputMap → sendDcsBiosMessage()
    //   controlId < 0x8000  → wrap in HID frame (0xAA 0x55 + controlId + value) → UART
    // Handles READY / node recovery → broadcasts SYNC_REQ.
    // Checks heartbeat timeouts → fires onNodeDead if needed.
}
```

## What PanelBridge does NOT need

- No per-panel control declarations — the generated `A4EC_InputMap.h` covers the full A-4E-C
  control set automatically.
- No per-axis declarations — HID routing is triggered purely by `controlId < 0x8000`.
- No `SimGateway.h` include — PanelBridge communicates with SimGateway over `Serial` (UART2)
  using raw bytes and HID frames, not via the SimGateway library API.
- No `STM32Board.h` include unless the sketch directly calls board helpers such as
  `STM32Board::setDebug(true)`. `PanelBridge::setup()` handles board init internally.

`A4EC_InputMap.h` is included by the PanelBridge library implementation, not by the sketch.
