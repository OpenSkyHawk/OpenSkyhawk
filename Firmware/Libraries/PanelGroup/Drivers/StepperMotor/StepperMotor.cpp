#ifdef ARDUINO_ARCH_STM32

#include <Drivers/StepperMotor/StepperMotor.h>

namespace OpenSkyhawk {

// SwitecX25 default acceleration table (cumulative accel-step → inter-step delay µs).
// Last delayUs (600) sets the top speed; last stepThreshold (300) is maxVel.
const AccelPoint kSwitecDefaultAccel[5] = {
    {  20, 3000 },
    {  50, 1500 },
    { 100, 1000 },
    { 150,  800 },
    { 300,  600 },
};

namespace {
// Homing seek rate (µs/step). MUST stay under the air-core motor's start-stop speed (X27 family
// ωss ≈ 258°/s ≈ 774 steps/s ≈ 1290 µs/step) or the open-loop seek slips and the zero wanders
// between homes. The former 800 µs (1250 steps/s) sat right at that edge — the cause of the
// "home left, then home right" band-swap. Overridable per motor via StepperConfig::homeStepUs.
constexpr uint16_t kHomeStepUsDefault = 2000;  // ≈ 500 steps/s — safe margin; homing is boot-only
constexpr uint8_t  REFINE_BACKOFF     = 40;    // steps to back off the sensor before the 2nd home pass
constexpr uint8_t  HOMING_MARGIN_DIV  = 8;     // STALL over-drive = range/8 — guarantees the stop is reached

// Coil energising sequences. Bit i (LSB first) drives coil i.
const uint8_t SWITEC_6[6] = { 0x9, 0x1, 0x7, 0x6, 0xE, 0x8 };
const uint8_t FULL_4[4]   = { 0x5, 0x6, 0xA, 0x9 };

inline const uint8_t* stateTable(StepPattern p, uint8_t& count) {
    if (p == StepPattern::FULL_4STATE) { count = 4; return FULL_4; }
    count = 6; return SWITEC_6;
}
} // namespace

StepperMotor::StepperMotor(PinRef c1, PinRef c2, PinRef c3, PinRef c4, const StepperConfig& cfg,
                           PinRef homeSense, PinRef sleepEn)
    : _coil{ c1, c2, c3, c4 }, _homeSense(homeSense), _sleepEn(sleepEn), _cfg(cfg),
      _maxVel(cfg.accelN ? cfg.accel[cfg.accelN - 1].stepThreshold : 1),
      _currentStep(0), _targetStep(0), _vel(0), _dir(0), _stopped(true),
      _state(0), _microDelay(0), _time0(0), _homed(false),
      _lastRecalMs(0), _sensorOverride(-1) {}

void StepperMotor::writeIO() {
    uint8_t count;
    const uint8_t* tbl = stateTable(_cfg.pattern, count);
    uint8_t mask = tbl[_state];
    for (uint8_t i = 0; i < 4; i++) {
        _coil[i].writeDeferred(mask & 0x1);   // MCP coils: cache-only; GPIO coils: immediate
        mask >>= 1;
    }
    // Push all four coils in one writePort() per MCP port (8 per-pin I2C writes → 1). For GPIO
    // coils nothing is dirty, so this just scans the (small) expander list — negligible.
    PanelGroup::flushExpanderWrites();
}

void StepperMotor::stepOnce(bool up) {
    uint8_t count;
    stateTable(_cfg.pattern, count);
    if (up) { _currentStep++; _state = (_state + 1) % count; }
    else    { _currentStep--; _state = (_state + count - 1) % count; }
    writeIO();
}

// Ported from SwitecX25::advance — integer trapezoidal accel/decel. Steps once toward
// the target, then adjusts the velocity proxy and looks up the next inter-step delay.
void StepperMotor::advance() {
    if (_currentStep == _targetStep && _vel == 0) {   // settled
        _stopped = true; _dir = 0; _time0 = micros();
        return;
    }
    if (_vel == 0) {                                  // start from rest: pick a direction
        _dir = (_currentStep < _targetStep) ? 1 : -1;
        _vel = 1;
    }
    stepOnce(_dir > 0);

    // steps remaining toward target in the travel direction (negative if past it)
    int32_t delta = (_dir > 0) ? (_targetStep - _currentStep) : (_currentStep - _targetStep);
    if (delta > 0) {
        if (delta < (int32_t)_vel)  _vel--;           // close in → decelerate
        else if (_vel < _maxVel)    _vel++;           // far    → accelerate
        // else cruise at max
    } else {
        _vel--;                                       // at/past target → bleed speed to reverse
    }

    uint8_t i = 0;
    while (i + 1 < _cfg.accelN && _cfg.accel[i].stepThreshold < _vel) i++;
    _microDelay = _cfg.accel[i].delayUs;
    _time0 = micros();
}

void StepperMotor::configure() {
    for (uint8_t i = 0; i < 4; i++) _coil[i].configureAsOutput();
    if (!_sleepEn.isNC())   { _sleepEn.configureAsOutput(); _sleepEn.write(true); } // enable driver
    if (_cfg.home == HomeMode::SENSOR && !_homeSense.isNC()) _homeSense.configureAsInput();
    writeIO(); // energise the initial coil state
}

// live=true bypasses the MCP cache (fresh I2C read) — needed during blocking homing, before
// PanelGroup::loop() starts refreshing the cache. GPIO/ADS reads are already live either way.
bool StepperMotor::sensorAsserted(bool live) const {
    bool raw;
    if (_sensorOverride >= 0) raw = (_sensorOverride != 0);
    else                      raw = live ? _homeSense.readLive() : _homeSense.read();  // true = HIGH
    return _cfg.sensor.activeLow ? !raw : raw;
}

bool StepperMotor::sensorConfirmed() const {
    if (!sensorAsserted(true)) return false;
    uint32_t t0 = millis();
    while ((uint32_t)(millis() - t0) < _cfg.sensor.debounceMs) {
        if (!sensorAsserted(true)) return false;         // bounced off → not stable
    }
    return true;
}

bool StepperMotor::seekHomeBlocking() {
    uint16_t stepUs = _cfg.homeStepUs ? _cfg.homeStepUs : kHomeStepUsDefault;
    for (uint16_t i = 0; i < _cfg.sensor.maxSeekSteps; i++) {
        if (sensorConfirmed()) return true;
        stepOnce(_cfg.homeSeekClockwise);
        delayMicroseconds(stepUs);
    }
    return false; // sensor never confirmed — mis-wired / disconnected
}

void StepperMotor::runToStopBlocking() {
    while (!_stopped) {
        advance();
        delayMicroseconds(_microDelay);
    }
}

void StepperMotor::home() {
    _vel = 0; _dir = 0; _stopped = true;
    uint16_t stepUs = _cfg.homeStepUs ? _cfg.homeStepUs : kHomeStepUsDefault;

    if (_cfg.home == HomeMode::STALL) {
        // Drive into the mechanical end-stop. Drive the mechanical RANGE (+ a small margin),
        // NOT a full revolution — over-driving a stop-limited gauge by the rev/range difference
        // grinds the already-seated rotor and desyncs it, so the zero lands differently each home.
        // rangeSteps == 0 falls back to stepsPerRev (legacy configs).
        uint16_t range = _cfg.rangeSteps ? _cfg.rangeSteps : _cfg.stepsPerRev;
        uint16_t drive = range + range / HOMING_MARGIN_DIV;
        for (uint16_t i = 0; i < drive; i++) {
            stepOnce(_cfg.homeSeekClockwise);
            delayMicroseconds(stepUs);
        }
        _currentStep = _cfg.homePosition;
        _homed = true;
    } else { // SENSOR — two-pass for accuracy
        _homed = seekHomeBlocking();
        if (_homed) {
            _currentStep = _cfg.homePosition;
            for (uint8_t i = 0; i < REFINE_BACKOFF; i++) {  // clear the sensor, then re-seek to refine
                stepOnce(!_cfg.homeSeekClockwise);
                delayMicroseconds(stepUs);
            }
            _homed = seekHomeBlocking();                    // refine pass; a failed re-seek clears homed
            if (_homed) _currentStep = _cfg.homePosition;
        }
        if (!_homed) { _targetStep = _currentStep; return; } // aborted: don't drive blind
    }

    // settle to the resting position
    _targetStep = _currentStep;
    moveTo(_cfg.parkPosition);
    runToStopBlocking();
    _lastRecalMs = millis();
}

void StepperMotor::moveTo(int32_t pos) {
    int32_t t;
    if (_cfg.wrap && _cfg.stepsPerRev > 0) {
        int32_t range = _cfg.stepsPerRev;
        int32_t cur   = ((_currentStep % range) + range) % range;
        int32_t tgt   = ((pos % range) + range) % range;
        int32_t diff  = (tgt - cur) % range;
        if (diff < 0)        diff += range;
        if (diff > range / 2) diff -= range;            // shortest signed path
        t = _currentStep + diff;
    } else {
        t = constrain(pos, (int32_t)_cfg.minPos, (int32_t)_cfg.maxPos);
    }

    int32_t d = t - _targetStep;
    if (d < 0) d = -d;
    if (d <= (int32_t)_cfg.deadband) return;            // within deadband of the current target

    _targetStep = t;
    if (_stopped) { _stopped = false; _time0 = micros(); _microDelay = 0; }
}

void StepperMotor::update() {
    if (!_stopped && (uint32_t)(micros() - _time0) >= _microDelay) advance();

    if (_cfg.autoRecal && !_homeSense.isNC() &&
        (uint32_t)(millis() - _lastRecalMs) > _cfg.recalDebounceMs && sensorAsserted(false)) {
        _currentStep = _cfg.homePosition;               // re-zero; the move toward _targetStep continues
        _lastRecalMs = millis();                         // cache read: loop()'s ~20 ms poll keeps it fresh
    }
}

int32_t StepperMotor::position() const {
    if (_cfg.wrap && _cfg.stepsPerRev > 0) {
        int32_t range = _cfg.stepsPerRev;
        return ((_currentStep % range) + range) % range;
    }
    return _currentStep;
}

StepperConfig makeX27Config(int16_t homePosition, int16_t parkPosition,
                            int16_t minPos, int16_t maxPos,
                            HomeMode home, bool homeSeekClockwise, HomeSensor sensor,
                            bool wrap, uint8_t deadband, bool autoRecal, uint32_t recalDebounceMs,
                            uint16_t stepsPerRev, uint16_t rangeSteps, uint16_t homeStepUs) {
    return StepperConfig{
        stepsPerRev, StepPattern::SWITEC_6STATE, kSwitecDefaultAccel, kSwitecDefaultAccelN,
        home, homeSeekClockwise, sensor,
        homePosition, parkPosition, minPos, maxPos,
        wrap, deadband, autoRecal, recalDebounceMs,
        rangeSteps, homeStepUs,
    };
}

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
