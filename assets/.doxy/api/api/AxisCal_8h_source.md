

# File AxisCal.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**AxisCal.h**](AxisCal_8h.md)

[Go to the documentation of this file](AxisCal_8h.md)


```C++

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace OpenSkyhawk {

struct AxisCal {
    uint16_t min;       
    uint16_t centre;    
    uint16_t max;       
    uint16_t deadzone;
};

constexpr uint8_t AXIS_CAL_SLOTS = 8;

constexpr uint32_t CAL_MAGIC = ((uint32_t)'O')
                             | ((uint32_t)'S' <<  8)
                             | ((uint32_t)'K' << 16)
                             | ((uint32_t)'C' << 24);

constexpr uint16_t CAL_VERSION = 1;

struct CalBlob {
    uint32_t magic;                  
    uint16_t version;                
    AxisCal  axes[AXIS_CAL_SLOTS];   
    uint16_t crc;                    
};

static_assert(sizeof(AxisCal) == 8,  "AxisCal layout changed — bump CAL_VERSION");
static_assert(sizeof(CalBlob) == 72, "CalBlob layout changed — bump CAL_VERSION");
static_assert(offsetof(CalBlob, crc) == 70, "CRC coverage is offsetof(crc); layout changed");

uint16_t calCrc16(const uint8_t* data, size_t len);

bool axisCalValid(const AxisCal& cal);

uint16_t axisCalApply(const AxisCal& cal, uint16_t raw);

uint16_t calBlobCrc(const CalBlob& blob);

bool calBlobValid(const CalBlob& blob);

void calBlobClear(CalBlob& blob);

void calBlobSeal(CalBlob& blob);

} // namespace OpenSkyhawk
```


