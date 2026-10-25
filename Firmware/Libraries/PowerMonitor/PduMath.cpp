#include "PduMath.h"
#include "PduConfig.h"
#include <NodeStatus.h>   // NTC_TEMP_UNAVAILABLE
#include <math.h>

namespace OpenSkyhawk {
namespace Pdu {

int16_t clampI16(int32_t v) {
    if (v > INT16_MAX) return INT16_MAX;
    if (v < INT16_MIN) return INT16_MIN;
    return (int16_t)v;
}

uint16_t clampU16(int32_t v) {
    if (v < 0) return 0;
    if (v > UINT16_MAX) return UINT16_MAX;
    return (uint16_t)v;
}

uint16_t vddaMv(uint16_t vrefintRaw, uint16_t vrefintMv) {
    if (vrefintMv == 0 || vrefintRaw == 0) return VDDA_NOMINAL_MV;
    return clampU16((int32_t)((uint32_t)vrefintMv * ADC_FULL / vrefintRaw));
}

uint32_t countsToMv(uint16_t counts, uint16_t vddaMv) {
    return (uint32_t)counts * vddaMv / ADC_FULL;
}

static uint16_t trimmed(uint32_t mv, uint16_t gain) {
    return clampU16((int32_t)(mv * gain / GAIN_UNITY));
}

uint16_t rail12Mv(uint32_t adcMv, uint16_t vGain) {
    return trimmed(adcMv * V12_DIV_NUM / V12_DIV_DEN, vGain);
}

uint16_t rail5Mv(uint32_t adcMv, uint16_t vGain) {
    return trimmed(adcMv * V5_DIV_NUM / V5_DIV_DEN, vGain);
}

int32_t rawCurrentDmA(uint32_t adcMv) {
    return (int32_t)(adcMv * I_MA_PER_MV * 10);
}

int32_t currentMa(uint32_t adcMv, int16_t zeroDmA, uint16_t iGain) {
    // Zero first (it was measured at unity gain), then the gain trim; round to nearest mA.
    const int32_t dmA = (rawCurrentDmA(adcMv) - zeroDmA) * (int32_t)iGain / (int32_t)GAIN_UNITY;
    return dmA >= 0 ? (dmA + 5) / 10 : (dmA - 5) / 10;
}

bool railed(uint16_t counts) {
    return counts >= (uint16_t)(ADC_FULL * 995 / 1000);
}

int16_t ntcTempDC(uint16_t counts, int16_t offsetDC, bool& fault) {
    // Open NTC → node pulled to +3V3 (top); shorted → 0. Either way the reading means nothing.
    fault = counts < ADC_FULL * 2 / 100 || counts > ADC_FULL * 98 / 100;
    if (fault) return NTC_TEMP_UNAVAILABLE;
    const float x = (float)counts / (float)ADC_FULL;               // node / VDDA
    const float rNtc = NTC_BIAS_OHM * x / (1.0f - x);              // NTC is the low side
    const float invT = 1.0f / NTC_T0_K + logf(rNtc / NTC_R0_OHM) / NTC_BETA_K;
    const float dc = (1.0f / invT - 273.15f) * 10.0f;
    return clampI16((int32_t)lroundf(dc) + offsetDC);
}

uint32_t crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

} // namespace Pdu
} // namespace OpenSkyhawk
