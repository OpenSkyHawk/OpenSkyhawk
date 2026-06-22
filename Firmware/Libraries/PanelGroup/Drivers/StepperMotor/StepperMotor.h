/**
 * @file StepperMotor.h
 * @brief Non-blocking 4-wire stepper driver on PinRef coils.
 *
 * @details A control-agnostic MotorDriver for instrument gauge steppers. Drives four
 * coils through PinRef, so the coils may be native STM32 GPIO **or** an MCP23017
 * expander with no code change. The motion engine is ported from Guy Carpenter's
 * SwitecX25 library: an integer (no-FPU) table-driven trapezoidal accel/decel — a
 * `vel` proxy ramps up one step at a time and decelerates once the steps remaining
 * to target fall below `vel`, giving smooth acceleration into and out of every move.
 *
 * One drive profile (StepPattern::SWITEC_6STATE) covers the air-core instrument
 * stepper family on hand — X27.589 / VID-29 / BKA-30 are the same motor electrically;
 * a coil that runs reversed is corrected by swapping two constructor pins, not a new
 * profile. StepPattern::FULL_4STATE is reserved for generic geared 4-wire steppers.
 *
 * Homing: HomeMode::STALL drives into a mechanical end-stop (no sensor); HomeMode::SENSOR
 * seeks a debounced digital home sensor — a micro switch, reed, hall, or opto-interrupter
 * all read identically through one PinRef + an active-level flag.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                       // PinRef
#include <Drivers/MotorDriver/MotorDriver.h>  // MotorDriver base

namespace OpenSkyhawk {

/** @brief How the driver establishes its zero reference at boot. */
enum class HomeMode : uint8_t {
    STALL,   ///< Drive into a mechanical end-stop, then call that position home. No sensor.
    SENSOR   ///< Seek a digital home sensor (switch / reed / hall / opto), then offset to park.
    // ABSOLUTE — future: read an absolute encoder (AS5600); reports position, no seek.
};

/** @brief Coil energising sequence. */
enum class StepPattern : uint8_t {
    SWITEC_6STATE,  ///< 6-state air-core drive (X27.589 / VID-29 / BKA-30).
    FULL_4STATE     ///< 4-state full-step, two coils energised (generic geared 4-wire).
};

/**
 * @brief One point on the acceleration curve (SwitecX25 form).
 * @details `delayUs` is the inter-step delay once cumulative accel-steps reach
 * `stepThreshold`. The last entry's `delayUs` sets the maximum angular velocity.
 */
struct AccelPoint {
    uint16_t stepThreshold;  ///< cumulative steps under acceleration
    uint16_t delayUs;        ///< inter-step delay at/above that threshold
};

/**
 * @brief Home-sensor parameters (HomeMode::SENSOR only).
 * @details Any digital home detector reduces to a debounced level read; the sensor
 * type is purely a wiring/polarity choice handled by @c activeLow.
 */
struct HomeSensor {
    bool     activeLow;     ///< true: sensor asserts LOW; false: asserts HIGH
    uint8_t  debounceMs;    ///< stable-assert confirmation window
    uint16_t maxSeekSteps;  ///< abort the seek after this many steps (mis-wired safety)
};

/**
 * @brief Full per-instance stepper configuration. Authored per sketch (panel wiring).
 * @note @c accel must point at storage that outlives the StepperMotor (e.g. a static
 * const table). @c homePosition / @c parkPosition / @c minPos / @c maxPos are in steps;
 * a sketch may compute them from degrees as @c round(deg*stepsPerRev/360).
 */
struct StepperConfig {
    uint16_t          stepsPerRev;       ///< steps per full revolution (calibrate empirically)
    StepPattern       pattern;           ///< coil drive sequence
    const AccelPoint* accel;             ///< acceleration curve (not owned)
    uint8_t           accelN;            ///< entries in accel[]
    HomeMode          home;              ///< homing strategy
    bool              homeSeekClockwise; ///< direction to seek the home reference
    HomeSensor        sensor;            ///< SENSOR-mode params (ignored for STALL)
    int16_t           homePosition;      ///< step index assigned at the home reference
    int16_t           parkPosition;      ///< step to rest at after homing
    int16_t           minPos;            ///< lower travel clamp for moveTo (ignored if wrap)
    int16_t           maxPos;            ///< upper travel clamp for moveTo (ignored if wrap)
    bool              wrap;              ///< continuous-rotation gauge (shortest-path, no clamp)
    uint8_t           deadband;          ///< ignore target changes within this many steps
    bool              autoRecal;         ///< re-zero to homePosition when the sensor next asserts
    uint32_t          recalDebounceMs;   ///< minimum interval between auto-recals
    // Appended for back-compat: existing positional initialisers omit these → value-initialised to
    // 0 → legacy behaviour (STALL home drives stepsPerRev at the library default rate).
    uint16_t          rangeSteps;        ///< mechanical stop-to-stop travel in steps; STALL home drives this (+margin). 0 → stepsPerRev
    uint16_t          homeStepUs;        ///< homing seek rate µs/step; MUST stay under the motor start-stop rate or the seek slips. 0 → library default (2000)
};

/** @brief Default SwitecX25 acceleration table; fits the X27/VID-29/BKA-30 air-core family. */
extern const AccelPoint kSwitecDefaultAccel[5];
constexpr uint8_t kSwitecDefaultAccelN = 5;

/**
 * @brief Build a StepperConfig with the X27 air-core motor defaults filled in.
 *
 * Bakes the motor-invariant fields — `stepsPerRev`, `pattern` (SWITEC_6STATE), and the default
 * SwitecX25 accel table — so a sketch specifies only the per-gauge wiring/travel. Shared by every
 * X27 / VID-29 / BKA-30 gauge; override any default for a specific panel.
 *
 * @param homePosition       step index at the home reference.
 * @param parkPosition       rest position after homing.
 * @param minPos             lower moveTo travel clamp (ignored if wrap).
 * @param maxPos             upper moveTo travel clamp (ignored if wrap).
 * @param home               homing strategy. Default STALL.
 * @param homeSeekClockwise  seek direction. Default false.
 * @param sensor             home-sensor params (SENSOR mode). Default active-low, 5 ms, 2000 steps.
 * @param wrap               continuous-rotation gauge. Default false.
 * @param deadband           anti-jitter band, steps. Default 1.
 * @param autoRecal          re-zero on sensor crossing. Default false.
 * @param recalDebounceMs    minimum interval between auto-recals. Default 0.
 * @param stepsPerRev        full revolution in steps. Default 1080 (X27/BKA datasheet, 1/3°/step).
 * @param rangeSteps         mechanical stop-to-stop travel in steps = STALL home distance. Default 945
 *                           (X27.589 ~315°); set per gauge (e.g. 960 for a 320° BKA-30).
 * @param homeStepUs         homing seek rate µs/step. Default 0 → library default (2000 ≈ 500 steps/s).
 *                           Keep under the motor start-stop rate (~774 steps/s) or the seek slips.
 * @return Populated StepperConfig.
 */
StepperConfig makeX27Config(int16_t homePosition, int16_t parkPosition,
                            int16_t minPos, int16_t maxPos,
                            HomeMode home = HomeMode::STALL,
                            bool homeSeekClockwise = false,
                            HomeSensor sensor = { true, 5, 2000 },
                            bool wrap = false, uint8_t deadband = 1,
                            bool autoRecal = false, uint32_t recalDebounceMs = 0,
                            uint16_t stepsPerRev = 1080, uint16_t rangeSteps = 945,
                            uint16_t homeStepUs = 0);

/**
 * @brief Non-blocking instrument-gauge stepper driven through PinRef coils.
 */
class StepperMotor : public MotorDriver {
public:
    /**
     * @brief Construct a stepper over four coil pins.
     * @param c1..c4     Coil PinRefs (GPIO or MCP23017). Swap two to reverse direction.
     * @param cfg        Per-instance configuration (copied; cfg.accel must outlive this).
     * @param homeSense  Home-sensor PinRef for HomeMode::SENSOR (NC default for STALL).
     * @param sleepEn    Optional driver ~SLEEP/enable PinRef, driven HIGH in configure().
     */
    StepperMotor(PinRef c1, PinRef c2, PinRef c3, PinRef c4, const StepperConfig& cfg,
                 PinRef homeSense = PinRef(), PinRef sleepEn = PinRef());

    void    configure() override;            ///< coils OUTPUT, ~SLEEP HIGH, sensor INPUT, energise.
    void    home() override;                 ///< blocking homing (STALL or SENSOR), then park.
    void    moveTo(int32_t pos) override;    ///< retarget (clamp/wrap + deadband); non-blocking.
    void    update() override;               ///< step toward target if due; auto-recal.
    int32_t position() const override;       ///< current step (wrapped to 0..stepsPerRev if wrap).

    /** @brief True once homing has completed successfully (false if a SENSOR seek aborted). */
    bool homed() const { return _homed; }

#ifdef STEPPERMOTOR_TEST
    void    debugAdvance()            { advance(); }             ///< force one engine step (no micros gate)
    int32_t debugCurrentStep() const  { return _currentStep; }
    int32_t debugTargetStep() const   { return _targetStep; }
    uint16_t debugVel() const         { return _vel; }
    uint16_t debugMicroDelay() const  { return _microDelay; }
    bool    debugStopped() const      { return _stopped; }
    bool    debugSensorAsserted() const { return sensorAsserted(false); }
    void    debugSetSensorOverride(int8_t level) { _sensorOverride = level; } ///< -1 pin, 0/1 forced
#endif

private:
    // collaborators / config (plain // — EXTRACT_PRIVATE NO, not in API docs)
    PinRef       _coil[4];        // coil pins
    PinRef       _homeSense;      // home sensor (NC if unused)
    PinRef       _sleepEn;        // driver enable / ~SLEEP (NC if unused)
    StepperConfig _cfg;           // copied config (accel table referenced, not owned)
    uint16_t     _maxVel;         // last accel threshold = top speed gate

    // motion state (SwitecX25 model)
    int32_t      _currentStep;    // absolute step position
    int32_t      _targetStep;     // commanded target
    uint16_t     _vel;            // accel-steps proxy for velocity
    int8_t       _dir;            // +1 / -1 / 0
    bool         _stopped;        // true when settled at target
    uint8_t      _state;          // coil-state index (0..stateCount-1)
    uint16_t     _microDelay;     // delay until next step, µs
    uint32_t     _time0;          // micros() at last step
    bool         _homed;          // homing succeeded

    // auto-recal
    uint32_t     _lastRecalMs;    // millis() of last recal
    int8_t       _sensorOverride; // -1 = read pin; 0/1 = forced (test seam)

    // helpers
    void     writeIO();                    // energise coils for _state
    void     stepOnce(bool up);            // one detent in a direction
    void     advance();                    // SwitecX25 accel/step kernel
    bool     sensorAsserted(bool live) const; // single read through activeLow; live=true → bypass cache
    bool     sensorConfirmed() const;      // sensorAsserted stable for debounceMs
    bool     seekHomeBlocking();           // step toward sensor until confirmed or maxSeekSteps
    void     runToStopBlocking();          // advance() + delay until stopped (homing/park moves)
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
