

# File NodeStatus.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**NodeStatus**](dir_9111f7d39b5fc785aa55dffe02a55e74.md) **>** [**NodeStatus.h**](NodeStatus_8h.md)

[Go to the documentation of this file](NodeStatus_8h.md)


```C++

#pragma once
#include <stdint.h>

// ── _NODE_STATUS DCS-BIOS host-reporting contract (#86) ───────────────────────
// PanelBridge reports connected nodes + health to the host over DCS-BIOS. Bump
// NODE_STATUS_PROTO_VERSION on any _NODE_STATUS wire change so the client's sync fails loudly.
// Full 26-hex field decode: FirmwarePlan/04-dcs-bios-integration.md.
#define NODE_STATUS_PROTO_VERSION 2
#define NODE_STATUS_REQ_ADDR      0x86FE     // host→device roster-request export address
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

enum class NodeHealthFlag : uint8_t {
    OVERHEAT = 0x01,  // die temp >= NODE_OVERHEAT_C (opt-in, #213)
    DEGRADED = 0x02,  // a FaultSource reports faultCode() != NodeFaultCode::NONE (#163)
};

enum class NodeFaultCode : uint8_t {
    NONE           = 0x00,
    I2C_PERIPHERAL = 0x01,  // an I2C device (OLED/mux/expander) tripped its I2cHealth breaker
    OVER_VOLTAGE   = 0x02,  // rail over-voltage (PDU)
    UNDER_VOLTAGE  = 0x03,  // rail under-voltage (PDU)
    SHORT_CIRCUIT  = 0x04,  // rail short / over-current (PDU)
    HOST_LINK_LOST = 0x05,  // PanelBridge lost the host serial link
    // 0x06–0xFF reserved for future fault sources
};

namespace OpenSkyhawk {

class FaultSource {
public:
    virtual NodeFaultCode faultCode() const { return NodeFaultCode::NONE; }

    virtual const char* faultDetail() const { return ""; }

    static FaultSource* head();
    FaultSource* next() const;

protected:
    FaultSource();          
    ~FaultSource() = default;  

private:
    static FaultSource* _head;
    FaultSource* _next;
};

} // namespace OpenSkyhawk
```


