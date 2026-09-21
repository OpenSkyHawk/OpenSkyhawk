# AngleSensorInput — Technical Specification

**Status:** Ready for implementation (#294, Firmware v0.1.0) — `AnalogInput` family member (D16)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md` (AngleSensorInput), `FirmwarePlan/00-decisions.md` (D16)
**Depends on:** `AnalogInput.md`, `PinRef.md`

---

## Responsibility

`AngleSensorInput : AnalogInput` — a magnetic angle sensor (AS5600 / MT6701) as an **absolute**
knob or axis. No wiper wear and a full 360° mechanical range, where a pot stops at ~270°. Intended
for absolute DCS-BIOS knobs such as `GUNSIGHT_KNB` (gunsight elevation) and `RADAR_RETICLE`.
Flight-control axes use linear Hall sensors on plain `AnalogInput` (#278).

**It takes a `PinRef`, like every control class.** The sensor's **analog output pin** is read
through any analog-capable `PinRef` — an STM32 ADC pin or an ADS1115 channel over I²C. The class
never touches a pin or an I²C bus directly; the `PinRef` backend does.

Everything `AnalogInput` already does is inherited — integer EWMA, hysteresis, per-instance
`pollMs`, and routing by `controlId` (`DCSIN_*` → ABS `set_state` through PanelBridge; `CTRL_*` →
HID through SimGateway, where `AxisCal` applies per-axis calibration). The subclass adds only the
**angle meaning**: degrees → raw range, and the 0°/360° wrap.

---

## Design

### Construction

`AngleSensorInput` calls `AnalogInput`'s existing **public** constructor with its `PinRef` — no
new base constructor. `centerDeg` / `travelDeg` are converted to `[minRaw, maxRaw]` before the
base call. `configure()` is inherited unchanged (`_pin.configureAsInput()`).

### The one base hook (added in the same PR as this class)

- `AnalogInput` gains `protected: virtual uint16_t readRaw();` whose default returns
  `_pin.readAnalog()`. `readScaled()` calls it instead of reading the pin itself. The members it
  needs become `protected`.
- `AngleSensorInput::readRaw()` calls `AnalogInput::readRaw()` and **re-centres** the value so
  `centerDeg` lands at mid-scale (32768). The 0°/360° seam then only matters for travel wider than
  ±180°, which removes the old "rotate the magnet mount" constraint.
- The public `AnalogInput` constructor and behaviour are unchanged, so existing sketches and the
  eight `Firmware/Tests/AnalogInput` envs are unaffected. Cost: one virtual call per read.

### Faults

No new fault contract in this class — it has no I²C of its own. An ADS1115 source behaves exactly
as it does under any `AnalogInput` today (the ADS1115's blocking read is #269's topic).

### Resolution trade-off

The chips' analog output is coarser than their register read and picks up ADC noise. That is fine
for a gunsight or reticle knob. The AS5600 can also be programmed with a start/stop angle
(ZPOS/MPOS) so its output spans only the knob's travel, recovering resolution.

### Later: digital I²C read

Reading the angle register over I²C (12-bit AS5600 / 14-bit MT6701, no ADC noise) belongs in a
**new `PinRef` backend** — the same way the ADS1115 is one (`PinRef(as5600)`) — so
`AngleSensorInput` keeps taking a `PinRef`. That backend, not this class, would carry the I²C fault
contract: `I2cHealth` (probe + back-off, as in `DrumDisplay`) and a `FaultSource` reporting
`I2C_PERIPHERAL` while tripped, holding the last good reading instead of returning a bogus value.
See `AngleSensor.md`.

### Relative-only DCS knobs (later)

A knob DCS only moves relatively (`variable_step` only) can't take an absolute value. That needs a
syncing mode like `DcsBios::RotarySyncingPotentiometer`: read DCS's current value back from the
export stream (PanelGroup nodes already receive it over CAN) and send `%+d` corrections until they
match. Not part of the first cut.

---

## Public API (sketch)

```cpp
class AngleSensorInput : public AnalogInput {
public:
    AngleSensorInput(uint16_t controlId, PinRef pin,
                     float centerDeg, float travelDeg,
                     uint16_t pollMs = DEFAULT_POLL_MS);
protected:
    uint16_t readRaw() override;        // AnalogInput::readRaw(), re-centred on centerDeg
private:
    uint16_t _centerRaw;
};
```

```cpp
ADS1115 adc(0x48, Wire);
// AS5600 OUT pin → ADS1115 channel 2 (or an STM32 ADC pin: PinRef(PA3))
OpenSkyhawk::AngleSensorInput gunsight(DCSIN_GUNSIGHT_KNB, PinRef(adc, 2),
                                        /*centerDeg*/ 180.0f, /*travelDeg*/ 150.0f);
```

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| AnalogInput | `Inputs/AnalogInput` | family base — filtering, emit, routing |
| PinRef | `PanelGroup/PinRef` | analog source: STM32 ADC or ADS1115 channel |
