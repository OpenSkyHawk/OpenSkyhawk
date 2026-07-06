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
 * #include <Outputs/LED/LED.h>
 * #include <A4EC_OutputIds.h>
 *
 * MCP23017 exp1(0x20, Wire);
 * ADS1115  adc1;
 *
 * const PinRef PIN_MASTER_ARM  = PinRef(exp1, PORT_A, 3);
 * const PinRef PIN_CAUTION_LED = PinRef(PB0);
 *
 * OpenSkyhawk::LED masterCaution(A_4E_C_MASTER_CAUTION, A_4E_C_MASTER_CAUTION_AM,
 *                                PIN_CAUTION_LED);
 *
 * void setup() {
 *     Wire.begin();
 *     PanelGroup::registerExpander(exp1, PB12, PB13);  // INTA→PB12, INTB→PB13
 *     // (PB3–PB5/PB8/PB9 belong to ShiftBus1 on a node that also uses shift registers)
 *     PanelGroup::registerADC(adc1, 0x48, Wire);
 *     PanelGroup::setup();
 * }
 * void loop() { PanelGroup::loop(); }
 * @endcode
 *
 * @version 0.3.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <Wire.h>
#include <MCP23017.h>
#include "ADS1115.h"
#include "PinRef.h"
#include "Helpers/ShiftBus/ShiftBus.h"   // ShiftBus + the pre-defined ShiftBus1 instance
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

    /**
     * @brief High-rate sample hook — called from a sampling ISR when the node runs one
     *        (e.g. ShiftBus timer sampling, -DSHIFTBUS_ISR_HZ). Default no-op.
     *
     * Implementations must be ISR-safe: read cached pin state only, no CAN, no I2C,
     * no allocation. Level-sampled inputs never need this; RotaryEncoder overrides it
     * to decode quadrature at the sample rate (loop stalls then cannot lose transitions).
     * The input class does not know who calls this or at what rate — PanelGroup owns
     * the wiring.
     */
    virtual void sampleTick() {}

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

    /**
     * @brief Node-health fault code for this output (#163). 0 = healthy.
     *
     * Rolled up across all outputs into the HEALTH_n frame's faultId. Must be cheap and
     * const — read cached state only (e.g. an I2cHealth breaker flag), never an I2C
     * transaction — it is called from the periodic health path. Default: never faults.
     * @return A NodeFaultId value (see CANProtocol.h), or 0 if healthy.
     */
    virtual uint8_t faultCode() const { return 0; }

    /**
     * @brief Human-readable fault detail for the local DiagSerial tap only (#163).
     *
     * Never transmitted on the wire — the CAN frame carries only the coarse faultCode().
     * @return A short static string naming the failing peripheral, or "" if healthy.
     */
    virtual const char* faultDetail() const { return ""; }

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
     * Adafruit_ADS1115 takes address and bus via begin(), not the constructor.
     * Pattern:
     * @code
     * ADS1115 adc;
     * PanelGroup::registerADC(adc, 0x48, Wire);   // 0x48–0x4B via ADDR pin
     * @endcode
     *
     * @param adc   ADS1115 instance. Must outlive PanelGroup.
     * @param addr  I2C address (0x48–0x4B via ADDR pin). Default 0x48.
     * @param wire  I2C bus. Default Wire (I2C1 on STM32).
     * @note Wire.begin() (or Wire1.begin()) must be called by the sketch before setup().
     */
    void registerADC(ADS1115& adc, uint8_t addr = 0x48, TwoWire& wire = Wire);

    /**
     * @brief Register an MCP23017 expander in interrupt-driven mode. Call before setup().
     *
     * PanelGroup calls chip.init(), configures IOCON (MIRROR and/or open-drain as
     * detected), enables interrupt-on-change on input pins only (IODIR-masked, bit 7
     * excluded per GPA7/GPB7 silicon erratum), reads baseline port state, and attaches
     * STM32 ISRs.
     *
     * Pass the same STM32 pin for intaPin and intbPin to use MIRROR mode
     * (IOCON.MIRROR=1), where either port interrupt asserts the shared line.
     *
     * If two or more chips share an interrupt line (wired-OR), PanelGroup sets
     * IOCON.ODR=1 (open-drain) automatically on all chips on that line.
     *
     * @param chip     blemasle/MCP23017 instance. Must outlive PanelGroup.
     * @param intaPin  STM32 GPIO pin connected to chip INTA.
     * @param intbPin  STM32 GPIO pin connected to chip INTB. Same as intaPin for MIRROR.
     * @note Do not use PB14/PB15 (status LED) or PC13–PC15 (RTC/oscillator).
     */
    void registerExpander(MCP23017& chip, uint8_t intaPin, uint8_t intbPin);

    /**
     * @brief Register an MCP23017 expander in polling-fallback mode. Call before setup().
     *
     * PanelGroup reads all port registers every ~20 ms in loop(). No STM32 interrupt
     * pin is required. Use when interrupt lines are not wired or not needed.
     *
     * @param chip  blemasle/MCP23017 instance. Must outlive PanelGroup.
     */
    void registerExpander(MCP23017& chip);

    /**
     * @brief Initialise PanelGroup. Call from sketch setup() after Wire.begin().
     *
     * Performs the 8-step boot sequence:
     *   1. STM32Board::begin()
     *   2. For each registered ADC: begin(addr, wire). For each registered expander:
     *      init(), configure IOCON, enable interrupt-on-change on input pins,
     *      read baseline port state, attach STM32 ISR.
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
     *   2. Polling fallback (~20 ms): read ports for chips registered without interrupt.
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

    /**
     * @brief Deferred MCP23017 write — update the cache + mark the port dirty, no I2C.
     * @note Pair with flushExpanderWrites(). Lets a multi-pin output (e.g. a stepper's four
     *       coils) collapse N per-pin read-modify-writes into one writePort() per port.
     */
    void writeCachedPinDeferred(MCP23017& chip, uint8_t port, uint8_t bit, bool value);

    /**
     * @brief Push every port dirtied by writeCachedPinDeferred() — one writePort() each.
     * @note No-op when nothing is pending (GPIO-only outputs never dirty a port).
     */
    void flushExpanderWrites();

    /**
     * @brief Live MCP23017 pin read — fresh readPort() over I2C, refreshing the cache.
     * @param chip  Chip reference.
     * @param port  PORT_A (0) or PORT_B (1).
     * @param bit   Bit index 0–7.
     * @return      Live logical level (true = HIGH).
     * @note Called by PinRef::readLive() for time-critical reads before loop() refreshes the
     *       cache (e.g. blocking homing on an MCP-backed home sensor). One I2C transaction per call.
     */
    bool readLivePin(MCP23017& chip, uint8_t port, uint8_t bit);

    /**
     * @brief Collect a ShiftBus at configure time. Called by PinRef::configureAsInput() /
     *        configureAsOutput() on SR pins — never by sketches (zero-setup lifecycle).
     *
     * Deduplicated. setup() begin()s every collected bus after step 3; loop() transfers
     * them each iteration; flushExpanderWrites() flushes dirty ones.
     */
    void noteShiftBus(OpenSkyhawk::ShiftBus& bus);

} // namespace PanelGroup

#endif // ARDUINO_ARCH_STM32
