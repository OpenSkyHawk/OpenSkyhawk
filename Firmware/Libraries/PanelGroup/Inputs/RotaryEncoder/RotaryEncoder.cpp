/**
 * @file RotaryEncoder.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

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
      _pendingDetents(0),
      _sampled(false),
      _initialized(false) {}

void RotaryEncoder::configure() {
    _pinA.configureAsInput();
    _pinB.configureAsInput();
}

uint8_t RotaryEncoder::readState() {
    return (uint8_t)(((_pinA.read() ? 1u : 0u) << 1) | (_pinB.read() ? 1u : 0u));
}

void RotaryEncoder::emit(int8_t detents) {
    // REL → one frame carrying detents×step (coalesced burst reads as one bigger twist,
    // same DCS result, fewer frames); DIR → ±1 per frame (DCS steps one position per
    // INC/DEC, so a burst must stay one frame per detent — the caller loops).
    const int16_t  value = (_mode == EncoderMode::Rel) ? (int16_t)(detents * _step)
                                                       : (int16_t)detents;
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
    // Runs wherever the sampling happens — sampleTick() (possibly ISR context) once a
    // sampler ticks, poll() otherwise; never both. Completed detents go to _pendingDetents;
    // emission always happens loop-side in drainPending().
    switch (_lastState) {
        case 0: if (state == 2) _delta--; if (state == 1) _delta++; break;
        case 1: if (state == 0) _delta--; if (state == 3) _delta++; break;
        case 2: if (state == 3) _delta--; if (state == 0) _delta++; break;
        case 3: if (state == 1) _delta--; if (state == 2) _delta++; break;
    }
    _lastState = state;

    // Pending cap: REL coalesces on drain, so deep accumulation is cheap (one frame);
    // DIR emits one CAN frame per detent — an absurd backlog (multi-second stall while
    // spinning) would flood the 16-slot TX ring and silently drop frames, so clamp it.
    const int8_t cap = (_mode == EncoderMode::Dir) ? 8 : INT8_MAX;
    if (_delta >= (int8_t)_stepsPerDetent) {       // clockwise
        if (_pendingDetents <  cap) _pendingDetents = _pendingDetents + 1;
        _delta -= (int8_t)_stepsPerDetent;
    }
    if (_delta <= -(int8_t)_stepsPerDetent) {      // counter-clockwise
        if (_pendingDetents > -cap) _pendingDetents = _pendingDetents - 1;
        _delta += (int8_t)_stepsPerDetent;
    }
}

void RotaryEncoder::sampleTick() {
    // Generic InputBase high-rate hook — the encoder does not know (or care) who calls it.
    // ISR-safe: cached pin reads + the transition table, no CAN.
    // Claim ownership BEFORE the initialized check: once a sampler ticks at all, poll()
    // must never decode again, or a poll between forceReport() and the next tick could be
    // preempted mid-decode by this ISR (torn _delta/_lastState).
    _sampled = true;
    if (!_initialized) return;
    decode(readState());
}

void RotaryEncoder::drainPending() {
    // Read-and-clear must be atomic against a sampler ticking from ISR context. Masking
    // unconditionally costs a few cycles and spares this class any knowledge of whether
    // (or from where) a sampler runs.
    noInterrupts();
    int8_t d = _pendingDetents;
    _pendingDetents = 0;
    interrupts();
    if (d == 0) return;
#ifdef ROTARYENCODER_TEST
    _netDetents += d;
#endif

    if (_mode == EncoderMode::Rel) {
        // Coalesce, chunked so detents×step stays within int16. Magnitude-based so a
        // negative step (direction-invert config) chunks correctly too.
        const int16_t mag = (_step < 0) ? (int16_t)-_step : _step;
        const int16_t maxChunk = (mag > 1) ? (int16_t)(32767 / mag) : 127;
        while (d != 0) {
            int8_t chunk = d;
            if (chunk >  maxChunk) chunk = (int8_t)maxChunk;
            if (chunk < -maxChunk) chunk = (int8_t)-maxChunk;
            emit(chunk);
            d = (int8_t)(d - chunk);
        }
    } else {
        while (d > 0) { emit(+1); d--; }
        while (d < 0) { emit(-1); d++; }
    }
}

void RotaryEncoder::forceReport() {
    // Resync so the first decode sees no spurious transition. Masked against a sampler
    // (SYNC_REQ arrives in loop context).
    noInterrupts();
    _lastState      = readState();
    _delta          = 0;
    _pendingDetents = 0;
    _initialized    = true;
    interrupts();
    // No EVT — a relative encoder has no absolute state to report at boot / SYNC.
}

void RotaryEncoder::poll() {
    if (!_initialized) return;
    // Once a sampler ticks, it owns the decode; poll() only drains.
    if (!_sampled) decode(readState());
    drainPending();
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
