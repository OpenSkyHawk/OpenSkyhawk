

# File MotorDriver.h

[**File List**](files.md) **>** [**Drivers**](dir_da1b6a20235952b69490534d482f5898.md) **>** [**MotorDriver**](dir_7cabaf4812e32c14ff26922d3804a645.md) **>** [**MotorDriver.h**](MotorDriver_8h.md)

[Go to the documentation of this file](MotorDriver_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>

namespace OpenSkyhawk {

class MotorDriver {
public:
    virtual ~MotorDriver() = default;

    virtual void configure() = 0;

    virtual void home() = 0;

    virtual void moveTo(int32_t pos) = 0;

    virtual void update() = 0;

    virtual int32_t position() const = 0;
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
```


