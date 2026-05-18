#pragma once
#include <stdint.h>

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

// ── Reserved controlId values ─────────────────────────────────────────────────

static constexpr uint16_t CTRL_TEST_SEQ     = 0xFFFF; // triggers TEST_SEQ CAN frame

// ── Diagnostic framing (UART: master → Arduino/tap) ──────────────────────────

static constexpr uint8_t DIAG_MAGIC = 0xAA;
static constexpr uint8_t DIAG_RTT   = 0x01;
static constexpr uint8_t DIAG_HB    = 0x02;
static constexpr uint8_t DIAG_ERR   = 0x03;
