

# File PanelBridge.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelBridge**](dir_f592a3c441b32532ba8eb6b28add2a90.md) **>** [**PanelBridge.h**](PanelBridge_8h.md)

[Go to the documentation of this file](PanelBridge_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <stdint.h>

namespace PanelBridge {

    using NodeCallback = void(*)(uint8_t nodeId);

    void onNodeAlive(NodeCallback cb);

    void onNodeDead(NodeCallback cb);

    void setup();

    void loop();

} // namespace PanelBridge

// ── Test hooks (PANELBRIDGE_TEST builds only) ─────────────────────────────────
// Analogous to SimGateway::feedByte(). Allow unit tests to exercise dispatch logic
// without a physical CAN bus or a live DCS-BIOS connection.
#ifdef PANELBRIDGE_TEST
namespace PanelBridge {

    void testDispatchEvt(uint16_t controlId, uint16_t value);

    void testDispatchRel(uint16_t controlId, uint16_t value);

    void testDispatchDir(uint16_t controlId, uint16_t value);

    void testHandleExport(uint16_t address, uint16_t value);

#ifdef PANELBRIDGE_NODE_STATUS
    void testFeedHeartbeat(uint8_t nodeId, uint8_t flags, uint16_t uptime,
                           uint16_t rxCount, uint16_t esr);

    void testFeedHealth(uint8_t nodeId, int8_t dieTempC,
                        uint8_t hFlags = 0, uint8_t faultMask = 0, uint8_t faultId = 0);

    void testRequestNodeStatus();

    void testCheckTimeouts(uint32_t now);
#endif // PANELBRIDGE_NODE_STATUS

} // namespace PanelBridge
#endif // PANELBRIDGE_TEST

#endif // ARDUINO_ARCH_STM32
```


