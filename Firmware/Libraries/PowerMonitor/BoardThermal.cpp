#include "BoardThermal.h"
#include "PduConfig.h"

namespace OpenSkyhawk {

void BoardThermal::sample(int16_t tempDC, bool ntcFault, bool armed) {
    _tempDC = ntcFault ? NTC_TEMP_UNAVAILABLE : tempDC;
    if (!armed) { _warn = _crit = _ntcFault = false; return; }

    _ntcFault = ntcFault;
    if (ntcFault) return;   // keep warn/crit as they were — no reading to change them with

    const int16_t h = PDU_TEMP_HYST_DC;
    _warn = _warn ? tempDC > PDU_TEMP_WARN_DC - h : tempDC >= PDU_TEMP_WARN_DC;
    _crit = _crit ? tempDC > PDU_TEMP_CRIT_DC - h : tempDC >= PDU_TEMP_CRIT_DC;
}

uint8_t BoardThermal::flags() const {
    uint8_t f = 0;
    if (_warn)     f |= (uint8_t)PduStatusFlag::TEMP_WARN;
    if (_crit)     f |= (uint8_t)PduStatusFlag::TEMP_CRIT;
    if (_ntcFault) f |= (uint8_t)PduStatusFlag::NTC_FAULT;
    return f;
}

NodeFaultCode BoardThermal::faultCode() const {
    if (_crit)     return NodeFaultCode::OVER_TEMPERATURE;
    if (_ntcFault) return NodeFaultCode::SENSOR_FAULT;
    return NodeFaultCode::NONE;
}

const char* BoardThermal::faultDetail() const {
    if (_crit)     return "board temperature critical";
    if (_ntcFault) return "NTC open or shorted";
    return "";
}

} // namespace OpenSkyhawk
