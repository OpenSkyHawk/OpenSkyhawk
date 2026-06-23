

# File CANProtocol.h

[**File List**](files.md) **>** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md) **>** [**CANProtocol.h**](CANProtocol_8h.md)

[Go to the documentation of this file](CANProtocol_8h.md)


```C++

#pragma once
#include <stdint.h>
#include <HIDControls.h>

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

enum class CanStatus {
    STARTING,   
    NORMAL,     
    TX_ERROR,   
    BUS_OFF,    
};

// ── Fixed CAN arbitration IDs (PanelBridge -> All) ───────────────────────────

static constexpr uint32_t CAN_ID_CTRL_BCAST = 0x010;  
static constexpr uint32_t CAN_ID_TEST_SEQ   = 0x011;  
static constexpr uint32_t CAN_ID_SYNC_REQ   = 0x012;  

// ── CAN ID functions (per-node IDs computed from NODE_ID) ─────────────────────

constexpr uint32_t canIdHb(uint8_t n)    { return 0x100 + n; }

constexpr uint32_t canIdEvt(uint8_t n)   { return 0x200 + n; }

constexpr uint32_t canIdEvtRel(uint8_t n) { return 0x500 + n; }

constexpr uint32_t canIdEvtDir(uint8_t n) { return 0x600 + n; }

constexpr uint32_t canIdEcho(uint8_t n)  { return 0x300 + n; }

constexpr uint32_t canIdReady(uint8_t n) { return 0x400 + n; }

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

    uint32_t txDropCount();

} // namespace CANProtocol

#endif // ARDUINO_ARCH_STM32
```


