

# File PduCalStore.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**PduCalStore.h**](PduCalStore_8h.md)

[Go to the documentation of this file](PduCalStore_8h.md)


```C++

#pragma once
#include <stdint.h>
#include "PduConfig.h"
#include "PduMath.h"

namespace OpenSkyhawk {
namespace Pdu {

enum CalFlag : uint8_t {
    CAL_VDDA = 0x01,  
    CAL_ZERO = 0x02,  
    CAL_GAIN = 0x04,  
    CAL_NTC  = 0x08,  
};

struct __attribute__((packed)) CalRecord {
    uint32_t magic;        
    uint8_t  version;      
    uint8_t  calFlags;     
    uint16_t vrefintMv;    
    int16_t  zero12DmA;    
    int16_t  zero5DmA;     
    uint16_t iGain12;      
    uint16_t iGain5;       
    uint16_t vGain12;      
    uint16_t vGain5;       
    int16_t  ntcOffsetDC;  
    uint16_t rsvd;         
    uint32_t seq;          
    uint32_t crc;          
};
static_assert(sizeof(CalRecord) == 32, "CalRecord must be 32 bytes");

static constexpr uint32_t CAL_MAGIC   = 0x4C414350;  
static constexpr uint8_t  CAL_VERSION = 1;
static constexpr uint32_t CAL_PAGE_SIZE = 1024;
static constexpr uint8_t  CAL_SLOTS   = CAL_PAGE_SIZE / sizeof(CalRecord);  // 32

CalRecord calDefaults();
void calSeal(CalRecord& rec);
bool calValid(const CalRecord& rec);
CalValues calValues(const CalRecord& rec);

class CalStore {
public:
    explicit CalStore(uint32_t pageAddr = PDU_CAL_PAGE_ADDR) : _page(pageAddr) {}

    bool load(CalRecord& out) const;

    bool save(CalRecord& rec);

    bool erase();

    uint8_t usedSlots() const;

private:
    uint32_t _page;
    const CalRecord* slot(uint8_t i) const {
        return reinterpret_cast<const CalRecord*>(_page + (uint32_t)i * sizeof(CalRecord));
    }
};

} // namespace Pdu
} // namespace OpenSkyhawk
```


