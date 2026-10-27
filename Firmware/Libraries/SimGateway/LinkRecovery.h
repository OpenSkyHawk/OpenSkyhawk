/**
 * @file LinkRecovery.h
 * @brief USB self-recovery ladder for SimGateway — pure decision logic, no hardware.
 *
 * The SimGateway is powered only from its USB-C, so while it runs it is plugged into
 * something. Running but not configured by a host is therefore abnormal, and the failure
 * this exists for (#355) looks exactly like that: the device dropped off the bus while the
 * core kept running, and only a power cycle brought it back. The ladder escalates:
 *
 *   1. unhealthy for LINK_BAD_MS          → detach, then attach LINK_REATTACH_GAP_MS later
 *                                           (toggles the D+ pull-up: the host sees a fresh plug-in)
 *   2. still unhealthy LINK_REBOOT_AFTER_MS → reboot (fully resets the USB controller)
 *   3. after a recovery reboot             → re-attach every LINK_BACKOFF_MS, never reboot again
 *                                           until the link has been healthy once
 *
 * Step 3 keeps a SimGateway on a charger, or back-fed with no host, from reboot-looping.
 *
 * Pure: takes `now` and the sampled health, returns what to do. SimGateway.cpp applies the
 * action, so SIMGATEWAY_TEST builds drive the whole ladder with injected inputs.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>

namespace OpenSkyhawk {

constexpr uint32_t LINK_BAD_MS          = 5000;   ///< unhealthy this long → first re-attach
constexpr uint32_t LINK_REATTACH_GAP_MS = 100;    ///< detached this long before attach
constexpr uint32_t LINK_REBOOT_AFTER_MS = 5000;   ///< re-attached, still unhealthy → reboot
constexpr uint32_t LINK_BACKOFF_MS      = 60000;  ///< after a recovery reboot: re-attach period

/** @brief What the caller should do this tick. */
enum class LinkAction : uint8_t {
    NONE,    ///< Nothing.
    DETACH,  ///< TinyUSBDevice.detach().
    ATTACH,  ///< TinyUSBDevice.attach().
    REBOOT,  ///< Record the reason, then reboot the chip.
};

/** @brief Ladder state. Zero-initialised = healthy, no backoff. */
struct LinkRecovery {
    uint8_t  phase;      // internal: LinkPhase
    bool     backoff;    // a recovery reboot already happened and has not been followed by health
    uint32_t sinceMs;    // start of the current phase
};

/**
 * @brief Reset the ladder.
 * @param s                   Ladder state.
 * @param afterRecoveryReboot True when this boot was caused by the ladder's own REBOOT.
 *                            Puts the ladder straight into backoff, so it never reboots twice
 *                            in a row.
 */
void linkRecoveryBegin(LinkRecovery& s, bool afterRecoveryReboot);

/**
 * @brief Advance the ladder one tick.
 * @param s        Ladder state.
 * @param now      millis().
 * @param healthy  The host link is usable: mounted, not suspended with a recent UART fault, and
 *                 the CDC IN endpoint not stalled.
 * @return The action to apply now. At most one per tick.
 *
 * @note A DETACH is always followed by its ATTACH, whatever `healthy` says in between — a
 *       detached device is unmounted by definition, and a half-run ladder must never leave the
 *       pull-up off.
 */
LinkAction linkRecoveryStep(LinkRecovery& s, uint32_t now, bool healthy);

} // namespace OpenSkyhawk
