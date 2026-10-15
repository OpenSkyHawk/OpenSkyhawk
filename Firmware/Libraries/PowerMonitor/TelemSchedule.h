/**
 * @file TelemSchedule.h
 * @brief When to send which TELEM_n page — periodic rotation plus rate-limited edge sends (#216).
 *
 * - Periodic: one frame per TELEM_PERIOD_MS (the HEALTH_n rate), rotating through the pages, so
 *   each page refreshes every PAGES × period. A periodic send is the one that restarts that page's
 *   statistics window.
 * - Edge: when a page's flag byte differs from what was last sent for it, send that page now —
 *   at most once per EDGE_MIN_MS per page; a change inside the hold-off goes out when it ends.
 *   An edge send does not restart the window. Every page starts from an all-clear baseline (0),
 *   so a condition present at boot goes out promptly instead of waiting for its periodic slot.
 *
 * Hardware-free — the caller supplies the clock and the current flag bytes.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>

namespace OpenSkyhawk {
namespace Pdu {

class TelemSchedule {
public:
    static constexpr uint8_t  PAGES           = 3;     ///< PDU_RAIL_12V, PDU_RAIL_5V, PDU_STATUS
    static constexpr uint32_t TELEM_PERIOD_MS = 1000;  ///< one frame per second per node
    static constexpr uint32_t EDGE_MIN_MS     = 100;   ///< per-page edge-send hold-off

    /** @brief Start the clock; the first periodic frame is due one period later. */
    void start(uint32_t nowMs);

    /**
     * @brief Which page to send now, if any.
     * @param nowMs     millis().
     * @param flags     Current flag byte of each page (index = page slot).
     * @param periodic  Out: true for the periodic (window-restarting) send.
     * @return Page slot 0..PAGES-1, or -1 when nothing is due.
     */
    int8_t next(uint32_t nowMs, const uint8_t flags[PAGES], bool& periodic);

    /** @brief Record that page `slot` went out carrying `flags` at `nowMs`. */
    void sent(uint8_t slot, uint8_t flags, uint32_t nowMs);

private:
    uint32_t _lastPeriodic = 0;
    uint8_t  _rr = 0;
    uint32_t _lastSent[PAGES]  = {};
    uint8_t  _lastFlags[PAGES] = {};
};

} // namespace Pdu
} // namespace OpenSkyhawk
