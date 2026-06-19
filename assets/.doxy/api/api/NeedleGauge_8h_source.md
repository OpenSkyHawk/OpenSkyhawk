

# File NeedleGauge.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Outputs**](dir_529c528362a647a34d31d0b3b420ca72.md) **>** [**NeedleGauge**](dir_61ced45d99aac20e353c7cae873553bb.md) **>** [**NeedleGauge.h**](NeedleGauge_8h.md)

[Go to the documentation of this file](NeedleGauge_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                       // OutputBase
#include <Drivers/MotorDriver/MotorDriver.h>  // MotorDriver

namespace OpenSkyhawk {

struct GaugeCal {
    int16_t         minTravel;  
    int16_t         maxTravel;  
    bool            reverse;    
    const uint16_t* curveIn;    
    const uint16_t* curveOut;   
    uint8_t         curveN;     
};

class NeedleGauge : public OutputBase {
public:
    NeedleGauge(uint16_t controlId, uint16_t mask, MotorDriver& motor, const GaugeCal& cal);

    void configure() override;

    void onControlPacket(uint16_t controlId, uint16_t value) override;

    void update() override;

#ifdef NEEDLEGAUGE_TEST
    int32_t debugValueToPos(uint16_t value) const { return valueToPos(value); } 
#endif

private:
    uint16_t      _controlId;
    uint16_t      _mask;
    MotorDriver*  _motor;   // caller-owned
    const GaugeCal* _cal;   // caller-owned

    int32_t valueToPos(uint16_t value) const;  // decode → position (linear or piecewise)
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
```


