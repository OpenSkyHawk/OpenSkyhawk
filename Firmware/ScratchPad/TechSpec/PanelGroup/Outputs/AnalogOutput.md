# AnalogOutput — Technical Specification

**Status:** Ready for implementation
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#analogoutput-new`, `FirmwarePlan/10-implementation-plan.md` (Phase 5)
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

PWM analog output. Maps a 16-bit DCS-BIOS value to a PWM duty cycle on a **direct STM32
GPIO pin**. Primary use: backlight zone dimming — the IRLML2502 low-side MOSFET gate of an
LED zone (see `docs/_source/hardware-standards.md` → *LED Zone Switch*), three independent
zones per MCU board, each with its own `AnalogOutput`. Also usable for any other
PWM-proportional load (floodlights, dimmable indicators). Self-registers into PanelGroup's
OutputBase list.

Does **not** support MCP23017 or ADS1115 PinRefs — MCP23017 cannot do PWM; rejected at
`configure()` time with a debug assertion (mirrors LED's ADS1115 rejection). Does **not**
do on/off threshold logic — that is `LED`'s responsibility. Does **not** invert polarity —
the zone-switch hardware standard is fixed (higher duty = brighter); a lamp-test or
inverted-drive need would be a `scale` function, not a constructor flag. It **does** keep
the last written duty (`_lastDuty` / `_hasState`) and skips the `analogWrite()` when the
newly computed duty is unchanged — a redundant-write dedup, not value buffering.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Outputs/AnalogOutput/AnalogOutput.{h,cpp}
```

### Test project

```
Firmware/Tests/AnalogOutput/
├── platformio.ini
└── tests/
    ├── configure/
    │   └── configure.cpp        — configure() drives duty 0 (pin constant LOW, verified via
    │                              digitalRead); MCP23017 / ADS1115 PinRefs rejected with
    │                              debug assertion; GPIO PinRef accepted
    ├── mapping/
    │   └── mapping.cpp          — default map value >> 8: 0 → 0 (constant LOW), 65535 → 255
    │                              (constant HIGH), both verified via digitalRead; 32768 → 128
    │                              (~50% duty, verified by scope / measured brightness on bench)
    ├── scale_fn/
    │   └── scale_fn.cpp         — custom scale function overrides the default map; identity,
    │                              inversion, and gamma-style curve each applied correctly
    ├── controlid_filter/
    │   └── controlid_filter.cpp — non-matching controlId leaves duty unchanged; correct
    │                              controlId after a non-matching one still applies
    └── dedup/
        └── dedup.cpp            — repeated identical value results in exactly one
                                   analogWrite() (observed via the test seam / write counter)
```

Tests run on physical STM32 hardware. Duty extremes (0 and 255) produce a constant pin level
and are verified with `digitalRead()`; intermediate duty is verified on the bench with a
scope or an LED string on the real zone-switch circuit.

**`platformio.ini`:** same template as `Firmware/Tests/LED/` (env_base, one
`build_src_filter` per scenario, `-DNODE_ID=1`, `-DHAL_CAN_MODULE_ENABLED`, local
`file://../../Libraries/...` deps). No `gnu++20` requirement — AnalogOutput uses no
designated initializers.

---

## Public API

```cpp
// AnalogOutput.h

#pragma once
#include <PanelGroup.h>   // OutputBase, PinRef

namespace OpenSkyhawk {

/**
 * @brief PWM analog output. Maps a 16-bit DCS-BIOS value to PWM duty on a direct GPIO pin.
 *
 * Receives state via onControlPacket() — called by PanelGroup when a CTRL_BCAST frame
 * arrives. Ignores packets whose controlId does not match.
 *
 * Default mapping: dutyCycle = value >> 8 (16-bit → 8-bit). A custom scale function may be
 * supplied for non-linear response (e.g. perceptual dimming curves) or inversion.
 *
 * Requires a direct STM32 GPIO PinRef on a timer/PWM-capable pin — MCP23017 cannot generate
 * PWM. Non-GPIO PinRefs are rejected at configure() time with a debug assertion. The pin's
 * PWM capability itself is a schematic-time responsibility (choose a TIMx_CHy pin).
 *
 * Duty is driven to 0 during configure() and remains 0 (zone dark) until a CTRL_BCAST
 * packet with a matching controlId is received.
 */
class AnalogOutput : public OutputBase {
public:
    /** @brief Scale function type: raw 16-bit DCS value → 8-bit PWM duty. */
    using ScaleFn = uint8_t (*)(uint16_t value);

    /**
     * @brief Construct and register a PWM analog output.
     * @param controlId  DCS-BIOS output address for this value (A_4E_C_* constant from
     *                   A4EC_OutputIds.h — full 16-bit axis word, no mask parameter;
     *                   the matching *_AM mask is 0xffff for these axis outputs).
     * @param pin        PinRef for the PWM output pin. Must be a direct STM32 GPIO on a
     *                   timer channel (e.g. the zone-switch MOSFET gate).
     * @param scale      Optional custom mapping from 16-bit value to 8-bit duty.
     *                   nullptr (default) = value >> 8.
     */
    AnalogOutput(uint16_t controlId, PinRef pin, ScaleFn scale = nullptr);

    /**
     * @brief Configure the output pin and drive duty to 0.
     * Called by PanelGroup::setup() after chip.begin(). Asserts _pin.isGpio() (debug
     * assertion — MCP23017/ADS1115 cannot PWM). Sets the pin as output and writes duty 0
     * immediately: the MOSFET gate sits at 0 V and the zone stays dark until the first
     * matching CTRL_BCAST arrives.
     */
    void configure() override;

    /**
     * @brief Update PWM duty from an incoming CTRL_BCAST packet.
     * If controlId matches, computes duty = scale ? scale(value) : value >> 8 and applies
     * it via analogWrite(). Skips the write when the computed duty equals the last written
     * duty. Ignores the packet if controlId does not match.
     */
    void onControlPacket(uint16_t controlId, uint16_t value) override;

private:
    uint16_t _controlId;
    PinRef   _pin;
    ScaleFn  _scale;      // nullptr = default value >> 8
    uint8_t  _lastDuty;   // last duty written (dedup)
    bool     _hasState;   // _lastDuty valid
};

} // namespace OpenSkyhawk
```

---

## Sketch Usage

```cpp
#include <PanelGroup.h>
#include <Outputs/AnalogOutput/AnalogOutput.h>
#include <A4EC_OutputIds.h>

// ── Wiring map ────────────────────────────────────────────────────────────────
// Zone-switch MOSFET gates — direct STM32 GPIO, timer-capable pins only.
const PinRef PIN_ZONE_INSTR = PinRef(PB9);   // TIM4_CH4
const PinRef PIN_ZONE_FLOOD = PinRef(PB8);   // TIM4_CH3

// ── AnalogOutput declarations ─────────────────────────────────────────────────
// Knob-axis outputs (0–65535): A_4E_C_LIGHT_INT_INSTR = instrument-lighting knob,
// A_4E_C_LIGHT_INT_FLOOD_WHT = white floodlight knob (A4EC_OutputIds.h).
// Each constructor self-registers into PanelGroup's OutputBase list.
OpenSkyhawk::AnalogOutput instrLight(A_4E_C_LIGHT_INT_INSTR,     PIN_ZONE_INSTR);
OpenSkyhawk::AnalogOutput floodLight(A_4E_C_LIGHT_INT_FLOOD_WHT, PIN_ZONE_FLOOD);

void setup() {
    PanelGroup::setup();   // configure() drives both zones to duty 0 (dark)
}

void loop() {
    PanelGroup::loop();    // CTRL_BCAST → onControlPacket → analogWrite — nothing else needed
}
```

Like `LED`, the sketch has no direct interaction with `AnalogOutput` after construction.

---

## Key Data Structures

`ScaleFn` typedef only. All state is per-instance — no statics.

---

## Implementation Notes

### configure()

```cpp
void AnalogOutput::configure() {
    // debug assertion: _pin.isGpio() — MCP23017/ADS1115 cannot PWM
    pinMode(_pin.gpioPin(), OUTPUT);
    analogWrite(_pin.gpioPin(), 0);   // zone dark until first CTRL_BCAST
    _lastDuty = 0;
    _hasState = true;
}
```

Uses the Arduino pin API directly rather than `_pin.configureAsOutput()` / `_pin.write()` —
the PinRef digital path is bypassed because PWM is a GPIO-only, timer-backed feature. The
GPIO pin number is obtained via `_pin.gpioPin()` (valid only after the `isGpio()` assert).

### onControlPacket()

```cpp
void AnalogOutput::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    uint8_t duty = _scale ? _scale(value) : (uint8_t)(value >> 8);
    if (_hasState && duty == _lastDuty) return;   // redundant-write dedup
    analogWrite(_pin.gpioPin(), duty);
    _lastDuty = duty;
    _hasState = true;
}
```

`analogWrite()` on the stm32duino core reconfigures the timer channel on each call; the
dedup avoids that churn for repeated CTRL_BCAST values (DCS-BIOS re-broadcasts full state
after SYNC_REQ). No `update()` override — the timer generates PWM in hardware with no
per-loop work.

### PWM frequency and resolution

stm32duino defaults: **1 kHz, 8-bit** (`PWM_FREQUENCY` / `ANALOG_WRITE` resolution). 1 kHz
is flicker-free for LED backlighting and inaudible with no inductive load on the gate; the
defaults are used as-is. If a future consumer needs a different frequency,
`analogWriteFrequency()` is a global (per-timer) setting — raise it in the sketch, not in
this class.

### Pin choice is a schematic-time contract

`isGpio()` catches expander pins, but **cannot** catch a direct GPIO that lacks a timer
channel — `analogWrite()` on a non-timer pin degrades to digital threshold behaviour on the
stm32duino core. The zone-gate pin must be chosen as a `TIMx_CHy` pin at schematic capture
(pcb-design skill owns this). Beware timer sharing: all channels of one timer share a
frequency, and a timer already used elsewhere (e.g. a future tone/servo use) conflicts.

### DCS-BIOS value semantics

Brightness knobs (`defineFloat`-style axis outputs) publish the full 0–65535 word at the
address — no mask parameter, unlike `LED`. `value >> 8` maps 65535 → 255 (fully on) and
0 → 0 (off) with no special cases. Perceived LED brightness is roughly linear-enough with
duty for a first pass; a gamma-style `ScaleFn` is the hook if a zone needs a better
perceptual curve after bench evaluation.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | OutputBase, PinRef, onControlPacket dispatch |
| stm32duino core | `platform = ststm32`, `framework = arduino` | `analogWrite()` timer PWM |
| CANProtocol | `Firmware/Libraries/CANProtocol` | CTRL_BCAST handled by PanelGroup; AnalogOutput calls nothing directly |
