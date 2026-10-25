/**
 * @file RailMonitor.h
 * @brief PDU monitoring node — one supply rail: condition flags, window statistics, page packing (#216).
 *
 * Fed one converted sample per scan (rail mV, current mA). Keeps:
 *   - condition flags (PduRailFlag) with set persistence, clear persistence and hysteresis, so a
 *     brief blip neither raises nor drops a flag;
 *   - window statistics for its PDU_RAIL_* page — average + minimum voltage, average current —
 *     reset each time the page is sent periodically;
 *   - the peak current for the PDU_STATUS page, reset each time that page is sent periodically.
 * It is a FaultSource, so its worst condition reaches HEALTH_n.faultId through aggregateFaults().
 *
 * Hardware-free: the caller converts ADC counts (PduMath) and supplies the clock, so the logic is
 * tested with injected values.
 *
 * @note Static lifetime only (FaultSource registry rule) — construct as a global.
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>
#include <NodeStatus.h>
#include <CANProtocol.h>   // TelemPayload

namespace OpenSkyhawk {
namespace Pdu {

/** @brief A condition that must hold `setMs` to raise and its clear condition hold `clearMs` to drop. */
struct Latch {
    bool     on      = false;
    bool     pending = false;
    uint32_t since   = 0;
    /** @brief Advance the latch. `cond` raises it, `clear` (evaluated while on) drops it. */
    void update(bool cond, bool clear, uint32_t setMs, uint32_t clearMs, uint32_t nowMs);
    void reset() { on = pending = false; }
};

} // namespace Pdu

class RailMonitor : public FaultSource {
public:
    /** @brief Per-rail limits. */
    struct Config {
        TelemPage   page;           ///< PDU_RAIL_12V or PDU_RAIL_5V
        const char* name;           ///< "12V" / "5V" — DiagSerial detail only
        uint16_t    uvMv;           ///< under-voltage threshold
        uint16_t    ovMv;           ///< over-voltage threshold
        uint16_t    ocMa;           ///< over-current threshold
        bool        fuseOpenCheck;  ///< true on the 12 V rail; a blown 5 V fuse powers the node down
    };

    explicit RailMonitor(const Config& cfg);

    /**
     * @brief Feed one scan.
     * @param vMv     Rail voltage, mV.
     * @param iMa     Rail current, mA (signed — residual offset may read slightly negative).
     * @param vRailed Voltage channel at the top of the ADC range (can't measure).
     * @param nowMs   millis().
     * @param armed   false during the power-up blanking window: statistics accumulate but every
     *                flag is held clear (rail ramp + inrush would otherwise trip UV / OC).
     */
    void sample(uint16_t vMv, int32_t iMa, bool vRailed, uint32_t nowMs, bool armed);

    /** @brief Current PduRailFlag bits (all zero = healthy). */
    uint8_t flags() const;

    /**
     * @brief Build this rail's PDU_RAIL_* page from the current window.
     * @param resetWindow true for the periodic send (window restarts); false for an edge send.
     */
    TelemPayload buildPage(bool resetWindow);

    /** @brief Peak current since the last reset, mA (≥ 0). */
    uint16_t peakMa() const { return _peakMa; }
    /** @brief Restart the peak — called when the PDU_STATUS page is sent periodically. */
    void resetPeak() { _peakMa = 0; }

    /** @brief Last converted voltage, mV — DiagSerial / bring-up. */
    uint16_t lastMv() const { return _lastMv; }
    /** @brief Last converted current, mA — DiagSerial / bring-up. */
    int32_t  lastMa() const { return _lastMa; }

    NodeFaultCode faultCode() const override;
    const char*   faultDetail() const override;

private:
    Config      _cfg;
    Pdu::Latch  _uv, _ov, _oc, _fuse, _sensor;
    uint32_t    _sumMv = 0;
    int32_t     _sumMa = 0;
    uint32_t    _n     = 0;
    uint16_t    _minMv = UINT16_MAX;
    uint16_t    _peakMa = 0;
    uint16_t    _lastMv = 0;
    int32_t     _lastMa = 0;
    mutable char _detail[32];  // faultDetail() formats here (const, slow health path)
};

} // namespace OpenSkyhawk
