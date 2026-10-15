

# File RailMonitor.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**RailMonitor.h**](RailMonitor_8h.md)

[Go to the documentation of this file](RailMonitor_8h.md)


```C++

#pragma once
#include <stdint.h>
#include <NodeStatus.h>
#include <CANProtocol.h>   // TelemPayload

namespace OpenSkyhawk {
namespace Pdu {

struct Latch {
    bool     on      = false;
    bool     pending = false;
    uint32_t since   = 0;
    void update(bool cond, bool clear, uint32_t setMs, uint32_t clearMs, uint32_t nowMs);
    void reset() { on = pending = false; }
};

} // namespace Pdu

class RailMonitor : public FaultSource {
public:
    struct Config {
        TelemPage   page;           
        const char* name;           
        uint16_t    uvMv;           
        uint16_t    ovMv;           
        uint16_t    ocMa;           
        bool        fuseOpenCheck;  
    };

    explicit RailMonitor(const Config& cfg);

    void sample(uint16_t vMv, int32_t iMa, bool vRailed, uint32_t nowMs, bool armed);

    uint8_t flags() const;

    TelemPayload buildPage(bool resetWindow);

    uint16_t peakMa() const { return _peakMa; }
    void resetPeak() { _peakMa = 0; }

    uint16_t lastMv() const { return _lastMv; }
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
```


