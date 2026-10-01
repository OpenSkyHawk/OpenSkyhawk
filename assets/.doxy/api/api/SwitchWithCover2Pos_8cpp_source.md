

# File SwitchWithCover2Pos.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**SwitchWithCover2Pos**](dir_0a1cdbbabc21d9a1747f90460cf2cff5.md) **>** [**SwitchWithCover2Pos.cpp**](SwitchWithCover2Pos_8cpp.md)

[Go to the documentation of this file](SwitchWithCover2Pos_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include "SwitchWithCover2Pos.h"
#include <CANProtocol.h>  // sendBatched, canIdEvt, ControlPacket
#include <STM32Board.h>

namespace OpenSkyhawk {

SwitchWithCover2Pos::SwitchWithCover2Pos(uint16_t switchId, uint16_t coverId, PinRef pin,
                                         bool reverse)
    : Switch2Pos(switchId, pin, reverse),
      _coverId(coverId),
      _state(Cover::OFF_CLOSED),
      _target(Cover::OFF_CLOSED),
      _lastStepMs(0) {}

void SwitchWithCover2Pos::emit(bool active, bool init) {
    _target = active ? Cover::ON_OPEN : Cover::OFF_CLOSED;

    // Boot / SYNC_REQ: re-assert BOTH frames, not just the one that would have changed. Seeding
    // the machine at the far end makes the sequencer walk the whole travel in the right order —
    // cover first when opening, switch first when closing (FirmwarePlan D5).
    if (init) _state = active ? Cover::OFF_CLOSED : Cover::ON_OPEN;

    // Back-date the step clock so the first frame goes on the next poll(): COVER_DELAY_MS is the
    // gap *between* the pair, not latency added before the first one.
    _lastStepMs = millis() - COVER_DELAY_MS;
}

void SwitchWithCover2Pos::poll() {
    Switch2Pos::poll();   // debounce; a confirmed change calls emit() above
    step();
}

void SwitchWithCover2Pos::step() {
    if (_state == _target) return;
    if ((uint32_t)(millis() - _lastStepMs) < COVER_DELAY_MS) return;

    // One position per step, and the frame is whatever that transition means.
    if (_target > _state) {
        _state = (Cover)((uint8_t)_state + 1);
        if (_state == Cover::OFF_OPEN) sendCover(true);     // OFF_CLOSED → OFF_OPEN
        else                           sendSwitch(true);    // OFF_OPEN   → ON_OPEN
    } else {
        _state = (Cover)((uint8_t)_state - 1);
        if (_state == Cover::OFF_OPEN) sendSwitch(false);   // ON_OPEN    → OFF_OPEN
        else                           sendCover(false);    // OFF_OPEN   → OFF_CLOSED
    }
    _lastStepMs = millis();
}

void SwitchWithCover2Pos::sendCover(bool open) {
    CANProtocol::sendBatched(canIdEvt(NODE_ID),
                             ControlPacket{_coverId, static_cast<uint16_t>(open ? 1u : 0u)});
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[SWC] cover 0x")); d.print(_coverId, HEX);
        d.print(F(": ")); d.println(open ? 1 : 0);
    }
}

void SwitchWithCover2Pos::sendSwitch(bool on) {
    CANProtocol::sendBatched(canIdEvt(NODE_ID),
                             ControlPacket{_controlId, static_cast<uint16_t>(on ? 1u : 0u)});
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[SWC] switch 0x")); d.print(_controlId, HEX);
        d.print(F(": ")); d.println(on ? 1 : 0);
    }
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


