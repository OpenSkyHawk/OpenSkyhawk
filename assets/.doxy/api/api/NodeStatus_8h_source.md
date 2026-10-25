

# File NodeStatus.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**NodeStatus**](dir_9111f7d39b5fc785aa55dffe02a55e74.md) **>** [**NodeStatus.h**](NodeStatus_8h.md)

[Go to the documentation of this file](NodeStatus_8h.md)


```C++

#pragma once
#include <stdint.h>

// ── _NODE_STATUS DCS-BIOS host-reporting contract (#86) ───────────────────────
// PanelBridge reports connected nodes + health to the host over DCS-BIOS. Bump
// NODE_STATUS_PROTO_VERSION on any _NODE_STATUS wire change so the client's sync fails loudly.
// Full 34-hex field decode: FirmwarePlan/04-dcs-bios-integration.md.
#define NODE_STATUS_PROTO_VERSION 4
// Host→device roster-request export address (#295, v4). No DCS-BIOS module exports here: every
// aircraft ends by 0xB2FB and MetadataEnd is 0xFFFE (checked against DCS-BIOS v0.11.8). The old
// 0x86FE is a live address for the AH-64D and the Mirage F1, so flying either answered every
// export of that control with a roster burst. The A4EC refresh warns if a new DCS-BIOS release ever
// puts an aircraft here (tools/gen_a4ec, --modules-dir).
#define NODE_STATUS_REQ_ADDR      0xFF00
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

constexpr uint8_t GATEWAY_NODE_ID = 0;

constexpr uint8_t NODE_STATUS_ROW_BYTES = 17;

enum class NodeStatusField : uint8_t {
    ID           = 0,   // u8: node ID; GATEWAY_NODE_ID = the bridge's own row, 1-63 = CAN nodes
    PRESENT      = 1,   // u8: 01 alive, 00 removed
    HB_FLAGS     = 2,   // u8: HeartbeatFlag bits
    UPTIME       = 3,   // u16: seconds since boot, wraps at 65535
    RX_COUNT     = 5,   // u16: node-owned accepted RX count, wraps at 65535
    ESR          = 7,   // u16: TEC at HB_ESR_TEC_SHIFT, REC at HB_ESR_REC_SHIFT
    DIE_TEMP_C   = 9,   // i8: whole degC, DIE_TEMP_UNAVAILABLE = not seen
    HEALTH_FLAGS = 10,  // u8: NodeHealthFlag bits
    FAULT_MASK   = 11,  // u8: reserved, 00
    FAULT_ID     = 12,  // u8: NodeFaultCode
    NODE_KIND    = 13,  // u8: NodeKind
    FW_MAJOR     = 14,  // u8: firmware release version from the node's READY_n; 00 00 00 = not reported
    FW_MINOR     = 15,  // u8
    FW_PATCH     = 16,  // u8
};

// ── _NODE_TELEM DCS-BIOS host-reporting contract (#216) ───────────────────────
// PanelBridge forwards every TELEM_n CAN frame to the host unread, as one DCS-BIOS message per
// frame: `_NODE_TELEM <nodeId(2 hex)><8 frame bytes as 16 hex, wire order>` (18 hex chars). Pushed
// as frames arrive — no request, no cache, no end marker. Versioned separately from
// _NODE_STATUS: a new TelemPage is additive and needs no bump; bump on a change to the message
// framing or to an existing page's layout. Full page layouts: FirmwarePlan/02-can-protocol.md.
#define NODE_TELEM_PROTO_VERSION  1
#define NODE_TELEM_MSG_NAME       "_NODE_TELEM"

constexpr uint8_t NODE_TELEM_ROW_BYTES = 9;

enum class NodeTelemField : uint8_t {
    ID      = 0,  // u8: sending node ID (from the CAN ID)
    FRAME   = 1,  // 8 bytes: the TELEM_n frame unread, wire order (TelemPage at FRAME + 0)
};

enum class HeartbeatFlag : uint8_t {
    BOFF = 0x01,  // CAN controller is bus-off
    EPVF = 0x02,  // error passive: TEC or REC >= 128
};

constexpr uint8_t HB_ESR_TEC_SHIFT = 0;
constexpr uint8_t HB_ESR_REC_SHIFT = 8;

enum class NodeHealthFlag : uint8_t {
    OVERHEAT = 0x01,  // die temp >= NODE_OVERHEAT_C (opt-in, #213)
    DEGRADED = 0x02,  // a FaultSource reports faultCode() != NodeFaultCode::NONE (#163)
};

constexpr int8_t DIE_TEMP_UNAVAILABLE = -128;
static_assert(DIE_TEMP_UNAVAILABLE == INT8_MIN, "DIE_TEMP_UNAVAILABLE is the int8 minimum");

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

enum class PduRailField : uint8_t {
    FLAGS    = 1,  // u8: PduRailFlag bits
    V_AVG_MV = 2,  // u16: window-average rail voltage, mV
    V_MIN_MV = 4,  // u16: window-minimum rail voltage, mV
    I_AVG_MA = 6,  // int16: window-average rail current, mA (signed: zero offset stays visible)
};

enum class PduStatusField : uint8_t {
    FLAGS       = 1,  // u8: PduStatusFlag bits
    NTC_TEMP_DC = 2,  // int16: board temperature, 0.1 degC (NTC_TEMP_UNAVAILABLE = no reading)
    I12_PEAK_MA = 4,  // u16: 12 V rail peak current since the last page, mA
    I5_PEAK_MA  = 6,  // u16: 5 V rail peak current since the last page, mA
};

constexpr int16_t NTC_TEMP_UNAVAILABLE = -32768;
static_assert(NTC_TEMP_UNAVAILABLE == INT16_MIN, "NTC_TEMP_UNAVAILABLE is the int16 minimum");

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


