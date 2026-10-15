/**
 * @file BoardThermal.h
 * @brief PDU monitoring node — board temperature from the NTC: warn / critical / sensor fault (#216).
 *
 * Fed a converted reading (0.1 °C, from PduMath::ntcTempDC) roughly every 100 ms. Thresholds use
 * hysteresis only — board temperature moves slowly, so no persistence timers. A FaultSource:
 * critical → OVER_TEMPERATURE, an open/shorted NTC → SENSOR_FAULT. TEMP_WARN is a telemetry flag
 * only, not a node fault.
 *
 * This is the PDU's real board temperature (±0.5 °C class NTC between the 12 V and 5 V pours). The
 * MCU die temperature in HEALTH_n is a separate, uncalibrated reading.
 *
 * @note Static lifetime only (FaultSource registry rule) — construct as a global.
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>
#include <NodeStatus.h>

namespace OpenSkyhawk {

class BoardThermal : public FaultSource {
public:
    BoardThermal() = default;

    /**
     * @brief Feed one reading.
     * @param tempDC   Board temperature, 0.1 °C (ignored when `ntcFault`).
     * @param ntcFault NTC channel open or shorted.
     * @param armed    false during power-up blanking — the flags are held clear.
     */
    void sample(int16_t tempDC, bool ntcFault, bool armed);

    /** @brief PduStatusFlag bits this monitor owns: TEMP_WARN, TEMP_CRIT, NTC_FAULT. */
    uint8_t flags() const;

    /** @brief Latest temperature, 0.1 °C; INT16_MIN when unavailable (no reading yet / NTC fault). */
    int16_t tempDC() const { return _tempDC; }

    NodeFaultCode faultCode() const override;
    const char*   faultDetail() const override;

private:
    int16_t _tempDC   = INT16_MIN;
    bool    _warn     = false;
    bool    _crit     = false;
    bool    _ntcFault = false;
};

} // namespace OpenSkyhawk
