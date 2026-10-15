

# File TelemSchedule.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**TelemSchedule.cpp**](TelemSchedule_8cpp.md)

[Go to the documentation of this file](TelemSchedule_8cpp.md)


```C++
#include "TelemSchedule.h"

namespace OpenSkyhawk {
namespace Pdu {

void TelemSchedule::start(uint32_t nowMs) {
    _lastPeriodic = nowMs;
    _rr = 0;
    for (uint8_t i = 0; i < PAGES; i++) { _lastSent[i] = nowMs - EDGE_MIN_MS; _lastFlags[i] = 0; }
}

int8_t TelemSchedule::next(uint32_t nowMs, const uint8_t flags[PAGES], bool& periodic) {
    if (nowMs - _lastPeriodic >= TELEM_PERIOD_MS) {
        _lastPeriodic += TELEM_PERIOD_MS;
        if (nowMs - _lastPeriodic >= TELEM_PERIOD_MS) _lastPeriodic = nowMs;  // fell behind: don't burst
        const uint8_t slot = _rr;
        _rr = (uint8_t)((_rr + 1) % PAGES);
        periodic = true;
        return (int8_t)slot;
    }
    for (uint8_t i = 0; i < PAGES; i++) {
        if (flags[i] == _lastFlags[i]) continue;
        if (nowMs - _lastSent[i] < EDGE_MIN_MS) continue;
        periodic = false;
        return (int8_t)i;
    }
    return -1;
}

void TelemSchedule::sent(uint8_t slot, uint8_t flags, uint32_t nowMs) {
    if (slot >= PAGES) return;
    _lastSent[slot]  = nowMs;
    _lastFlags[slot] = flags;
}

} // namespace Pdu
} // namespace OpenSkyhawk
```


