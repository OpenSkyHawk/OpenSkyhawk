

# File RotaryEncoder.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**RotaryEncoder**](dir_d61b64c3ddc6557ee529e3725418e11d.md) **>** [**RotaryEncoder.cpp**](RotaryEncoder_8cpp.md)

[Go to the documentation of this file](RotaryEncoder_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include "RotaryEncoder.h"
#include <CANProtocol.h>  // sendBatched, canIdEvtRel, canIdEvtDir, ControlPacket
#include <STM32Board.h>

namespace OpenSkyhawk {

RotaryEncoder::RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent, EncoderMode mode, int16_t step)
    : _controlId(controlId),
      _pinA(pinA),
      _pinB(pinB),
      _mode(mode),
      _step(step),
      _stepsPerDetent((uint8_t)stepsPerDetent),
      _lastState(0),
      _delta(0),
      _initialized(false) {}

void RotaryEncoder::configure() {
    _pinA.configureAsInput();
    _pinB.configureAsInput();
}

uint8_t RotaryEncoder::readState() {
    return (uint8_t)(((_pinA.read() ? 1u : 0u) << 1) | (_pinB.read() ? 1u : 0u));
}

void RotaryEncoder::emit(int8_t dir) {
    // dir = +1 (CW) / -1 (CCW). REL → ±step on the relative frame; DIR → ±1 on the directional
    // frame. The bridge reads the payload as int16 and formats by frame (%+d vs INC/DEC).
    const int16_t  value = (_mode == EncoderMode::Rel) ? (int16_t)(dir * _step) : (int16_t)dir;
    const uint32_t frame = (_mode == EncoderMode::Rel) ? canIdEvtRel(NODE_ID) : canIdEvtDir(NODE_ID);
    CANProtocol::sendBatched(frame, ControlPacket{_controlId, (uint16_t)value});
#ifdef ROTARYENCODER_TEST
    _emitCount++;
    _lastValue = value;
    _lastFrame = frame;
#endif
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[ENC] 0x")); d.print(_controlId, HEX);
        d.print(_mode == EncoderMode::Rel ? F(" REL ") : F(" DIR "));
        d.println(value);   // signed: + = CW, - = CCW
    }
}

void RotaryEncoder::decode(uint8_t state) {
    // Quadrature transition table — ported verbatim from DcsBios RotaryEncoder (Encoders.h).
    switch (_lastState) {
        case 0: if (state == 2) _delta--; if (state == 1) _delta++; break;
        case 1: if (state == 0) _delta--; if (state == 3) _delta++; break;
        case 2: if (state == 3) _delta--; if (state == 0) _delta++; break;
        case 3: if (state == 1) _delta--; if (state == 2) _delta++; break;
    }
    _lastState = state;

    if (_delta >= (int8_t)_stepsPerDetent) {       // clockwise
        emit(+1);
        _delta -= (int8_t)_stepsPerDetent;
    }
    if (_delta <= -(int8_t)_stepsPerDetent) {      // counter-clockwise
        emit(-1);
        _delta += (int8_t)_stepsPerDetent;
    }
}

void RotaryEncoder::forceReport() {
    _lastState   = readState();   // resync so the first poll sees no spurious transition
    _delta       = 0;
    _initialized = true;
    // No EVT — a relative encoder has no absolute state to report at boot / SYNC.
}

void RotaryEncoder::poll() {
    if (!_initialized) return;
    decode(readState());
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


