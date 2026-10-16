

# File NodeStatus.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**NodeStatus**](dir_9111f7d39b5fc785aa55dffe02a55e74.md) **>** [**NodeStatus.h**](NodeStatus_8h.md)

[Go to the documentation of this file](NodeStatus_8h.md)


```C++

#pragma once
#include <stdint.h>

// ── _NODE_STATUS DCS-BIOS host-reporting contract (#86) ───────────────────────
// PanelBridge reports connected nodes + health to the host over DCS-BIOS. Bump
// NODE_STATUS_PROTO_VERSION on any _NODE_STATUS wire change so the client's sync fails loudly.
// Full 28-hex field decode: FirmwarePlan/04-dcs-bios-integration.md.
#define NODE_STATUS_PROTO_VERSION 3
#define NODE_STATUS_REQ_ADDR      0x86FE     // host→device roster-request export address
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

// ── _NODE_TELEM DCS-BIOS host-reporting contract (#216) ───────────────────────
// PanelBridge forwards every TELEM_n CAN frame to the host unread, as one DCS-BIOS message per
// frame: `_NODE_TELEM <nodeId(2 hex)><8 frame bytes as 16 hex, wire order>` (18 hex chars). Pushed
// as frames arrive — no request, no cache, no end marker. Versioned separately from
// _NODE_STATUS: a new TelemPage is additive and needs no bump; bump on a change to the message
// framing or to an existing page's layout. Full page layouts: FirmwarePlan/02-can-protocol.md.
#define NODE_TELEM_PROTO_VERSION  1
#define NODE_TELEM_MSG_NAME       "_NODE_TELEM"

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
    FUSE_OPEN      = 0x06,  // rail fuse blown — rail read ~0 V while the node is powered (PDU)
    OVER_TEMPERATURE = 0x07,  // board temperature at the critical threshold (PDU NTC)
    SENSOR_FAULT   = 0x08,  // a measurement channel reads implausibly (railed / open / shorted)
    DUPLICATE_NODE_ID = 0x09,  // another board transmits on this node's HB_n — two boards share a NODE_ID (#344)
    // 0x0A–0xFF reserved for future fault sources
};

enum class NodeKind : uint8_t {
    UNKNOWN     = 0x00,  // pre-#344 firmware, or no HEALTH_n seen yet
    PANEL_GROUP = 0x01,  // PanelGroup node (PanelGroup library)
    PDU         = 0x02,  // power distribution / monitoring node (PowerMonitor library)
    BRIDGE      = 0x03,  // the SimGateway + PanelBridge board — reported by its PanelBridge MCU (node 0)
    // 0x04–0xFF reserved for future node types
};

enum class TelemPage : uint8_t {
    INVALID      = 0x00,  // never sent
    PDU_RAIL_12V = 0x01,  // PDU 12 V rail: flags, avg + min voltage, avg current
    PDU_RAIL_5V  = 0x02,  // PDU 5 V rail: flags, avg + min voltage, avg current
    PDU_STATUS   = 0x03,  // PDU board: flags, NTC temperature, per-rail peak current
};

enum class PduRailFlag : uint8_t {
    UNDER_V      = 0x01,  // rail below its under-voltage threshold
    OVER_V       = 0x02,  // rail above its over-voltage threshold
    OVER_CURRENT = 0x04,  // sustained current above the over-current threshold
    FUSE_OPEN    = 0x08,  // rail read ~0 V while the node is powered — fuse blown
    SENSOR_FAULT = 0x10,  // voltage or current channel reads implausibly (railed)
};

enum class PduStatusFlag : uint8_t {
    UNCAL_VDDA   = 0x01,  // no VDDA calibration stored — readings assume a nominal 3.3 V
    UNCAL_ZERO   = 0x02,  // no current zero-offset calibration stored
    TEMP_WARN    = 0x04,  // board temperature at the warning threshold
    TEMP_CRIT    = 0x08,  // board temperature at the critical threshold
    NTC_FAULT    = 0x10,  // NTC channel open or shorted
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

NodeFaultCode aggregateFaults(const char** detailOut = nullptr);

} // namespace OpenSkyhawk
```


