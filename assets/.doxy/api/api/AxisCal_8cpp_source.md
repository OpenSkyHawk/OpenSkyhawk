

# File AxisCal.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**AxisCal.cpp**](AxisCal_8cpp.md)

[Go to the documentation of this file](AxisCal_8cpp.md)


```C++
#include "AxisCal.h"

#include <string.h>

namespace OpenSkyhawk {

uint16_t calCrc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000u) ? (uint16_t)((uint16_t)(crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

bool axisCalValid(const AxisCal& cal) {
    return (uint32_t)cal.min    + cal.deadzone < cal.centre
        && (uint32_t)cal.centre + cal.deadzone < cal.max;
}

uint16_t axisCalApply(const AxisCal& cal, uint16_t raw) {
    if (!axisCalValid(cal)) return raw;   // uncalibrated — identity passthrough
    if (raw <= cal.min)     return 0;
    if (raw >= cal.max)     return 65535;

    // Lower segment maps [min, centre) onto [0, 32768); upper maps [centre, max) onto
    // [32768, 65535). Both divisors are >= 1 because axisCalValid passed. Note that the
    // degenerate cases (centre == min + 1, centre == max - 1) leave their segment with no
    // interior value at all, so neither branch can be entered with a zero numerator range.
    if (raw < cal.centre) {
        return (uint16_t)(((uint32_t)(raw - cal.min) * 32768u)
                          / (uint32_t)(cal.centre - cal.min));
    }
    return (uint16_t)(32768u + ((uint32_t)(raw - cal.centre) * 32767u)
                               / (uint32_t)(cal.max - cal.centre));
}

uint16_t calBlobCrc(const CalBlob& blob) {
    return calCrc16(reinterpret_cast<const uint8_t*>(&blob), offsetof(CalBlob, crc));
}

bool calBlobValid(const CalBlob& blob) {
    return blob.magic   == CAL_MAGIC
        && blob.version == CAL_VERSION
        && blob.crc     == calBlobCrc(blob);
}

void calBlobClear(CalBlob& blob) {
    memset(&blob, 0, sizeof(blob));
}

void calBlobSeal(CalBlob& blob) {
    blob.magic   = CAL_MAGIC;
    blob.version = CAL_VERSION;
    blob.crc     = calBlobCrc(blob);
}

} // namespace OpenSkyhawk
```


