/**
 * @file AngleSensorInput.h
 * @brief Magnetic angle sensor as an absolute knob / axis, for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Inputs/AnalogInput/AnalogInput.h>  // family base (InputBase, PinRef via PanelGroup.h)

namespace OpenSkyhawk {

/**
 * @brief Magnetic angle sensor (AS5600 / MT6701) as an absolute knob or axis.
 *
 * @details An `AnalogInput` that reads the sensor's **analog output** through any analog-capable
 * PinRef — an STM32 ADC pin or an ADS1115 channel. No wiper to wear out, and a full 360°
 * mechanical range where a pot stops around 270°. Intended for absolute DCS-BIOS knobs such as
 * GUNSIGHT_KNB and RADAR_RETICLE; flight-control axes use linear Hall sensors on a plain
 * AnalogInput.
 *
 * The base owns everything that makes an analog input behave: EWMA smoothing, hysteresis,
 * per-instance pollMs, and routing by controlId (DCSIN_* → absolute set_state via PanelBridge;
 * CTRL_* → HID via SimGateway). This class adds only the **angle meaning** — where the travel sits
 * on the circle, and what happens at the 0°/360° seam.
 *
 * A full turn is 65536 counts, so one degree is 65536/360 ≈ 182 counts. The constructor turns
 * centre and travel into the base's [minRaw, maxRaw] window, and readRaw() re-centres every
 * reading so the centre angle lands at mid-scale. Unsigned arithmetic wraps for free, which is
 * what makes a travel spanning 0°/360° (say 340°→40°) behave like any other — no rotating the
 * magnet mount to dodge the seam.
 *
 * **The seam does not disappear, it moves opposite the centre.** A single-turn absolute sensor
 * reads the same angle at both ends of a full turn, so the circle cannot map onto a line without
 * one break; re-centring puts that break as far from the knob's centre as possible. Travel is
 * therefore up to *just under* a full turn: at exactly 360° the two ends would be the same sensor
 * angle, and the output would jump full-scale there. A knob that genuinely rotates without end
 * needs different semantics (turn counting, or a relative mode) and is not this class.
 *
 * Resolution note: the analog output is coarser than the chip's I²C register and picks up ADC
 * noise — fine for a gunsight or reticle knob. The AS5600's ZPOS/MPOS can narrow its output span
 * to the knob's travel to win some of that back. Reading the angle register over I²C belongs in a
 * future PinRef backend, not here, so this class keeps taking a PinRef.
 */
class AngleSensorInput : public AnalogInput {
public:
    /** @brief Counts per degree: a full turn spans the 16-bit range (65536 / 360). */
    static constexpr float COUNTS_PER_DEG = 65536.0f / 360.0f;

    /**
     * @brief Construct an absolute angle input.
     *
     * @param controlId  DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pin        analog PinRef for the sensor's output (STM32 ADC pin or ADS1115 channel).
     * @param centerDeg  sensor angle at the knob's centre position. Wraps, so 370 == 10.
     * @param travelDeg  total mechanical travel, centred on centerDeg: the knob spans
     *                   centerDeg ± travelDeg/2. Clamped to (0, 360) — up to just under a full
     *                   turn, because the ends of a full turn are one sensor angle. The wrap
     *                   point sits opposite centerDeg, so the closer travel comes to 360° the
     *                   nearer that break sits to the ends of travel.
     * @param pollMs     min interval between ADC reads, ms (default DEFAULT_POLL_MS). Per
     *                   instance; an ADS1115 PinRef blocks ~8 ms per conversion regardless.
     */
    AngleSensorInput(uint16_t controlId, PinRef pin, float centerDeg, float travelDeg,
                     uint16_t pollMs = DEFAULT_POLL_MS);

protected:
    /** @brief The base's reading, re-centred so centerDeg sits at mid-scale (0°/360° wraps). */
    uint16_t readRaw() override;

private:
    uint16_t _centerRaw;   ///< centerDeg in counts — the offset subtracted from every reading
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
