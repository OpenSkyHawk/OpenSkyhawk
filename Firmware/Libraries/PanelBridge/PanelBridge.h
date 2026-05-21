#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <CANProtocol.h>

// ── PanelBridge namespace ─────────────────────────────────────────────────────
// Static singleton for the CAN master node. Transparent bridge:
//   ControlPacket from UART → broadcast on CAN bus
//   CAN frames from sub-nodes → UART to SimGateway (as DIAG frames)
// Accepts all CAN frames. Monitors sub-node heartbeats.

namespace PanelBridge {
    // uartPort: hardware serial connected to RP2040 SimGateway (UART2 on PA2/PA3)
    void setup(HardwareSerial& uartPort);
    void loop();

    // Optional liveness callbacks — set before setup().
    // onNodeAlive fires when the first heartbeat arrives after a dead period.
    // onNodeDead fires when no heartbeat is seen for 3 seconds.
    void onNodeAlive(void (*cb)(uint8_t nodeId));
    void onNodeDead(void (*cb)(uint8_t nodeId));
}

#endif // ARDUINO_ARCH_STM32
