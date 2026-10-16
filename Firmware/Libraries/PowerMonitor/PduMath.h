/**
 * @file PduMath.h
 * @brief PDU monitoring node — pure conversions, no hardware access (#216).
 *
 * Everything here is a function of its arguments, so it is tested with injected values on any
 * target. ADC counts are 16-bit (analogReadResolution(16)); VDDA is in mV.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>
#include <stddef.h>

namespace OpenSkyhawk {
namespace Pdu {

/** @brief Calibration values the conversions need. Defaults = uncalibrated. */
struct CalValues {
    uint16_t vrefintMv = 0;      ///< this chip's Vrefint in mV; 0 = not calibrated (use VDDA_NOMINAL_MV)
    int16_t  zero12DmA = 0;      ///< 12 V current zero offset, 0.1 mA
    int16_t  zero5DmA  = 0;      ///< 5 V current zero offset, 0.1 mA
    uint16_t iGain12   = 10000;  ///< 12 V current gain trim, 10000 = ×1
    uint16_t iGain5    = 10000;  ///< 5 V current gain trim
    uint16_t vGain12   = 10000;  ///< 12 V voltage gain trim
    uint16_t vGain5    = 10000;  ///< 5 V voltage gain trim
    int16_t  ntcOffsetDC = 0;    ///< NTC one-point offset, 0.1 °C
};

/**
 * @brief VDDA in mV from a raw Vrefint reading.
 * @param vrefintRaw  16-bit ADC count of the internal reference (0 = unreadable).
 * @param vrefintMv   This chip's calibrated Vrefint, or 0 when uncalibrated.
 * @return VDDA in mV: Vrefint-tracked when calibrated, VDDA_NOMINAL_MV otherwise (the untrimmed
 *         F103 Vrefint is a worse reference than the regulator's nominal until anchored).
 */
uint16_t vddaMv(uint16_t vrefintRaw, uint16_t vrefintMv);

/** @brief ADC pin voltage in mV for a 16-bit count at the given VDDA. */
uint32_t countsToMv(uint16_t counts, uint16_t vddaMv);

/** @brief 12 V rail in mV from the ADC pin voltage, after the divider and gain trim. */
uint16_t rail12Mv(uint32_t adcMv, uint16_t vGain);
/** @brief 5 V rail in mV from the ADC pin voltage, after the divider and gain trim. */
uint16_t rail5Mv(uint32_t adcMv, uint16_t vGain);

/**
 * @brief Rail current in mA from the INA180 output voltage.
 * @param adcMv   INA180 output at the ADC pin, mV.
 * @param zeroDmA Zero offset to subtract, 0.1 mA (from CAL ZERO).
 * @param iGain   Gain trim, 10000 = ×1.
 * @return Signed mA — a small negative value is residual offset, kept so it stays visible.
 */
int32_t currentMa(uint32_t adcMv, int16_t zeroDmA, uint16_t iGain);

/** @brief Uncorrected current in 0.1 mA (no zero, unity gain) — what CAL ZERO measures. */
int32_t rawCurrentDmA(uint32_t adcMv);

/**
 * @brief Board temperature in 0.1 °C from the NTC channel (ratiometric — VDDA cancels).
 * @param counts  16-bit ADC count of the NTC node.
 * @param offsetDC One-point offset, 0.1 °C.
 * @param fault   Set true when the reading is outside 2–98 % of full scale (open / shorted NTC).
 * @return Temperature in 0.1 °C, or NTC_TEMP_UNAVAILABLE (NodeStatus.h) when `fault`.
 */
int16_t ntcTempDC(uint16_t counts, int16_t offsetDC, bool& fault);

/** @brief True when a 16-bit count is at the top rail (≥ 99.5 % FS) — the channel can't measure. */
bool railed(uint16_t counts);

/** @brief CRC-32 (IEEE 802.3, reflected 0xEDB88320) over `len` bytes. */
uint32_t crc32(const uint8_t* data, size_t len);

/** @brief Clamp an int32 into int16 range. */
int16_t clampI16(int32_t v);
/** @brief Clamp an int32 into uint16 range. */
uint16_t clampU16(int32_t v);

/** @brief Store `v` little-endian at `p[0..1]` — a multi-byte TELEM page field. */
inline void putLe16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)(v & 0xFF); p[1] = (uint8_t)(v >> 8); }

} // namespace Pdu
} // namespace OpenSkyhawk
