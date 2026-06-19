/**
 * @file MotorDriver.h
 * @brief Abstract base for non-blocking motor/servo drivers.
 *
 * @details A MotorDriver moves a physical actuator (gauge stepper, geared stepper,
 * RC servo) toward a commanded position without blocking the loop. It owns the
 * low-level drive (coil energising / PWM), homing, and per-step timing; it knows
 * nothing about DCS-BIOS. High-level controls — NeedleGauge (value → angle), and
 * later a motorised DrumDisplay or trim indicator — *compose* a MotorDriver and
 * drive it, so no stepper/servo code is duplicated across controls.
 *
 * A MotorDriver is NOT an OutputBase: it is owned and ticked (update()) by whatever
 * control uses it, not registered on PanelGroup's output list directly.
 *
 * Concrete drivers: StepperMotor (now); ServoMotor / StepDirMotor (future siblings).
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>

namespace OpenSkyhawk {

/**
 * @brief Common interface every motor/servo backend implements.
 *
 * @details Positions are in driver-native units (steps for a stepper, microseconds
 * for a servo). The owning control maps DCS-BIOS values into that space.
 */
class MotorDriver {
public:
    virtual ~MotorDriver() = default;

    /**
     * @brief Configure pins / drive hardware. Call once from the owner's configure().
     * @note Runs after bus/board init (PanelGroup::setup()), never from a constructor.
     */
    virtual void configure() = 0;

    /**
     * @brief Establish the zero reference (mechanical stop, home sensor, or absolute read).
     * @note May block (boot-time homing). Call once, after configure().
     */
    virtual void home() = 0;

    /**
     * @brief Set the target position in driver-native units. Non-blocking.
     * @param pos Target position; the driver clamps / wraps to its own limits.
     */
    virtual void moveTo(int32_t pos) = 0;

    /**
     * @brief Advance one step/increment toward the target if due. Call every loop().
     * @note Non-blocking — does at most the work for the current instant.
     */
    virtual void update() = 0;

    /** @brief Current position in driver-native units. */
    virtual int32_t position() const = 0;
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
