

# File CANProtocol.h

[**File List**](files.md) **>** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md) **>** [**CANProtocol.h**](CANProtocol_8h.md)

[Go to the documentation of this file](CANProtocol_8h.md)


```C++
#pragma once
#include <stdint.h>

// ── CAN bus status ────────────────────────────────────────────────────────────

enum class CanStatus {
    STARTING,   
    NORMAL,     
    TX_ERROR,   
    BUS_OFF,    
};

// ── Packet types ──────────────────────────────────────────────────────────────

// Primary inter-node packet: 4 bytes, sent raw over UART and as CAN payload.
// controlId=0xFFFF is reserved for TEST_SEQ (throughput test).
struct __attribute__((packed)) ControlPacket {
    uint16_t controlId;
    uint16_t value;
};

// ── CAN message IDs ───────────────────────────────────────────────────────────

static constexpr uint32_t CAN_ID_CTRL_BCAST = 0x010; // Master → All: ControlPacket payload
static constexpr uint32_t CAN_ID_TEST_SEQ   = 0x011; // Master → All: throughput test seq

static constexpr uint32_t CAN_ID_HB_1       = 0x100; // Sub-1 → All: heartbeat
static constexpr uint32_t CAN_ID_HB_2       = 0x101; // Sub-2 → All: heartbeat

static constexpr uint32_t CAN_ID_EVT_1      = 0x200; // Sub-1 → Master: input event
static constexpr uint32_t CAN_ID_EVT_2      = 0x201; // Sub-2 → Master: input event

static constexpr uint32_t CAN_ID_ECHO_1     = 0x210; // Sub-1 → Master: seq echo
static constexpr uint32_t CAN_ID_ECHO_2     = 0x211; // Sub-2 → Master: seq echo

// ── controlId namespace ───────────────────────────────────────────────────────
//
// controlId determines how the RP2040 gateway routes an incoming packet:
//
//   0x0010 – 0x00FF  Flight control axes & buttons (HID only — never DCS-BIOS)
//   0x8000 – 0x86FF  DCS-BIOS A-4E-C addresses    (DCS-BIOS only — never HID)
//   0xFFFF           Reserved: TEST_SEQ
//
// Using DCS-BIOS addresses directly as controlIds in the 0x8000+ range means
// no translation table is needed for the downstream direction (DCS → cockpit).
// RP2040 routing logic: controlId < 0x8000 → HID, else → sendDcsBiosMessage().

// ── Flight control axis IDs ───────────────────────────────────────────────────
// Defined in the HIDControls library — include that header for CTRL_ROLL etc.
#include <HIDControls.h>

// ── Reserved controlId values ─────────────────────────────────────────────────

static constexpr uint16_t CTRL_TEST_SEQ     = 0xFFFF; // triggers TEST_SEQ CAN frame

// ── Batched CAN frame (2 × ControlPacket in one 8-byte frame) ────────────────
// MAIN_NODE should batch two ControlPackets per CAN frame when forwarding
// DCS-BIOS state downstream. Halves bus utilisation (~30% at worst case vs ~60%
// unbatched). RP2040 and sub-nodes unpack both slots; slot B controlId 0x0000
// signals an empty/padding slot.

struct __attribute__((packed)) ControlPacketPair {
    ControlPacket a;
    ControlPacket b;  // controlId 0x0000 = unused padding slot
};

// ── Diagnostic framing (UART: master → Arduino/tap) ──────────────────────────

static constexpr uint8_t DIAG_MAGIC = 0xAA;
static constexpr uint8_t DIAG_RTT   = 0x01;
static constexpr uint8_t DIAG_HB    = 0x02;
static constexpr uint8_t DIAG_ERR   = 0x03;
static constexpr uint8_t DIAG_EVT   = 0x04; // Sub-node input event forwarded upstream
```


