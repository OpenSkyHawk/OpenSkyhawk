# Dimmer — Technical Specification

**Status:** Ready for implementation (#288). Renamed from `AnalogOutput`, which is now the family
base this class derives from (D16).
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md` (AnalogOutput family), `FirmwarePlan/00-decisions.md` (D16), `FirmwarePlan/10-implementation-plan.md` (Phase 5)
**Depends on:** `AnalogOutput.md`, `PinRef.md`, `PanelGroup.md`

---

## Responsibility

`Dimmer : AnalogOutput` — the `DcsBios::Dimmer` equivalent. Maps a 16-bit DCS-BIOS value to a PWM
duty cycle on a **direct STM32 GPIO pin**. Primary use: backlight zone dimming — the low-side
MOSFET gate of an LED zone (AO3400A; IRLML2502 on base Rev 1 — see
`docs/_source/hardware-standards.md` → *LED Zone Switch*), one `Dimmer` per zone (two on the
PanelGroup base: PA6 / PA7). The A-4E-C drives five such outputs: `LIGHTS_CONSOLE`,
`LIGHTS_INSTRUMENTS`, `LIGHTS_FLOOD_RED`, `LIGHTS_FLOOD_WHITE`, `APG53A_GLOW`. Also usable for any
other PWM-proportional load (floodlights, dimmable indicators).

The `AnalogOutput` base owns `controlId` / mask / shift matching, registration and change dedup;
`Dimmer` implements `apply()` as a PWM write (with its own optional `ScaleFn`) and adds the
GPIO-only `configure()`.

**Takes a `PinRef`, like every control class, and checks its type.** PWM brightness is generated
by an STM32 **timer channel**, so only a direct GPIO wired to one can drive a zone. `configure()`
verifies that — `_pin.isGpio()` and a timer channel on that pin — and if the check fails, logs the
problem on DiagSerial and **disables the output** (no writes at all) instead of silently degrading:
on an MCP23017 or ShiftBus pin `PinRef::writeAnalog()` would be a no-op, and on a GPIO without a
timer `analogWrite()` degrades to an on/off threshold. There is no software PWM through an expander
on purpose: MCP23017 and '595 bits only change once per loop or per bus transfer, so a software duty
cycle there would flicker and keep the bus busy, where timer PWM runs in hardware with no CPU cost.
Does **not**
do on/off threshold logic — that is `LED`'s responsibility. Does **not** invert polarity —
the zone-switch hardware standard is fixed (higher duty = brighter); a lamp-test or
inverted-drive need would be a `scale` function, not a constructor flag. It **does** keep
the last written duty (`_lastDuty` / `_hasState`) and skips the `analogWrite()` when the
newly computed duty is unchanged — a redundant-write dedup, not value buffering.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
├── Outputs/AnalogOutput/AnalogOutput.{h,cpp}   ← family base (see AnalogOutput.md)
└── Outputs/Dimmer/Dimmer.{h,cpp}
```

### Test project

```
Firmware/Tests/Dimmer/
├── platformio.ini
└── tests/
    ├── configure/
    │   └── configure.cpp        — configure() drives duty 0 (pin constant LOW, verified via
    │                              digitalRead); a non-GPIO PinRef (MCP23017 / ShiftBus /
    │                              ADS1115) or a GPIO without a timer channel disables the
    │                              output and logs it — no writes afterwards
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
`file://../../Libraries/...` deps). No `gnu++20` requirement — Dimmer uses no
designated initializers.

---

## Public API

```cpp
// Dimmer.h

#pragma once
#include <Outputs/AnalogOutput/AnalogOutput.h>   // family base (OutputBase, PinRef via PanelGroup.h)

namespace OpenSkyhawk {

/**
 * @brief PWM backlight / dimmer output — DcsBios::Dimmer equivalent. Maps a 16-bit DCS-BIOS value
 *        to PWM duty on a direct GPIO timer pin.
 *
 * Writes through PinRef::writeAnalog() (16-bit in; the GPIO path outputs value >> 8 as 8-bit
 * duty). A custom scale function may reshape the 16-bit value first — perceptual dimming curves,
 * inversion.
 *
 * Takes a PinRef and checks it: configure() requires a direct GPIO on a timer channel; anything
 * else is logged and the output stays disabled. Choosing a TIMx_CHy pin is a schematic-time
 * responsibility.
 *
 * Duty is driven to 0 during configure() and remains 0 (zone dark) until a CTRL_BCAST
 * packet with a matching controlId is received.
 */
class Dimmer : public AnalogOutput {
public:
    /** @brief Scale function type: raw 16-bit DCS value → 16-bit value handed to writeAnalog(). */
    using ScaleFn = uint16_t (*)(uint16_t value);

    /**
     * @brief Construct and register a dimmer.
     * @param controlId  DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h — the full
     *                   16-bit word; the matching *_AM mask is 0xffff for these outputs).
     * @param pin        PinRef for the PWM output: must be a direct STM32 GPIO on a timer channel
     *                   (the zone-switch MOSFET gate) — PinRef(PA6) / PinRef(PA7) on the base.
     * @param scale      Optional reshaping of the 16-bit value before it is written.
     *                   nullptr (default) = identity.
     */
    Dimmer(uint16_t controlId, PinRef pin, ScaleFn scale = nullptr);

    /**
     * @brief Configure the output pin and drive duty to 0.
     * Called by PanelGroup::setup() after chip.begin(). Checks the PinRef is a direct GPIO on a timer
     * channel — otherwise logs and disables the output. Then configures it as an output and writes
     * duty 0 immediately: the
     * MOSFET gate sits at 0 V and the zone stays dark until the first matching CTRL_BCAST.
     */
    void configure() override;

protected:
    /**
     * @brief Called by the AnalogOutput base with a matching value. Computes
     *        v = scale ? scale(value) : value and writes it with _pin.writeAnalog(v), skipping the
     *        write when the resulting 8-bit duty (v >> 8) equals the last one written. No-op while
     *        the output is disabled.
     */
    void apply(uint16_t value) override;

private:
    PinRef   _pin;
    bool     _enabled;    // false when configure() rejected the pin — apply() writes nothing
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
#include <Outputs/Dimmer/Dimmer.h>
#include <A4EC_OutputIds.h>

// ── Wiring map ────────────────────────────────────────────────────────────────
// Zone-switch MOSFET gates — direct STM32 GPIO, timer-capable pins only (PanelGroup base).
const PinRef PIN_ZONE_INSTR = PinRef(PA6);   // TIM3_CH1 → J_BL1
const PinRef PIN_ZONE_FLOOD = PinRef(PA7);   // TIM3_CH2 → J_BL2

// ── Dimmer declarations ───────────────────────────────────────────────────────
// Lamp-intensity outputs (0–65535) computed by the sim — they already account for the knob,
// the master lighting switches and aircraft power, so they are preferred over the raw knob
// positions (A_4E_C_LIGHT_INT_*). Each constructor self-registers into PanelGroup's OutputBase list.
OpenSkyhawk::Dimmer instrLight(A_4E_C_LIGHTS_INSTRUMENTS,  PIN_ZONE_INSTR);
OpenSkyhawk::Dimmer floodLight(A_4E_C_LIGHTS_FLOOD_WHITE, PIN_ZONE_FLOOD);

void setup() {
    PanelGroup::setup();   // configure() drives both zones to duty 0 (dark)
}

void loop() {
    PanelGroup::loop();    // CTRL_BCAST → onControlPacket → analogWrite — nothing else needed
}
```

Like `LED`, the sketch has no direct interaction with `Dimmer` after construction.

---

## Key Data Structures

`ScaleFn` typedef only. All state is per-instance — no statics.

---

## Implementation Notes

### configure()

```cpp
void Dimmer::configure() {
    _enabled = _pin.isGpio() &&
               pinmap_peripheral(digitalPinToPinName(_pin.gpioPin()), PinMap_TIM) != NP;
    if (!_enabled) {
        STM32Board::log("[Dimmer] pin is not a timer-capable GPIO — output disabled");
        return;                        // never drive a pin that can't dim
    }
    _pin.configureAsOutput();
    _pin.writeAnalog(0);               // zone dark until first CTRL_BCAST
    _lastDuty = 0;
    _hasState = true;
}
```

All pin access goes through `PinRef` (`configureAsOutput()`, `writeAnalog()`); the class only
*inspects* the pin to check it. The timer check reads the stm32duino pin map for the GPIO behind the
PinRef — a direct GPIO without a timer channel is caught at startup, not discovered as a lamp that
only switches on and off.

### apply()

```cpp
// The AnalogOutput base filters controlId / mask / shift, dedups unchanged values and calls apply().
void Dimmer::apply(uint16_t value) {
    if (!_enabled) return;
    const uint16_t v    = _scale ? _scale(value) : value;
    const uint8_t  duty = (uint8_t)(v >> 8);          // what the GPIO path actually outputs
    if (_hasState && duty == _lastDuty) return;       // redundant-write dedup
    _pin.writeAnalog(v);
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

The `configure()` check rules out expander / ShiftBus pins and catches a direct GPIO that lacks a timer
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
| AnalogOutput | `Outputs/AnalogOutput` | family base — matching, registration, `apply()` dispatch |
| CANProtocol | `Firmware/Libraries/CANProtocol` | CTRL_BCAST handled by PanelGroup; Dimmer calls nothing directly |
