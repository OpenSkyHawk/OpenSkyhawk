

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
    : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, mode, step,
                    /*momentumFilter*/ false, /*fastStep*/ 0) {}

RotaryEncoder::RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent, EncoderMode mode, int16_t step,
                             bool momentumFilter, int16_t fastStep)
    : _controlId(controlId),
      _pinA(pinA),
      _pinB(pinB),
      _mode(mode),
      _step(step),
      _stepsPerDetent((uint8_t)stepsPerDetent),
      _lastState(0),
      _delta(0),
      _pendingDetents(0),
      _pendingFast(0),
      _filter(momentumFilter),
      _fastStep(mode == EncoderMode::Rel ? fastStep : 0),   // DIR carries ±1 only — no speed
      _momentum(0),
      _lastDetentMs(0),
      _hasLastDetent(false),
      _sampled(false),
      _initialized(false) {}

void RotaryEncoder::configure() {
    _pinA.configureAsInput();
    _pinB.configureAsInput();
}

uint8_t RotaryEncoder::readState() {
    return (uint8_t)(((_pinA.read() ? 1u : 0u) << 1) | (_pinB.read() ? 1u : 0u));
}

void RotaryEncoder::emit(int16_t value) {
    // REL → one frame carrying a coalesced magnitude (a burst reads as one bigger twist, same DCS
    // result, fewer frames); DIR → ±1 per frame (DCS steps one position per INC/DEC, so a burst
    // must stay one frame per detent — the caller loops). drainPending() computes the value.
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
    // sampler ticks, poll() otherwise; never both. Completed detents go to the pending
    // counters; emission always happens loop-side in drainPending().
    int8_t dir = 0;
    switch (_lastState) {
        case 0: if (state == 2) dir = -1; if (state == 1) dir = +1; break;
        case 1: if (state == 0) dir = -1; if (state == 3) dir = +1; break;
        case 2: if (state == 3) dir = -1; if (state == 0) dir = +1; break;
        case 3: if (state == 1) dir = -1; if (state == 2) dir = +1; break;
    }
    _lastState = state;

    if (_filter) {
        // Momentum filter — DcsBios RotaryAcceleratedEncoder: a transition against non-zero
        // momentum is a bounce on a noisy encoder; drop it and only decay the momentum.
        const int8_t maxM = (int8_t)(MAX_MOMENTUM * (int8_t)_stepsPerDetent);
        if (dir > 0) {
            if (_momentum >= 0) { if (_momentum < maxM) _momentum++; }
            else                { dir = 0; _momentum++; }
        } else if (dir < 0) {
            if (_momentum <= 0) { if (_momentum > -maxM) _momentum--; }
            else                { dir = 0; _momentum--; }
        } else if ((uint32_t)(millis() - _lastDetentMs) > STOPPED_THRESHOLD_MS) {
            _momentum = 0;   // knob at rest — a reversal from here is genuine
        }
    }

    _delta += dir;
    if (_delta >= (int8_t)_stepsPerDetent) {       // clockwise
        countDetent(+1);
        _delta -= (int8_t)_stepsPerDetent;
    }
    if (_delta <= -(int8_t)_stepsPerDetent) {      // counter-clockwise
        countDetent(-1);
        _delta += (int8_t)_stepsPerDetent;
    }
}

void RotaryEncoder::countDetent(int8_t dir) {
    // Pending cap: REL coalesces on drain, so deep accumulation is cheap (one frame);
    // DIR emits one CAN frame per detent — an absurd backlog (multi-second stall while
    // spinning) would flood the 16-slot TX ring and silently drop frames, so clamp it.
    const int8_t cap = (_mode == EncoderMode::Dir) ? 8 : INT8_MAX;

    // Speed is classified HERE, per detent, not at drain time: a loop stalled by an OLED flush
    // drains several detents at once and their spacing would be lost. Only the accelerated
    // class pays for the millis() read; the plain encoder never enters this branch.
    bool fast = false;
    if (_filter || _fastStep != 0) {
        const uint32_t now = millis();
        fast = _fastStep != 0 && _hasLastDetent &&
               (uint32_t)(now - _lastDetentMs) < FAST_THRESHOLD_MS;
        _lastDetentMs  = now;
        _hasLastDetent = true;
    }

    if (fast) {
        if (dir > 0) { if (_pendingFast <  cap) _pendingFast = _pendingFast + 1; }
        else         { if (_pendingFast > -cap) _pendingFast = _pendingFast - 1; }
    } else {
        if (dir > 0) { if (_pendingDetents <  cap) _pendingDetents = _pendingDetents + 1; }
        else         { if (_pendingDetents > -cap) _pendingDetents = _pendingDetents - 1; }
    }
}

void RotaryEncoder::sampleTick() {
    // Generic InputBase high-rate hook — the encoder does not know (or care) who calls it.
    // ISR-safe: cached pin reads + the transition table, no CAN.
    // Decline ownership unless BOTH pins are sampler-refreshed sources (ShiftBus '165):
    // a GPIO/MCP-pinned encoder on a mixed node keeps its loop-rate decode unchanged —
    // its cache only refreshes at loop rate, so sampler ownership would gain nothing and
    // silently rewire behavior the SHIFTBUS_ISR_HZ flag has no business touching.
    if (!(_pinA.isSampledSource() && _pinB.isSampledSource())) return;
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
    int8_t f = _pendingFast;
    _pendingDetents = 0;
    _pendingFast    = 0;
    interrupts();
    if (d == 0 && f == 0) return;
#ifdef ROTARYENCODER_TEST
    _netDetents += d + f;
#endif

    if (_mode == EncoderMode::Dir) {
        while (d > 0) { emit(+1); d--; }
        while (d < 0) { emit(-1); d++; }
        return;
    }

    if (f == 0) {
        // Plain REL path (unchanged): coalesce, chunked in whole detents so detents×step stays
        // within int16. Magnitude-based so a negative step (direction-invert config) chunks
        // correctly too.
        const int16_t mag = (_step < 0) ? (int16_t)-_step : _step;
        const int16_t maxChunk = (mag > 1) ? (int16_t)(32767 / mag) : 127;
        while (d != 0) {
            int8_t chunk = d;
            if (chunk >  maxChunk) chunk = (int8_t)maxChunk;
            if (chunk < -maxChunk) chunk = (int8_t)-maxChunk;
            emit((int16_t)(chunk * _step));
            d = (int8_t)(d - chunk);
        }
        return;
    }

    // Accelerated REL: slow and fast detents drain together as one signed magnitude, split
    // only if it overflows int16 (variable_step adds, so a split sums to the same result).
    int32_t total = (int32_t)d * _step + (int32_t)f * _fastStep;
    while (total != 0) {
        int32_t chunk = total;
        if (chunk >  32767) chunk =  32767;
        if (chunk < -32767) chunk = -32767;
        emit((int16_t)chunk);
        total -= chunk;
    }
}

void RotaryEncoder::forceReport() {
    // Resync so the first decode sees no spurious transition. Masked against a sampler
    // (SYNC_REQ arrives in loop context).
    noInterrupts();
    _lastState      = readState();
    _delta          = 0;
    _pendingDetents = 0;
    _pendingFast    = 0;
    _momentum       = 0;
    _hasLastDetent  = false;   // the first detent after a resync is never "fast"
    _lastDetentMs   = millis();
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
```


