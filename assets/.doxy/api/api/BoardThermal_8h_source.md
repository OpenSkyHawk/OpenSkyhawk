

# File BoardThermal.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**BoardThermal.h**](BoardThermal_8h.md)

[Go to the documentation of this file](BoardThermal_8h.md)


```C++

#pragma once
#include <stdint.h>
#include <NodeStatus.h>

namespace OpenSkyhawk {

class BoardThermal : public FaultSource {
public:
    BoardThermal() = default;

    void sample(int16_t tempDC, bool ntcFault, bool armed);

    uint8_t flags() const;

    int16_t tempDC() const { return _tempDC; }

    NodeFaultCode faultCode() const override;
    const char*   faultDetail() const override;

private:
    int16_t _tempDC   = NTC_TEMP_UNAVAILABLE;
    bool    _warn     = false;
    bool    _crit     = false;
    bool    _ntcFault = false;
};

} // namespace OpenSkyhawk
```


