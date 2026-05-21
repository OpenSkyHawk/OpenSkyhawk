/**
 * @file PanelGroup.h
 * @brief CAN sub-node domain layer for OpenSkyhawk panel boards.
 *
 * @details Provides the PanelGroup singleton namespace and the OpenSkyhawk output
 * and input object classes that panel sketches declare at global scope — mirroring
 * the DCS-BIOS design pattern.
 *
 * A production panel sketch looks like this:
 *
 * @code
 * #include <PanelGroup.h>
 *
 * OpenSkyhawk::LED     armLed(A_4E_C_ARM_MASTER, 0x4000, PB0);
 * OpenSkyhawk::Switch2Pos ejSafe(A_4E_C_SEAT_EJECT_SAFE, PA1);
 *
 * void setup() { STM32Board::setDebug(true); PanelGroup::setup(); }
 * void loop()  { PanelGroup::loop(); }
 * @endcode
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <CANProtocol.h>

// ── Output objects — DCS state → panel hardware ───────────────────────────────

/**
 * @brief Output and input classes for OpenSkyhawk panel boards.
 *
 * @details Objects are declared at global scope in a sketch; constructors
 * self-register into static linked lists. PanelGroup::loop() dispatches
 * incoming ControlPacket CAN frames to all registered OutputBase objects
 * and polls all registered InputBase objects every iteration.
 */
namespace OpenSkyhawk {

/**
 * @brief Base class for all DCS-driven output objects on a PanelGroup board.
 *
 * @details Subclass this to create custom output handlers. Objects declared at
 * global scope self-register into the static linked list; PanelGroup::loop()
 * walks the list on every received CTRL_BCAST frame.
 */
class OutputBase {
public:
    static OutputBase* first; ///< Head of the self-registration linked list.
    OutputBase* next;         ///< Next object in the list.

    /** @brief Registers this object at the head of the linked list. */
    OutputBase();

    /**
     * @brief Called by PanelGroup::loop() for every received ControlPacket.
     *
     * @details Implementations should check controlId against their own address
     * and update hardware only on a match.
     *
     * @param controlId controlId field from the received ControlPacket.
     * @param value     value field from the received ControlPacket.
     */
    virtual void onPacket(uint16_t controlId, uint16_t value) = 0;
};

/**
 * @brief Drive a GPIO pin from a single bit of a DCS-BIOS output value.
 *
 * @details Mirrors DcsBios::LED. Pin goes HIGH when (value & mask) is non-zero,
 * LOW otherwise.
 *
 * @code
 * // Direct STM32 GPIO:
 * OpenSkyhawk::LED warn(A_4E_C_MASTER_CAUTION, 0x4000, PB0);
 * @endcode
 */
class LED : public OutputBase {
    uint16_t addr_; ///< DCS-BIOS address (= controlId) this LED listens to.
    uint16_t mask_; ///< Bitmask to test within the received value.
    uint8_t  pin_;  ///< Output GPIO pin (active HIGH).
public:
    /**
     * @brief Construct an LED output object.
     * @param addr DCS-BIOS address for this indicator (used as controlId).
     * @param mask Bitmask to extract the relevant bit from the value word.
     * @param pin  GPIO pin to drive (active HIGH).
     */
    LED(uint16_t addr, uint16_t mask, uint8_t pin);

    /** @brief Sets pin HIGH when (value & mask) != 0, LOW otherwise. */
    void onPacket(uint16_t controlId, uint16_t value) override;
};

/**
 * @brief Call an arbitrary function with the raw value from a ControlPacket.
 *
 * @details Escape hatch for non-standard output logic — e.g. a value that maps
 * to motor positions, PWM brightness, or multi-bit state machines that do not
 * fit the LED pattern. Mirrors DcsBios::IntegerBuffer.
 *
 * @code
 * void onCanopyPos(uint16_t v) { analogWrite(CANOPY_MOTOR, v >> 8); }
 * OpenSkyhawk::IntegerOutput canopy(A_4E_C_CANOPY_POS, onCanopyPos);
 * @endcode
 */
class IntegerOutput : public OutputBase {
    uint16_t addr_;       ///< DCS-BIOS address (= controlId) to listen to.
    void (*cb_)(uint16_t); ///< User callback invoked on every matching packet.
public:
    /**
     * @brief Construct an IntegerOutput object.
     * @param addr DCS-BIOS address for this control (used as controlId).
     * @param cb   Function called with the raw value on every matching packet.
     */
    IntegerOutput(uint16_t addr, void (*cb)(uint16_t));

    /** @brief Calls cb_(value) if controlId matches addr_. */
    void onPacket(uint16_t controlId, uint16_t value) override;
};

// ── Input objects — panel hardware → DCS via CAN ─────────────────────────────

/**
 * @brief Base class for all hardware-polled input objects on a PanelGroup board.
 *
 * @details Subclass this to create custom input handlers. Objects self-register
 * at construction; PanelGroup::loop() calls poll() on every object each iteration.
 */
class InputBase {
public:
    static InputBase* first; ///< Head of the self-registration linked list.
    InputBase* next;         ///< Next object in the list.

    /** @brief Registers this object at the head of the linked list. */
    InputBase();

    /**
     * @brief Read hardware state and call PanelGroup::sendEvent() on change.
     *
     * @details Called by PanelGroup::loop() every iteration. Implementations
     * must be non-blocking and handle their own debounce.
     */
    virtual void poll() = 0;
};

/**
 * @brief Debounced 2-position GPIO switch — sends a ControlPacket CAN event on change.
 *
 * @details Mirrors DcsBios::Switch2Pos but sends a CAN packet via PanelGroup::sendEvent()
 * instead of writing to DCS-BIOS directly. The matching OpenSkyhawk::DCSInput on the
 * SimGateway translates the packet to a DCS-BIOS message.
 *
 * Value sent: 1 when pin is LOW (switch closed to GND), 0 when HIGH.
 *
 * @code
 * OpenSkyhawk::Switch2Pos ejSafe(A_4E_C_SEAT_EJECT_SAFE, PA1);
 * @endcode
 */
class Switch2Pos : public InputBase {
    uint16_t addr_;       ///< controlId sent in the CAN event packet.
    uint8_t  pin_;        ///< Input GPIO pin (INPUT_PULLUP).
    bool     lastStable_; ///< Last debounced stable state.
    bool     lastRaw_;    ///< Raw pin reading from previous poll().
    uint32_t debounceMs_; ///< Timestamp of last raw state change.
public:
    /**
     * @brief Construct a Switch2Pos input object.
     * @param addr controlId to send in the CAN event (typically the DCS-BIOS address).
     * @param pin  GPIO pin to read (configured INPUT_PULLUP by the constructor).
     */
    Switch2Pos(uint16_t addr, uint8_t pin);

    /**
     * @brief Read pin, debounce, and call PanelGroup::sendEvent() on state change.
     *
     * @details Debounce window is 20 ms. Sends value=1 when pin is LOW (active),
     * value=0 when HIGH.
     */
    void poll() override;
};

} // namespace OpenSkyhawk

// ── PanelGroup singleton ──────────────────────────────────────────────────────

/**
 * @brief Static singleton for CAN sub-node (PanelGroup) firmware.
 *
 * @details Reads node_id from the PA0 strap pin at boot, configures the CAN
 * receive filter for CTRL_BCAST and TEST_SEQ frames, dispatches incoming frames
 * to registered OpenSkyhawk output objects, polls registered input objects, and
 * sends heartbeat frames every 500 ms.
 *
 * Node ID strapping: tie PA0 to 3.3 V for node_id=1; leave floating (internal
 * pull-down) for node_id=2.
 */
namespace PanelGroup {

    /**
     * @brief Initialise hardware and start the CAN bus.
     *
     * @details Calls STM32Board::begin(), reads the PA0 strap pin to set node_id,
     * configures the CAN filter (CTRL_BCAST + TEST_SEQ), then calls canStart().
     * Call STM32Board::setDebug(true) before this to enable diagnostic output.
     */
    void setup();

    /**
     * @brief Service all PanelGroup activity. Call every Arduino loop() iteration.
     *
     * @details Calls STM32Board::update(), drains the CAN RX FIFO and dispatches
     * ControlPacket frames to registered OutputBase objects, polls all InputBase
     * objects, and sends a heartbeat CAN frame every 500 ms.
     */
    void loop();

    /**
     * @brief Send a hardware input event over CAN to the master node.
     *
     * @details Packs controlId and value into an 8-byte CAN frame on CAN_ID_EVT_n
     * (where n = nodeId()). Intended to be called by InputBase subclasses.
     *
     * @param controlId Identifies the physical control (HID range 0x0010–0x00FF,
     *                  or DCS-BIOS address 0x8000+).
     * @param value     Current control value (0/1 for switches, 0–65535 for axes).
     * @returns true on successful TX, false if the CAN mailbox was full.
     */
    bool sendEvent(uint16_t controlId, uint16_t value);

    /**
     * @brief Return the node ID read from the PA0 strap pin at boot.
     * @returns 1 if PA0 was HIGH, 2 if PA0 was LOW.
     */
    uint8_t nodeId();

} // namespace PanelGroup

#endif // ARDUINO_ARCH_STM32
