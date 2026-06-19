/**
 * @file NeedleGauge.h
 * @brief Pointer-gauge output: maps one DCS-BIOS value to a motor position.
 *
 * @details A thin OutputBase that *composes* a MotorDriver (StepperMotor today; a servo
 * or step/dir driver later) and does only the gauge semantics — decode the 16-bit
 * DCS-BIOS value, map it to a driver-native position (linear or piecewise-calibrated),
 * and command the motor. All low-level drive, acceleration, and homing live in the
 * MotorDriver, so 119 A-4E pointer gauges share one class over any backend.
 *
 * Usage (per-sketch wiring):
 * @code
 * StepperMotor driftMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), DRIFT_CFG);
 * NeedleGauge  drift(A_4E_C_APN153_DRIFT_GAUGE, A_4E_C_APN153_DRIFT_GAUGE_AM,
 *                    driftMotor, DRIFT_CAL);
 * @endcode
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                       // OutputBase
#include <Drivers/MotorDriver/MotorDriver.h>  // MotorDriver

namespace OpenSkyhawk {

/**
 * @brief Value → position calibration for one gauge.
 *
 * @details The DCS-BIOS value (0..65535) maps to a motor position. For a linear gauge,
 * 0 → @c minTravel and 65535 → @c maxTravel (either may exceed the other or be negative —
 * a centre-zero gauge sits mid-range). For a non-linear dial (airspeed, VVI), supply a
 * piecewise curve: @c curveIn holds ascending DCS breakpoints and @c curveOut the matching
 * positions (@c curveN entries); intermediate values are linearly interpolated.
 *
 * @note @c curveOut is unsigned — non-linear dials use a positive position range. Centre-zero
 * gauges use the linear path (signed @c minTravel / @c maxTravel) instead.
 */
struct GaugeCal {
    int16_t         minTravel;  ///< motor position at DCS value 0   (linear path)
    int16_t         maxTravel;  ///< motor position at DCS value 65535 (linear path)
    bool            reverse;    ///< flip direction (mounted/wired reversed)
    const uint16_t* curveIn;    ///< ascending DCS breakpoints, or nullptr for linear
    const uint16_t* curveOut;   ///< matching positions for curveIn
    uint8_t         curveN;     ///< breakpoint count (0 = linear)
};

/**
 * @brief DCS-driven pointer gauge over any MotorDriver backend.
 */
class NeedleGauge : public OutputBase {
public:
    /**
     * @brief Construct and register a pointer gauge.
     * @param controlId  DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h).
     * @param mask       Field mask (A_4E_C_*_AM, or 0xFFFF for a whole-word gauge).
     * @param motor      Caller-owned MotorDriver (e.g. a StepperMotor). Must outlive this.
     * @param cal        Value→position calibration. Must outlive this.
     */
    NeedleGauge(uint16_t controlId, uint16_t mask, MotorDriver& motor, const GaugeCal& cal);

    /** @brief Configure and home the motor (PanelGroup::setup()). */
    void configure() override;

    /**
     * @brief Retarget the motor from a CTRL_BCAST packet. Stores only — never steps here.
     * @param controlId  Incoming packet controlId. Ignored if != _controlId.
     * @param value      Raw 16-bit DCS-BIOS value.
     */
    void onControlPacket(uint16_t controlId, uint16_t value) override;

    /** @brief Advance the motor toward its target (non-blocking). */
    void update() override;

#ifdef NEEDLEGAUGE_TEST
    int32_t debugValueToPos(uint16_t value) const { return valueToPos(value); } ///< test: mapping only
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
