/**
 * @file NodeStatus.h
 * @brief Neutral node-status contract for every OpenSkyhawk node (PanelGroup, PanelBridge, PDU).
 *
 * Owns the cross-node **status API** — not just health:
 *   - the `_NODE_STATUS` DCS-BIOS host-reporting contract (proto version, request address, message
 *     names) — **the canonical source the client's `sync-a4ec.ts` parses**;
 *   - `NodeHealthFlag` (HEALTH_n flag bits) and `NodeFaultCode` (the compact wire faultId dictionary
 *     the client maps to labels);
 *   - `FaultSource` — the interface a fault-producing object implements so a node-level aggregator
 *     can roll it up. A `FaultSource` is *one piece* of node status, not the whole story.
 *
 * This is the neutral home for node-status vocabulary so it isn't shaped around any one node
 * flavour (PanelGroup/OutputBase) or the HID namespace. `NodeHealthPayload` (the CAN HEALTH_n
 * frame struct) stays in `CANProtocol.h`; the fault *detail* strings stay local (DiagSerial),
 * never on the wire. Header-only contract + a tiny `FaultSource` registry (`NodeStatus.cpp`).
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

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

/** @brief HEALTH_n `flags` bits — node-level health conditions (maps to NodeHealthPayload.flags). */
enum class NodeHealthFlag : uint8_t {
    OVERHEAT = 0x01,  // die temp >= NODE_OVERHEAT_C (opt-in, #213)
    DEGRADED = 0x02,  // a FaultSource is faulted → faultCode != 0 (#163)
};

/**
 * @brief HEALTH_n `faultId` dictionary (#163) — cross-node fault codes.
 *
 * Coarse, one active at a time on the wire; the exact device is logged locally on DiagSerial,
 * not the frame. The client maps id → human label (SkyHawkClient#40). Each node type contributes
 * the codes relevant to its fault sources; append as new sources appear.
 */
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

/**
 * @brief A source of node faults — implemented by any object that can fault (#163).
 *
 * Self-registers into a static list at construction; a node-level aggregator walks
 * `FaultSource::head()` to decide node health, pick the primary `faultCode()` for HEALTH_n, and
 * print `faultDetail()` to DiagSerial. Implementers report *cached* state only — cheap, const, no
 * blocking I/O — since the aggregator runs on the periodic health path. Examples: DrumDisplay
 * (I2C), a PDU rail monitor (over/under-voltage/short), a PanelBridge host-link watchdog.
 *
 * @note **Lifetime: a FaultSource must have static/global lifetime** (same rule as OutputBase /
 *       InputBase). Registration is permanent — there is no unregister — so a stack/local
 *       FaultSource would leave a dangling pointer in the registry the aggregator walks. Construct
 *       fault sources as globals (or members of a global object), never on the stack.
 */
class FaultSource {
public:
    /** @brief Current fault code (NodeFaultCode::NONE when healthy). Cheap/const, cached state only. */
    virtual NodeFaultCode faultCode() const { return NodeFaultCode::NONE; }

    /** @brief Human-readable fault detail for the local DiagSerial tap only — never on the wire. */
    virtual const char* faultDetail() const { return ""; }

    /** @brief Head of the self-registered fault-source list. */
    static FaultSource* head();
    /** @brief Next fault source; nullptr at end. */
    FaultSource* next() const;

protected:
    FaultSource();          ///< Registers this instance into the list.
    ~FaultSource() = default;  ///< Protected, non-virtual: a base/mixin, never deleted through this type.

private:
    static FaultSource* _head;
    FaultSource* _next;
};

} // namespace OpenSkyhawk
