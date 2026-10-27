

# File CANProtocol.h

[**File List**](files.md) **>** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md) **>** [**CANProtocol.h**](CANProtocol_8h.md)

[Go to the documentation of this file](CANProtocol_8h.md)


```C++

#pragma once
#include <stdint.h>
#include <HIDControls.h>
#include <NodeStatus.h>   // NodeFaultCode / NodeHealthFlag — packed into HEALTH_n by makeNodeHealthPayload

// ── Types ─────────────────────────────────────────────────────────────────────

struct __attribute__((packed)) ControlPacket {
    uint16_t controlId;  
    uint16_t value;      
};

struct __attribute__((packed)) ControlPacketPair {
    ControlPacket a;
    ControlPacket b;  
};

struct __attribute__((packed)) HeartbeatPayload {
    uint8_t  nodeId;   
    uint8_t  flags;    
    uint16_t uptime;   
    uint16_t rxCount;  
    uint16_t esr;      
};

struct __attribute__((packed)) NodeHealthPayload {
    uint8_t  nodeId;     
    int8_t   dieTempC;   
    uint8_t  flags;      
    uint8_t  faultMask;  
    uint8_t  faultId;    
    uint8_t  nodeKind;   
    uint8_t  rsvd[2];    
};
static_assert(sizeof(NodeHealthPayload) == 8, "NodeHealthPayload must fill one classic CAN frame");

struct __attribute__((packed)) TelemPayload {
    uint8_t page;     
    uint8_t body[7];  
};
static_assert(sizeof(TelemPayload) == 8, "TelemPayload must fill one classic CAN frame");

struct __attribute__((packed)) ReadyPayload {
    uint8_t fwMajor;     
    uint8_t fwMinor;     
    uint8_t fwPatch;     
    uint8_t bootloader;  
    uint8_t flags;       
    uint8_t rsvd[3];     
};

enum class ReadyFlag : uint8_t {
    RESYNC = 0x01,  // an answer to SYNC_REQ, not a boot: the bridge caches the version but must not
                    // broadcast SYNC_REQ for it (that would answer itself forever)
};
static_assert(sizeof(ReadyPayload) == 8, "ReadyPayload must fill one classic CAN frame");

enum class CanStatus {
    STARTING,   
    NORMAL,     
    TX_ERROR,   
    BUS_OFF,    
};

// ── Fixed CAN arbitration IDs (PanelBridge -> All) ───────────────────────────

static constexpr uint32_t CAN_ID_CTRL_BCAST = 0x7E0;
static constexpr uint32_t CAN_ID_TEST_SEQ   = 0x011;  
static constexpr uint32_t CAN_ID_SYNC_REQ   = 0x012;  

static constexpr uint8_t SYNC_REQ_LEN_TARGETED = 1;

// ── CAN ID functions (per-node IDs computed from NODE_ID) ─────────────────────

constexpr uint32_t canIdHb(uint8_t n)    { return 0x100 + n; }

constexpr uint32_t canIdHealth(uint8_t n) { return 0x140 + n; }

constexpr uint32_t canIdTelem(uint8_t n)  { return 0x180 + n; }

constexpr uint32_t canIdEvt(uint8_t n)   { return 0x200 + n; }

constexpr uint32_t canIdEvtRel(uint8_t n) { return 0x500 + n; }

constexpr uint32_t canIdEvtDir(uint8_t n) { return 0x600 + n; }

constexpr uint32_t canIdEvtAction(uint8_t n) { return 0x700 + n; }

constexpr uint32_t canIdEcho(uint8_t n)  { return 0x300 + n; }

constexpr uint32_t canIdReady(uint8_t n) { return 0x400 + n; }

// 0x740-0x7BF and 0x7C0-0x7C1: the bootloader's frames (#363) — BOOT_CMD_n = 0x740+n (bridge → node
// n), BOOT_RSP_n = 0x780+n (node n → bridge), and the unconfigured-board pair. Below every control and
// status frame, so update traffic never delays a control; only CTRL_BCAST (0x7E0) is lower (#389).
// Defined in BootProtocol.h, not here.

// ── controlId namespace ───────────────────────────────────────────────────────
// Payload controlIds live inside ControlPacket data bytes. They are intentionally
// separate from the 11-bit CAN arbitration IDs above.
// CTRL_ID_HID_MIN / CTRL_ID_HID_MAX are defined as macros in HIDControls.h (included above).
// Do not redeclare them here — macro expansion would cause a syntax error.

static constexpr uint16_t CTRL_ID_DCS_MIN  = 0x8000;  
static constexpr uint16_t CTRL_ID_DCS_MAX  = 0x86FF;  

// ── UART diagnostic framing constants (PanelBridge -> SimGateway) ─────────────
// Used by SimGateway to parse the UART diagnostic stream from PanelBridge.

static constexpr uint8_t DIAG_MAGIC = 0xAA;  
static constexpr uint8_t DIAG_RTT   = 0x01;  
static constexpr uint8_t DIAG_HB    = 0x02;  
static constexpr uint8_t DIAG_ERR   = 0x03;  
static constexpr uint8_t DIAG_EVT   = 0x04;  

// ── Callback types ────────────────────────────────────────────────────────────

using CanStatusCallback  = void(*)(CanStatus status);

using CanRxCallback      = void(*)(uint32_t canId, const uint8_t* data, uint8_t len);

using CanSyncReqCallback = void(*)();

// ── Runtime API (STM32 only) ──────────────────────────────────────────────────
//
// Library-internal: the node libraries (PanelGroup, PanelBridge, PduNode) drive the bus through
// these, not sketches. Not covered by the compatibility policy (#295) — except sendBatched(), which
// a custom control class uses to send its value (with ControlPacket and the canIdEvt* functions
// above, which are covered with the wire format).

#ifdef ARDUINO_ARCH_STM32

namespace CANProtocol {

    // ── Filter configuration (call before start()) ────────────────────────────

    void filterAcceptAll();

    void filterAcceptId(uint32_t canId);

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    void start();

    void startLoopback();

    void drain();

    // ── Transmit ──────────────────────────────────────────────────────────────

    void send(uint32_t canId, const uint8_t* data, uint8_t len);

    void sendBatched(uint32_t canId, const ControlPacket& pkt);

    void flushBatched(uint32_t canId);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void onStatusChange(CanStatusCallback cb);

    void onSyncReq(CanSyncReqCallback cb);

    void onReceive(CanRxCallback cb);

    // ── Diagnostics ───────────────────────────────────────────────────────────

    uint8_t tec();

    uint8_t rec();

    bool busOff();

    HeartbeatPayload makeHeartbeatPayload(uint8_t nodeId, uint16_t rxCount);

    NodeHealthPayload makeNodeHealthPayload(uint8_t nodeId, NodeKind kind, int8_t dieTempC,
                                            NodeFaultCode fault = NodeFaultCode::NONE);

    ReadyPayload makeReadyPayload(uint8_t flags = 0);

    void sendReady();

    uint32_t txDropCount();

    uint32_t rxDropCount();

    uint32_t rxOverrunCount();

    void sendSyncReq(uint8_t nodeId);

    // ── Node status service (every node) ──────────────────────────────────────

    void serviceNodeStatus(NodeKind kind, uint16_t rxCount = 0);

    const NodeHealthPayload& lastNodeHealth();

    static constexpr uint32_t MASTER_TIMEOUT_MS = 1500;

    static constexpr uint32_t DUP_NODE_TIMEOUT_MS = 3000;

} // namespace CANProtocol

#endif // ARDUINO_ARCH_STM32
```


