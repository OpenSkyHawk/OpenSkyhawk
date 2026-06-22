/**
 * @file MultiPosInput.h
 * @brief Shared base for multi-position selector inputs (SwitchMultiPos, AnalogMultiPos, ...).
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase

namespace OpenSkyhawk {

/**
 * @brief Base for the MULTIPOS input family — selectors that emit an absolute position
 *        index 0..N-1 over CAN. Self-registers into PanelGroup's InputBase list.
 *
 * @details A subclass reports the instantaneous resolved position via readRaw(); this base
 * debounces it (configurable window) and emits a CAN EVT only when the *confirmed* position
 * changes. The emitted value is the absolute index, never a delta — a jump from any position
 * to any other (even skipping intermediates) emits the new index directly; there is no ±1 or
 * adjacency assumption.
 *
 * readRaw() returns NO_POSITION when nothing resolves (e.g. a non-shorting rotary mid-throw
 * with no pin closed); the base then holds the last confirmed position — no spurious EVT.
 *
 * Subclasses: SwitchMultiPos (one-hot pins), AnalogMultiPos (resistor ladder, #114).
 */
class MultiPosInput : public InputBase {
public:
    /** @brief readRaw() sentinel: "nothing active right now — hold the last position". */
    static constexpr uint16_t NO_POSITION = 0xFFFF;

    /**
     * @brief Resolve the position, debounce it, emit a CAN EVT on confirmed change.
     *
     * Called by PanelGroup::loop(). No-op until forceReport() has run once.
     */
    void poll() override;

    /**
     * @brief Resolve the current position and emit a CAN EVT unconditionally — no debounce.
     *
     * Called by PanelGroup during the boot EVT burst and on SYNC_REQ. Establishes the
     * baseline so subsequent poll() calls have a valid reference.
     */
    void forceReport() override;

protected:
    /**
     * @brief Construct the base.
     *
     * @param controlId     DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param numPositions  number of discrete positions N (valid indices 0..N-1).
     * @param debounceMs     stability window before a changed position is confirmed.
     *                       0 = confirm on the next poll (subclass does its own filtering).
     */
    MultiPosInput(uint16_t controlId, uint8_t numPositions, uint16_t debounceMs);

    /**
     * @brief Resolve the instantaneous position index, or NO_POSITION to hold the last.
     *
     * Implemented per subclass (one-hot pin scan, analog band-resolve, ...). Must be
     * non-blocking. Return a value in 0..numPositions-1, or NO_POSITION when nothing is
     * currently active.
     *
     * @return resolved index, or NO_POSITION.
     */
    virtual uint16_t readRaw() = 0;

    uint16_t _controlId;     ///< DCS/HID control id (routing).
    uint8_t  _numPositions;  ///< N — number of discrete positions.

private:
    void emit(uint16_t pos, bool init = false);  ///< sendBatched a MULTIPOS EVT {controlId, pos}.

    uint16_t _debounceMs;       // stability window (ms); 0 = confirm next poll
    uint16_t _lastPos;          // last confirmed / emitted index
    uint16_t _pendingPos;       // last raw reading (pre-confirm)
    uint32_t _debounceStartMs;  // millis() when _pendingPos last changed
    bool     _initialized;      // false until forceReport(); poll() no-op before this
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
