

# File TelemSchedule.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**TelemSchedule.h**](TelemSchedule_8h.md)

[Go to the documentation of this file](TelemSchedule_8h.md)


```C++

#pragma once
#include <stdint.h>

namespace OpenSkyhawk {
namespace Pdu {

class TelemSchedule {
public:
    static constexpr uint8_t  PAGES           = 3;     
    static constexpr uint32_t TELEM_PERIOD_MS = 1000;  
    static constexpr uint32_t EDGE_MIN_MS     = 100;   

    void start(uint32_t nowMs);

    int8_t next(uint32_t nowMs, const uint8_t flags[PAGES], bool& periodic);

    void sent(uint8_t slot, uint8_t flags, uint32_t nowMs);

private:
    uint32_t _lastPeriodic = 0;
    uint8_t  _rr = 0;
    uint32_t _lastSent[PAGES]  = {};
    uint8_t  _lastFlags[PAGES] = {};
};

} // namespace Pdu
} // namespace OpenSkyhawk
```


