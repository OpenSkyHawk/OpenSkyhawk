

# File NeedleGauge.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Outputs**](dir_529c528362a647a34d31d0b3b420ca72.md) **>** [**NeedleGauge**](dir_61ced45d99aac20e353c7cae873553bb.md) **>** [**NeedleGauge.cpp**](NeedleGauge_8cpp.md)

[Go to the documentation of this file](NeedleGauge_8cpp.md)


```C++
#ifdef ARDUINO_ARCH_STM32

#include "NeedleGauge.h"
#include <STM32Board.h>

OpenSkyhawk::NeedleGauge::NeedleGauge(uint16_t controlId, uint16_t mask,
                                      MotorDriver& motor, const GaugeCal& cal)
    : _controlId(controlId), _mask(mask), _motor(&motor), _cal(&cal) {}

void OpenSkyhawk::NeedleGauge::configure() {
    _motor->configure();
    _motor->home();
}

void OpenSkyhawk::NeedleGauge::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    int32_t pos = valueToPos(value & _mask);
    _motor->moveTo(pos);
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[Gauge] 0x")); d.print(_controlId, HEX);
        d.print(F(" -> ")); d.println(pos);
    }
}

void OpenSkyhawk::NeedleGauge::update() {
    _motor->update();
}

// Decode the DCS value to a motor position. reverse flips the input; a piecewise curve
// (curveN >= 2) interpolates; otherwise the value maps linearly across [minTravel, maxTravel].
int32_t OpenSkyhawk::NeedleGauge::valueToPos(uint16_t value) const {
    uint16_t v = _cal->reverse ? (uint16_t)(65535u - value) : value;

    if (_cal->curveN >= 2 && _cal->curveIn && _cal->curveOut) {
        const uint16_t* in  = _cal->curveIn;
        const uint16_t* out = _cal->curveOut;
        uint8_t n = _cal->curveN;
        if (v <= in[0])     return out[0];
        if (v >= in[n - 1]) return out[n - 1];
        uint8_t lo = 0, hi = n - 1;            // binary search for the bracketing segment
        while (hi - lo > 1) {
            uint8_t mid = (lo + hi) / 2;
            if (v >= in[mid]) lo = mid; else hi = mid;
        }
        int32_t inLo = in[lo], inHi = in[hi];
        int32_t outLo = out[lo], outHi = out[hi];
        if (inHi == inLo) return outLo;
        return outLo + (int32_t)(v - inLo) * (outHi - outLo) / (inHi - inLo);
    }

    return map((int32_t)v, 0, 65535, _cal->minTravel, _cal->maxTravel);
}

#endif // ARDUINO_ARCH_STM32
```


