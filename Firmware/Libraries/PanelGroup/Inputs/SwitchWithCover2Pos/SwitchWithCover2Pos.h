/**
 * @file SwitchWithCover2Pos.h
 * @brief Guarded 2-position switch — one pin drives the sim's cover and the switch under it.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Inputs/Switch2Pos/Switch2Pos.h>  // family base (InputBase, PinRef via PanelGroup.h)

namespace OpenSkyhawk {

/**
 * @brief Guarded switch — the DcsBios::SwitchWithCover2Pos equivalent. One physical switch pin
 *        drives two sim controls: the guard cover and the switch it protects.
 *
 * @details The cover is **not sensed**. There is one switch on the panel, and this class sequences
 * the pair so the sim's cover animates and any sim logic gated on the cover is satisfied:
 *
 * | Physical switch | Frames, each COVER_DELAY_MS apart | End state |
 * |---|---|---|
 * | flips **on**  | cover `1` (open) → switch `1` | cover open, switch on |
 * | flips **off** | switch `0` → cover `0` (close) | cover closed, switch off |
 *
 * States mirror DCS-BIOS — OFF_CLOSED → OFF_OPEN → ON_OPEN and back — and the 20 ms debounce comes
 * from Switch2Pos. The sequencer steps from poll(), one frame at a time, and never calls delay():
 * a node blocking for 200 ms would stall every other control and the CAN drain with it. Flipping
 * back mid-sequence simply retargets the machine, so it walks back the way it came.
 *
 * On boot and SYNC_REQ the settled pair is re-asserted in the same order (cover first when on,
 * switch first when off). That is deliberately more than DCS-BIOS does — its reset re-sends
 * nothing — because FirmwarePlan D5 requires every absolute input to be re-sent on sync: a missed
 * input otherwise leaves DCS wrong until the control is next touched.
 *
 * Does **not** read a cover microswitch: a builder with a sensed cover wires it as its own
 * Switch2Pos on the cover's control. Does **not** cover 3-position guarded switches — the DCS-BIOS
 * class is 2-position only, and the A-4E-C's one cover guards a 3-position switch the mod does not
 * gate, so the A-4 build uses Switch3Pos there. This class is for DCS-BIOS parity and other
 * aircraft.
 */
class SwitchWithCover2Pos : public Switch2Pos {
public:
    /** @brief Minimum gap between the cover frame and the switch frame, ms. */
    static constexpr uint16_t COVER_DELAY_MS = 200;

    /**
     * @brief Construct a guarded 2-position switch.
     *
     * @param switchId  DCSIN_* constant for the switch under the cover.
     * @param coverId   DCSIN_* constant for the cover itself (the generator already emits covers
     *                  as their own control, e.g. AFCS_1N2_COVER).
     * @param pin       PinRef for the one physical switch (GPIO or MCP23017).
     * @param reverse   false (default): active-LOW, as Switch2Pos. true: active-HIGH.
     */
    SwitchWithCover2Pos(uint16_t switchId, uint16_t coverId, PinRef pin, bool reverse = false);

    /** @brief Debounce through Switch2Pos, then advance the cover/switch sequence. */
    void poll() override;

#ifdef SWITCHWITHCOVER_TEST
    /** @brief Test seam — 0 = OFF_CLOSED, 1 = OFF_OPEN, 2 = ON_OPEN. */
    uint8_t state() const { return (uint8_t)_state; }
    /** @brief Test seam — the state the sequencer is walking toward. */
    uint8_t target() const { return (uint8_t)_target; }
#endif

protected:
    /**
     * @brief Set the sequence target instead of sending. Called by Switch2Pos on a confirmed
     *        change, and with init = true for the boot / SYNC_REQ baseline.
     *
     * On init the machine is seeded at the opposite end of the travel so the sequencer re-asserts
     * both frames in the correct order rather than only the one that changed.
     */
    void emit(bool active, bool init) override;

private:
    /** @brief Sequence positions, in travel order. */
    enum class Cover : uint8_t { OFF_CLOSED = 0, OFF_OPEN = 1, ON_OPEN = 2 };

    void step();                    ///< advance one position toward _target when the delay elapsed
    void sendCover(bool open);
    void sendSwitch(bool on);

    uint16_t _coverId;
    Cover    _state;                ///< where the sim is believed to be
    Cover    _target;               ///< where the physical switch says it should be
    uint32_t _lastStepMs;           ///< millis() of the last frame sent by the sequencer
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
