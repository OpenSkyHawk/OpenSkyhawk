

# File RailMonitor.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**RailMonitor.cpp**](RailMonitor_8cpp.md)

[Go to the documentation of this file](RailMonitor_8cpp.md)


```C++
#include "RailMonitor.h"
#include "PduConfig.h"
#include "PduMath.h"
#include <stdio.h>
#include <string.h>

namespace OpenSkyhawk {
namespace Pdu {

void Latch::update(bool cond, bool clear, uint32_t setMs, uint32_t clearMs, uint32_t nowMs) {
    const bool want = on ? clear : cond;          // the transition we are waiting for
    const uint32_t hold = on ? clearMs : setMs;
    if (!want) { pending = false; return; }
    if (!pending) { pending = true; since = nowMs; }
    if (nowMs - since >= hold) { on = !on; pending = false; }
}

} // namespace Pdu

RailMonitor::RailMonitor(const Config& cfg) : _cfg(cfg) {
    _detail[0] = '\0';
}

void RailMonitor::sample(uint16_t vMv, int32_t iMa, bool vRailed, uint32_t nowMs, bool armed) {
    _lastMv = vMv;
    _lastMa = iMa;
    _sumMv += vMv;
    _sumMa += iMa;
    _n++;
    if (vMv < _minMv) _minMv = vMv;
    if (iMa > (int32_t)_peakMa) _peakMa = Pdu::clampU16(iMa);

    if (!armed) {
        _uv.reset(); _ov.reset(); _oc.reset(); _fuse.reset(); _sensor.reset();
        return;
    }
    const uint16_t h = PDU_V_HYST_MV;
    _uv.update(vMv < _cfg.uvMv, vMv > _cfg.uvMv + h, PDU_V_SET_MS, PDU_CLEAR_MS, nowMs);
    _ov.update(vMv > _cfg.ovMv, vMv + h < _cfg.ovMv, PDU_V_SET_MS, PDU_CLEAR_MS, nowMs);
    _oc.update(iMa > _cfg.ocMa, iMa < (int32_t)_cfg.ocMa * 9 / 10, PDU_OC_SET_MS, PDU_CLEAR_MS, nowMs);
    if (_cfg.fuseOpenCheck)
        _fuse.update(vMv < PDU_FUSE_OPEN_MV, vMv > PDU_FUSE_OPEN_MV + h, PDU_V_SET_MS, PDU_CLEAR_MS, nowMs);
    _sensor.update(vRailed, !vRailed, PDU_V_SET_MS, PDU_CLEAR_MS, nowMs);
}

uint8_t RailMonitor::flags() const {
    uint8_t f = 0;
    if (_uv.on)     f |= (uint8_t)PduRailFlag::UNDER_V;
    if (_ov.on)     f |= (uint8_t)PduRailFlag::OVER_V;
    if (_oc.on)     f |= (uint8_t)PduRailFlag::OVER_CURRENT;
    if (_fuse.on)   f |= (uint8_t)PduRailFlag::FUSE_OPEN;
    if (_sensor.on) f |= (uint8_t)PduRailFlag::SENSOR_FAULT;
    return f;
}

TelemPayload RailMonitor::buildPage(bool resetWindow) {
    const uint16_t vAvg = _n ? (uint16_t)(_sumMv / _n) : _lastMv;
    const uint16_t vMin = _n ? _minMv : _lastMv;
    const int16_t  iAvg = Pdu::clampI16(_n ? _sumMa / (int32_t)_n : _lastMa);

    TelemPayload p;
    p.page    = (uint8_t)_cfg.page;
    p.body[0] = flags();
    p.body[1] = (uint8_t)(vAvg & 0xFF);  p.body[2] = (uint8_t)(vAvg >> 8);
    p.body[3] = (uint8_t)(vMin & 0xFF);  p.body[4] = (uint8_t)(vMin >> 8);
    p.body[5] = (uint8_t)((uint16_t)iAvg & 0xFF);
    p.body[6] = (uint8_t)((uint16_t)iAvg >> 8);

    if (resetWindow) { _sumMv = 0; _sumMa = 0; _n = 0; _minMv = UINT16_MAX; }
    return p;
}

NodeFaultCode RailMonitor::faultCode() const {
    if (_fuse.on)   return NodeFaultCode::FUSE_OPEN;
    if (_oc.on)     return NodeFaultCode::SHORT_CIRCUIT;
    if (_ov.on)     return NodeFaultCode::OVER_VOLTAGE;
    if (_uv.on)     return NodeFaultCode::UNDER_VOLTAGE;
    if (_sensor.on) return NodeFaultCode::SENSOR_FAULT;
    return NodeFaultCode::NONE;
}

const char* RailMonitor::faultDetail() const {
    const char* what = _fuse.on ? "fuse open" : _oc.on ? "over-current" : _ov.on ? "over-voltage"
                     : _uv.on ? "under-voltage" : _sensor.on ? "voltage sensor railed" : nullptr;
    if (!what) return "";
    snprintf(_detail, sizeof(_detail), "%s rail %s", _cfg.name, what);
    return _detail;
}

} // namespace OpenSkyhawk
```


