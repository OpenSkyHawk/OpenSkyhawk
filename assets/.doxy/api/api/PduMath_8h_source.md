

# File PduMath.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**PduMath.h**](PduMath_8h.md)

[Go to the documentation of this file](PduMath_8h.md)


```C++

#pragma once
#include <stdint.h>
#include <stddef.h>

namespace OpenSkyhawk {
namespace Pdu {

struct CalValues {
    uint16_t vrefintMv = 0;      
    int16_t  zero12DmA = 0;      
    int16_t  zero5DmA  = 0;      
    uint16_t iGain12   = 10000;  
    uint16_t iGain5    = 10000;  
    uint16_t vGain12   = 10000;  
    uint16_t vGain5    = 10000;  
    int16_t  ntcOffsetDC = 0;    
};

uint16_t vddaMv(uint16_t vrefintRaw, uint16_t vrefintMv);

uint32_t countsToMv(uint16_t counts, uint16_t vddaMv);

uint16_t rail12Mv(uint32_t adcMv, uint16_t vGain);
uint16_t rail5Mv(uint32_t adcMv, uint16_t vGain);

int32_t currentMa(uint32_t adcMv, int16_t zeroDmA, uint16_t iGain);

int32_t rawCurrentDmA(uint32_t adcMv);

int16_t ntcTempDC(uint16_t counts, int16_t offsetDC, bool& fault);

bool railed(uint16_t counts);

uint32_t crc32(const uint8_t* data, size_t len);

int16_t clampI16(int32_t v);
uint16_t clampU16(int32_t v);

} // namespace Pdu
} // namespace OpenSkyhawk
```


