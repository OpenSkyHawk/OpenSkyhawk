/**
 * @file PduConfig.h
 * @brief PDU monitoring node — hardware constants and tunable thresholds (#216).
 *
 * Hardware constants match the PDU Rev 0.2.0 schematic (`PCB/Base/Power/PDU`, #202): pin map,
 * divider ratios, INA180A2 gain and shunt, NTC bias. Every threshold is a `PDU_*` macro with a
 * default, so a build overrides it with `build_flags = -DPDU_12V_UV_MV=11200` without a code
 * change; the defaults are starting points to be tuned on the bench against the real load.
 *
 * Units: mV, mA, ms, and tenths of a degree C (`DC`).
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>

// ── Pin map (Rev 0.2.0 netlist) ──────────────────────────────────────────────────
#ifndef PDU_PIN_V12
#define PDU_PIN_V12  PA0   ///< 12V_READ_ADC — 33k/10k divider
#endif
#ifndef PDU_PIN_V5
#define PDU_PIN_V5   PA1   ///< 5V_READ_ADC — 10k/13k divider
#endif
#ifndef PDU_PIN_I12
#define PDU_PIN_I12  PA2   ///< I_12V_ADC — INA180A2 output
#endif
#ifndef PDU_PIN_I5
#define PDU_PIN_I5   PA3   ///< I_5V_ADC — INA180A2 output
#endif
#ifndef PDU_PIN_NTC
#define PDU_PIN_NTC  PA6   ///< NTC_ADC — 10k from +3V3, NTC to GND
#endif

namespace OpenSkyhawk {
namespace Pdu {

// ── Analog front end ─────────────────────────────────────────────────────────────
static constexpr uint32_t ADC_FULL      = 1UL << 16;  ///< analogReadResolution(16), set by STM32Board
static constexpr uint16_t VDDA_NOMINAL_MV = 3300;     ///< assumed VDDA until CAL VDDA anchors Vrefint
static constexpr uint16_t VREFINT_TYP_MV  = 1200;     ///< F103 Vrefint typical (untrimmed, ±~3 %)

/// Rail = ADC × num / den. 12 V: (33k + 10k) / 10k. 5 V: (10k + 13k) / 13k.
static constexpr uint32_t V12_DIV_NUM = 43, V12_DIV_DEN = 10;
static constexpr uint32_t V5_DIV_NUM  = 23, V5_DIV_DEN  = 13;

/// INA180A2 (gain 50 V/V) on a 10 mΩ shunt: I = Vout / (50 × 0.010 Ω) → mA = 2 × mV.
static constexpr uint32_t I_MA_PER_MV = 2;

/// NTC: 10 kΩ ±1 % B3450 to GND, 10 kΩ bias from +3V3 (same rail as VDDA → ratiometric).
static constexpr float    NTC_R0_OHM   = 10000.0f;
static constexpr float    NTC_BETA_K   = 3450.0f;
static constexpr float    NTC_T0_K     = 298.15f;
static constexpr float    NTC_BIAS_OHM = 10000.0f;

static constexpr uint16_t GAIN_UNITY = 10000;  ///< cal gain trims: 10000 = ×1.0000

} // namespace Pdu
} // namespace OpenSkyhawk

// ── Thresholds (override with -D) ─────────────────────────────────────────────────
#ifndef PDU_12V_UV_MV
#define PDU_12V_UV_MV      11400   ///< 12 V under-voltage (ATX −5 %)
#endif
#ifndef PDU_12V_OV_MV
#define PDU_12V_OV_MV      13200   ///< 12 V over-voltage (below the SMBJ12A clamp)
#endif
#ifndef PDU_5V_UV_MV
#define PDU_5V_UV_MV        4750   ///< 5 V under-voltage (−5 %)
#endif
#ifndef PDU_5V_OV_MV
#define PDU_5V_OV_MV        5500   ///< 5 V over-voltage (below the SMBJ6.0A standoff)
#endif
#ifndef PDU_12V_OC_MA
#define PDU_12V_OC_MA       4500   ///< 12 V over-current — 90 % of the 5 A fuse
#endif
#ifndef PDU_5V_OC_MA
#define PDU_5V_OC_MA        1800   ///< 5 V over-current — 90 % of the 2 A fuse
#endif
#ifndef PDU_FUSE_OPEN_MV
#define PDU_FUSE_OPEN_MV    1000   ///< rail below this while the node runs → fuse open (12 V only)
#endif
#ifndef PDU_V_HYST_MV
#define PDU_V_HYST_MV        100   ///< voltage-threshold hysteresis
#endif
#ifndef PDU_V_SET_MS
#define PDU_V_SET_MS          50   ///< a voltage condition must persist this long to set
#endif
#ifndef PDU_OC_SET_MS
#define PDU_OC_SET_MS        100   ///< over-current must persist this long to set
#endif
#ifndef PDU_CLEAR_MS
#define PDU_CLEAR_MS         500   ///< a condition must be gone this long to clear
#endif
#ifndef PDU_TEMP_WARN_DC
#define PDU_TEMP_WARN_DC     600   ///< board temperature warning, 60.0 °C
#endif
#ifndef PDU_TEMP_CRIT_DC
#define PDU_TEMP_CRIT_DC     750   ///< board temperature critical, 75.0 °C
#endif
#ifndef PDU_TEMP_HYST_DC
#define PDU_TEMP_HYST_DC      30   ///< temperature-threshold hysteresis, 3.0 °C
#endif
#ifndef PDU_BOOT_BLANK_MS
#define PDU_BOOT_BLANK_MS    500   ///< ignore rail/thermal faults this long after boot (ramp + inrush)
#endif
#ifndef PDU_SAMPLE_US
#define PDU_SAMPLE_US       1000   ///< rail scan period (1 kHz target)
#endif
#ifndef PDU_ZERO_GUARD_12V_MA
#define PDU_ZERO_GUARD_12V_MA 75   ///< CAL ZERO refuses if the 12 V raw current exceeds this (load present)
#endif
#ifndef PDU_ZERO_GUARD_5V_MA
#define PDU_ZERO_GUARD_5V_MA 120   ///< 5 V guard — higher: the PDU's own ~30–50 mA flows through this shunt
#endif
#ifndef PDU_CAL_PAGE_ADDR
#define PDU_CAL_PAGE_ADDR 0x0800FC00UL   ///< last 1 KB page of the STM32F103C8 (64 KB) — keep out of the image
#endif
