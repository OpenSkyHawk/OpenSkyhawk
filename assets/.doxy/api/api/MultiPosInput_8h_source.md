

# File MultiPosInput.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**MultiPosInput**](dir_7bc1eaced50854697a5557e9b0a7cd3c.md) **>** [**MultiPosInput.h**](MultiPosInput_8h.md)

[Go to the documentation of this file](MultiPosInput_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase

namespace OpenSkyhawk {

class MultiPosInput : public InputBase {
public:
    static constexpr uint16_t NO_POSITION = 0xFFFF;

    void poll() override;

    void forceReport() override;

    uint16_t position() const { return _lastPos; }

#ifdef MULTIPOS_TEST
    uint16_t emitCount() const { return _emitCount; }
#endif

protected:
    MultiPosInput(uint16_t controlId, uint8_t numPositions, uint16_t debounceMs);

    virtual uint16_t readRaw() = 0;

    uint16_t _controlId;     
    uint8_t  _numPositions;  

private:
    void emit(uint16_t pos, bool init = false);  

    uint16_t _debounceMs;       // stability window (ms); 0 = confirm next poll
    uint16_t _lastPos;          // last confirmed / emitted index
    uint16_t _pendingPos;       // last raw reading (pre-confirm)
    uint32_t _debounceStartMs;  // millis() when _pendingPos last changed
    bool     _initialized;      // false until forceReport(); poll() no-op before this
#ifdef MULTIPOS_TEST
    uint16_t _emitCount = 0;    // test-only EVT counter
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


