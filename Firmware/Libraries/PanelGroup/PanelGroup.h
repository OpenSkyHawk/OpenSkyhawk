/**
 * @file PanelGroup.h
 * @brief CAN sub-node domain layer for OpenSkyhawk panel boards.
 *
 * @details Provides the PanelGroup namespace (expander registration, setup, loop, MCP cache
 * bridge) and the OpenSkyhawk base classes that input and output objects inherit from.
 * All input/output objects are declared at global scope in a sketch so their constructors
 * self-register before setup() runs.
 *
 * Sketch pattern:
 * @code
 * #include <Wire.h>
 * #include <MCP23017.h>
 * #include <PanelGroup.h>
 *
 * MCP23017 exp1(0x20, Wire);
 *
 * const PinRef PIN_MASTER_ARM  = PinRef(exp1, PORT_A, 3);
 * const PinRef PIN_CAUTION_LED = PinRef(PB0);
 *
 * OpenSkyhawk::Switch2Pos  masterArm(DCSIN_ARM_MASTER,          PIN_MASTER_ARM);
 * OpenSkyhawk::LED         masterCaution(A_4E_C_MASTER_CAUTION, 0x4000, PIN_CAUTION_LED);
 *
 * void setup() {
 *     Wire.begin();
 *     PanelGroup::registerExpander(exp1, PB3, PB4);
 *     PanelGroup::setup();
 * }
 * void loop() { PanelGroup::loop(); }
 * @endcode
 *
 * @version 0.2.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <Wire.h>
#include <MCP23017.h>
#include "ADS1115.h"
#include "PinRef.h"
#include <CANProtocol.h>

// ── OpenSkyhawk base classes ──────────────────────────────────────────────────

namespace OpenSkyhawk {

/**
 * @brief Abstract base for all hardware-polled input objects.
 *
 * @details Declare concrete input objects at global scope; the constructor
 * self-registers into a static linked list. PanelGroup::setup() calls configure()
 * on every registered input after chip initialisation, then calls forceReport()
 * for the boot EVT burst. PanelGroup::loop() calls poll() every iteration.
 */
class InputBase {
public:
    /**
     * @brief Configure hardware pins for this input.
     *
     * Called by PanelGroup::setup() after chip.init() completes. Call
     * _pin.configureAsInput() here — not in the constructor — because MCP23017
     * register writes require chip.init() to have run first.
     *
     * Default is a no-op. Override in every input class that owns a PinRef.
     */
    virtual void configure() {}

    /**
     * @brief Read hardware state and emit a CAN EVT if state changed.
     *
     * Called by PanelGroup::loop() every iteration. Must be non-blocking.
     * Implementations apply their own debounce / filtering and call
     * CANProtocol::sendBatched() only on confirmed state change.
     */
    virtual void poll() = 0;

    /**
     * @brief Read hardware state and emit a CAN EVT unconditionally.
     *
     * Called during the boot EVT burst and on every SYNC_REQ. Bypasses debounce.
     * Establishes the current reading as the new baseline so subsequent poll()
     * calls have a valid comparison point.
     */
    virtual void forceReport() = 0;

    /** @brief Head of the self-registered linked list. */
    static InputBase* head();

    /** @brief Next input in the list; nullptr at end. */
    InputBase* next() const;

protected:
    InputBase();  ///< Registers this instance into the linked list.

private:
    static InputBase* _head;
    InputBase* _next;
};

/**
 * @brief Abstract base for all DCS-driven output objects.
 *
 * @details Declare concrete output objects at global scope; the constructor
 * self-registers into a static linked list. PanelGroup::setup() calls configure()
 * after chip initialisation. PanelGroup::loop() calls onControlPacket() for every
 * non-null slot in each received CTRL_BCAST frame and calls update() every iteration.
 */
class OutputBase {
public:
    /**
     * @brief Configure hardware pins for this output.
     *
     * Called by PanelGroup::setup() after chip.init() completes. Call
     * _pin.configureAsOutput() here. Default is a no-op.
     */
    virtual void configure() {}

    /**
     * @brief Called for every non-null ControlPacket in a CTRL_BCAST frame.
     *
     * @param controlId  DCS-BIOS output address from the received packet.
     * @param value      16-bit value from DCS-BIOS.
     */
    virtual void onControlPacket(uint16_t controlId, uint16_t value) = 0;

    /**
     * @brief Called every PanelGroup::loop() iteration.
     *
     * Default is a no-op. Override for outputs that need continuous work
     * between CAN frames (stepper motor step generation, PWM updates).
     */
    virtual void update() {}

    /** @brief Head of the self-registered linked list. */
    static OutputBase* head();

    /** @brief Next output in the list; nullptr at end. */
    OutputBase* next() const;

protected:
    OutputBase();  ///< Registers this instance into the linked list.

private:
    static OutputBase* _head;
    OutputBase* _next;
};

// ── Concrete output classes ───────────────────────────────────────────────────

/**
 * @brief Drive a pin HIGH/LOW from a bitmask of a DCS-BIOS output value.
 *
 * @details Pin goes HIGH when `(value & mask) != 0`, LOW otherwise.
 * Works on GPIO, MCP23017, and ADS1115 PinRefs (ADS write is a no-op).
 *
 * @code
 * OpenSkyhawk::LED masterCaution(A_4E_C_MASTER_CAUTION, 0x4000, PinRef(PB0));
 * @endcode
 */
class LED : public OutputBase {
public:
    /**
     * @brief Construct an LED output.
     * @param addr       DCS-BIOS controlId this LED responds to.
     * @param mask       Bitmask applied to the received value.
     * @param pin        PinRef for the LED pin (GPIO or MCP23017 output bit).
     * @param activeHigh true (default) — pin HIGH when LED on (current-source).
     *                   false — pin LOW when LED on (current-sink / active-low).
     */
    LED(uint16_t addr, uint16_t mask, PinRef pin, bool activeHigh = true);

    void configure() override;
    void onControlPacket(uint16_t controlId, uint16_t value) override;

private:
    uint16_t _addr;
    uint16_t _mask;
    PinRef   _pin;
    bool     _activeHigh;
};

/**
 * @brief Forward a raw DCS-BIOS value to a user-supplied callback.
 *
 * @details Escape hatch for non-standard output logic (motor positions,
 * PWM brightness, multi-bit state machines). No pin ownership.
 *
 * @code
 * void onCanopyPos(uint16_t v) { analogWrite(MOTOR_PWM, v >> 8); }
 * OpenSkyhawk::IntegerOutput canopy(A_4E_C_CANOPY_POS, onCanopyPos);
 * @endcode
 */
class IntegerOutput : public OutputBase {
public:
    /**
     * @brief Construct an IntegerOutput.
     * @param addr  DCS-BIOS controlId to listen to.
     * @param cb    Callback invoked with the raw 16-bit value on each match.
     */
    IntegerOutput(uint16_t addr, void (*cb)(uint16_t));

    void onControlPacket(uint16_t controlId, uint16_t value) override;

private:
    uint16_t _addr;
    void (*_cb)(uint16_t);
};

} // namespace OpenSkyhawk

// ── PanelGroup namespace — sketch-facing API ──────────────────────────────────

/**
 * @brief Static singleton for CAN sub-node (PanelGroup) firmware.
 *
 * @details Manages MCP23017 expander registration and cache, ADS1115 registration,
 * the 8-step boot sequence, per-loop interrupt dispatch and polling fallback,
 * CAN EVT batching, CTRL_BCAST dispatch, SYNC_REQ response, and the 500 ms
 * HB_n heartbeat.
 */
namespace PanelGroup {

    /**
     * @brief Register an ADS1115 ADC. Call before setup().
     *
     * PanelGroup calls adc.begin(addr, wire) during setup(). Register each ADS1115
     * instance exactly once — multiple AnalogInput objects may share the same chip.
     *
     * @param adc   ADS1115 instance. Must outlive PanelGroup.
     * @param addr  I2C address (0x48–0x4B via ADDR pin). Default 0x48.
     * @param wire  I2C bus. Default Wire (I2C1 on STM32).
     * @note Wire.begin() (or Wire1.begin()) must be called by the sketch before setup().
     */
    void registerADC(ADS1115& adc, uint8_t addr = 0x48, TwoWire& wire = Wire);

    /**
     * @brief Register an MCP23017 expander. Call before setup().
     *
     * PanelGroup stores the reference and interrupt pin assignments. During setup(),
     * PanelGroup calls chip.init(), configures IOCON, reads baseline port state,
     * and attaches STM32 interrupt handlers.
     *
     * Pass the same pin for intaPin and intbPin to use MIRROR mode (IOCON.MIRROR=1),
     * which causes either port interrupt to assert the shared line.
     *
     * Pass NO_INT_PIN (0xFF) for both pins to enable polling-fallback mode (~20 ms).
     *
     * If two or more chips share the same interrupt line (wired-OR), PanelGroup
     * sets IOCON.ODR=1 (open-drain) automatically on all chips on that line.
     *
     * @param chip     blemasle/MCP23017 instance. Must outlive PanelGroup.
     * @param intaPin  STM32 GPIO pin connected to chip INTA. Use NO_INT_PIN for polling.
     * @param intbPin  STM32 GPIO pin connected to chip INTB. Same as intaPin for MIRROR.
     *
     * @note Do not use PB14/PB15 (status LED) or PC13–PC15 (RTC/oscillator).
     */
    void registerExpander(MCP23017& chip, uint8_t intaPin, uint8_t intbPin);

    /** @brief Sentinel: pass as intaPin and intbPin for polling-fallback mode. */
    static constexpr uint8_t NO_INT_PIN = 0xFF;

    /**
     * @brief Initialise PanelGroup. Call from sketch setup() after Wire.begin().
     *
     * Performs the 8-step boot sequence:
     *   1. STM32Board::begin()
     *   2. For each registered ADC: begin(). For each registered expander: init(),
     *      configure IOCON, enable interrupt-on-change, read baseline port state,
     *      attach STM32 ISR.
     *   3. configure() on all InputBase and OutputBase objects.
     *   4. Register CAN callbacks.
     *   5. CANProtocol::start().
     *   6. forceReport() burst — one EVT per registered input.
     *   7. flushBatched(canIdEvt(NODE_ID)).
     *   8. READY_n frame (canIdReady(NODE_ID), DLC=0). Arm heartbeat timer.
     *
     * @note Wire.begin() must be called by the sketch before this function.
     *       PanelGroup never calls Wire.begin() — only start buses in use.
     */
    void setup();

    /**
     * @brief Run all PanelGroup work. Call once per loop() iteration.
     *
     * Each call:
     *   1. Check interrupt flags; read INTCAP; update expander port caches.
     *   2. Polling fallback (~20 ms): read ports for chips with no interrupt pin.
     *   3. poll() on all InputBase objects.
     *   4. CANProtocol::drain() — dispatches CTRL_BCAST, SYNC_REQ, TEST_SEQ echo.
     *   5. update() on all OutputBase objects.
     *   6. Heartbeat: send HB_n every 500 ms.
     */
    void loop();

    // ── Package-internal — MCP cache bridge for PinRef ───────────────────────
    //
    // Not sketch API. Called exclusively by PinRef::read() and PinRef::write().
    // Direct GPIO and ADS1115 PinRefs bypass this path entirely.
    //
    // MCP23017 reads go through the cache (no live I2C per poll) because:
    //   • A full readPort() at 400 kHz takes ~100 µs — too slow for every poll().
    //   • INTCAP captures port state at interrupt time; a subsequent readPort()
    //     may return an already-transitioned value, losing the captured snapshot.
    //   • All poll() calls after an interrupt must see the same captured snapshot.

    /**
     * @brief Return cached MCP23017 pin state. Called by PinRef::read(). No I2C.
     * @param chip  Chip reference — used as key to locate the ExpanderEntry.
     * @param port  PORT_A (0) or PORT_B (1).
     * @param bit   Bit index 0–7.
     * @return      Cached logical level (true = HIGH).
     */
    bool readCachedPin(const MCP23017& chip, uint8_t port, uint8_t bit);

    /**
     * @brief Write MCP23017 pin and update cache. Called by PinRef::write().
     * @param chip   Chip reference.
     * @param port   PORT_A (0) or PORT_B (1).
     * @param bit    Bit index 0–7.
     * @param value  Logical level to write.
     */
    void writeCachedPin(MCP23017& chip, uint8_t port, uint8_t bit, bool value);

} // namespace PanelGroup

#endif // ARDUINO_ARCH_STM32
