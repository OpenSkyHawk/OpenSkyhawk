/**
 * @file I2cHealth.h
 * @brief Circuit-breaker mixin ("trait") for I2C-backed device classes.
 *
 * @details A blocking I2C transaction to an absent or dead device stalls `PanelGroup::loop()` and
 * starves the node heartbeat — the bridge then flaps the node online/offline (#164). Any OpenSkyhawk
 * class that drives an I2C device **mixes this in** and implements `i2cProbe()` (its own reachability
 * check); it then gates every I2C op behind `i2cReachable()`. A dead device drops from "block every
 * loop" to "one probe every `I2C_RETRY_MS`", and auto-recovers when it returns. The class's
 * data/decode path must stay I2C-free, so values stay current and the next reachable frame catches up
 * to the live value instead of showing a stale one.
 *
 * This is the C++ analog of a trait: shared behaviour lives here, the contract (`i2cProbe()`) is
 * pure-virtual so a mixing class cannot compile without honouring it.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>

namespace OpenSkyhawk {

/**
 * @brief Per-device I2C circuit breaker. Mix into any class that talks to an I2C device.
 *
 * @details Usage: `class Foo : public OutputBase, public I2cHealth { bool i2cProbe() override {...} };`
 * then `if (!i2cReachable()) return;` before any I2C transaction.
 */
class I2cHealth {
public:
    /** @brief Breaker state — true while the device last probed reachable. */
    bool i2cHealthy() const { return _i2cHealthy; }

#ifdef DRUMDISPLAY_TEST
    /** @brief Test seam — expire the back-off so the next i2cReachable() re-probes immediately. */
    void debugExpireBackoff() { _i2cLastAttempt = millis() - I2C_RETRY_MS - 1; }
#endif

protected:
    /**
     * @brief Contract: probe this device's reachability (e.g. the mux ACKs *and* the device ACKs).
     * @return true if reachable. The implementer records any fault detail it wants to report.
     * @note Must be cheap — a single address probe, no payload — and must not throw or block beyond
     *       one bounded I2C transaction.
     */
    virtual bool i2cProbe() = 0;

    /**
     * @brief Gate for every I2C op. Rate-limits the probe while tripped; trips/heals on the result.
     * @return true → safe to talk to the device; false → skip the op (dead/absent, backing off).
     */
    bool i2cReachable() {
        const uint32_t now = millis();
        if (!_i2cHealthy && (now - _i2cLastAttempt) < I2C_RETRY_MS) return false;  // tripped → back off
        _i2cLastAttempt = now;
        return (_i2cHealthy = i2cProbe());
    }

    /** @brief Back-off between retries once tripped (ms). A couple of seconds keeps the bus quiet. */
    static constexpr uint32_t I2C_RETRY_MS = 2000;

    ~I2cHealth() = default;  // protected, non-virtual: a mixin, never deleted through this type

private:
    bool     _i2cHealthy     = true;
    uint32_t _i2cLastAttempt = 0;
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
