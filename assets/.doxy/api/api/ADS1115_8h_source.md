

# File ADS1115.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**ADS1115.h**](ADS1115_8h.md)

[Go to the documentation of this file](ADS1115_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Adafruit_ADS1X15.h>

class ADS1115 : public Adafruit_ADS1115 {
public:
    using Adafruit_ADS1115::Adafruit_ADS1115;
};

#endif // ARDUINO_ARCH_STM32
```


