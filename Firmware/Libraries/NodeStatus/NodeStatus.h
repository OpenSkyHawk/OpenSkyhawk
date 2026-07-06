/**
 * @file NodeStatus.h
 * @brief PanelBridge node-status host contract + node fault dictionary.
 *
 * The reserved DCS-BIOS identifiers PanelBridge uses to report connected PanelGroup
 * nodes + their health to the host (OpenSkyhawk Client), plus the NodeFaultId dictionary
 * for the HEALTH_n `faultId` field. Split out of HIDControls.h (which is HID controlId
 * constants only): this is the node-status/health contract, a separate concern.
 *
 * Header-only, platform-agnostic (STM32 via CANProtocol/PanelBridge, RP2040 via SimGateway
 * relay). **This is the canonical contract source the client's `sync-a4ec.ts` parses** — bump
 * NODE_STATUS_PROTO_VERSION on any `_NODE_STATUS` wire change so the client's sync assertion
 * fails loudly.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>

// ── Reserved DCS-BIOS identifiers (node-status reporting, #86) ────────────────
//
// Node presence/health is surfaced to the host over the existing DCS-BIOS protocol,
// not a bespoke sideband. Owned by PanelBridge (SimGateway relays everything transparently).
//
//   NODE_STATUS_REQ_ADDR     host→device DCS-BIOS export address the client writes
//                         to request the roster. Above every real A-4E-C output
//                         (~0x8554), so DCS never exports it — no false trigger.
//   NODE_STATUS_MSG_NAME     device→host per-node status command name. Leading
//                         underscore — no A-4E-C control collides.
//   NODE_STATUS_END_MSG_NAME device→host burst terminator (request/boot replies).
//                         Argument = node count in the burst. Lets the client
//                         know a roster reply is complete and reconcile/prune.
//
// _NODE_STATUS argument: 26 chars, each field its numeric value as fixed-width
// uppercase hex (most-significant nibble first):
//   nodeId(2) present(2) flags(2) uptime(4) rxCount(4) esr(4)
//     dieTempC(2) hFlags(2) faultMask(2) faultId(2)
//   present: 01 alive, 00 removed.  flags: bit0 BOFF, bit1 EPVF.
//   esr: low byte TEC, high byte REC.  nodeId range 1–63.
//   uptime/rxCount are uint16 — wrap at 65535 (~18 h / 65 k frames). Treat as
//   health indicators, not monotonic counters.
//   The last four fields are the node's cached HEALTH_n telemetry (proto v2):
//     dieTempC: int8 two's-complement whole °C (internal MCU sensor — UNCALIBRATED,
//       die not ambient). 80 = INT8_MIN = not-yet-seen → render unknown.
//     hFlags:  node health bits — bit0 overheat, bit1 degraded.
//     faultMask/faultId: tripped-peripheral bitmap + detail id (0 until #163 populates;
//       faultId is an index into a client-side lookup table — no strings on the wire).
//
// Emission: a single bare _NODE_STATUS is a live delta (apply immediately). A
// request/boot reply is N _NODE_STATUS messages followed by _NODE_STATUS_END <count>;
// that set is the authoritative present-roster — prune nodes absent from it.
// Silent death (yank / bus-off) is reported as present=00 by PanelBridge's 3 s
// heartbeat timeout; a periodic client request reconciles any lost delta.
#define NODE_STATUS_PROTO_VERSION 2
#define NODE_STATUS_REQ_ADDR      0x86FE
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

// ── Node fault codes — HEALTH_n faultId values (#163) ─────────────────────────
// Coarse, one active at a time: the CAN wire carries just this generic id; the exact
// failing device is logged on the node's DiagSerial tap, not the frame. The client maps
// id → human label (SkyHawkClient#40). Grow as new fault sources appear.
enum class NodeFaultId : uint8_t {
    NONE           = 0x00,
    I2C_PERIPHERAL = 0x01,  // an I2C device (OLED / mux / expander) tripped its I2cHealth breaker
    // 0x02–0xFF reserved for future fault sources (stepper, ADC, …)
};
