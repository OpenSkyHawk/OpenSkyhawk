

# File I2cHealth.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**I2cHealth**](dir_741d33806df633606a48f25556e87791.md) **>** [**I2cHealth.h**](I2cHealth_8h.md)

[Go to the documentation of this file](I2cHealth_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>

namespace OpenSkyhawk {

class I2cHealth {
public:
    bool i2cHealthy() const { return _i2cHealthy; }

#ifdef DRUMDISPLAY_TEST
    void debugExpireBackoff() { _i2cLastAttempt = millis() - I2C_RETRY_MS - 1; }
#endif

protected:
    virtual bool i2cProbe() = 0;

    bool i2cReachable() {
        const uint32_t now = millis();
        if (!_i2cHealthy && (now - _i2cLastAttempt) < I2C_RETRY_MS) return false;  // tripped → back off
        _i2cLastAttempt = now;
        return (_i2cHealthy = i2cProbe());
    }

    static constexpr uint32_t I2C_RETRY_MS = 2000;

    ~I2cHealth() = default;  // protected, non-virtual: a mixin, never deleted through this type

private:
    bool     _i2cHealthy     = true;
    uint32_t _i2cLastAttempt = 0;
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


