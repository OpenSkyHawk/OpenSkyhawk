# AngleSensorInput — Technical Specification

**Status:** Ready for implementation — scheduled after v1.0 (D16). A pot on `AnalogInput` covers the A-4E-C today.
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md` (AngleSensorInput), `FirmwarePlan/00-decisions.md` (D16)
**Depends on:** `AnalogInput.md`, `AngleSensor.md`

---

## Responsibility

`AngleSensorInput : AnalogInput` — a magnetic angle sensor (AS5600 / MT6701) as an **absolute**
knob or axis. No wiper wear and a full 360° mechanical range, where a pot stops at ~270°. Intended
for absolute DCS-BIOS knobs such as `GUNSIGHT_KNB` (gunsight elevation) and `RADAR_RETICLE`.
Flight-control axes use linear Hall sensors on plain `AnalogInput` (#278).

It is **not** a sibling class with its own filtering: everything `AnalogInput` already does is
inherited — integer EWMA, hysteresis, per-instance `pollMs`, and routing by `controlId` (`DCSIN_*`
→ ABS `set_state` through PanelBridge; `CTRL_*` → HID through SimGateway, where `AxisCal` applies
per-axis calibration). The subclass changes only the **source**.

---

## Design

### The `AnalogInput` hook (added in the same PR as this class)

- `AnalogInput` gains `protected: virtual uint16_t readRaw();` whose default returns
  `_pin.readAnalog()`; `readScaled()` calls it instead of reading the pin directly.
- `AnalogInput`'s members become `protected`. Its public constructor and behaviour are unchanged,
  so existing sketches and the eight `Firmware/Tests/AnalogInput` envs are unaffected.
- Cost: one extra virtual call per read — nothing at 2–8 ms polling.

### The subclass

- Holds an `AngleSensor&` and overrides `readRaw()` to return the sensor's 16-bit angle.
- The constructor converts `centerDeg` / `travelDeg` into `AnalogInput`'s `[minRaw, maxRaw]`.
- **Wrap-around:** `readRaw()` re-centres the reading so `centerDeg` lands at mid-scale (32768).
  The 0°/360° seam then only matters for travel wider than ±180°, which removes the old
  "rotate the magnet mount" constraint.
- `configure()` calls `sensor.begin()`; if the chip does not answer, the node flags the fault the
  same way other absent I²C devices do (`I2cHealth`).

### Relative-only DCS knobs (later)

A knob DCS only moves relatively (`variable_step` only) can't take an absolute value. That needs a
syncing mode like `DcsBios::RotarySyncingPotentiometer`: read DCS's current value back from the
export stream (PanelGroup nodes already receive it over CAN) and send `%+d` corrections until they
match. Not part of the first cut.

### Zero-code fallback

Both chips also have an analog output pin. Wired to an ADC pin, the sensor is simply a plain
`AnalogInput` — no new class needed. The AS5600 can be programmed with a start/stop angle
(ZPOS/MPOS) so its output spans only the knob's travel.

---

## Public API (sketch)

```cpp
class AngleSensorInput : public AnalogInput {
public:
    AngleSensorInput(uint16_t controlId, AngleSensor& sensor,
                     float centerDeg, float travelDeg,
                     uint16_t pollMs = DEFAULT_POLL_MS);
    void configure() override;          // sensor.begin() + base configure
protected:
    uint16_t readRaw() override;        // sensor.readAngle(), re-centred on centerDeg
private:
    AngleSensor& _sensor;
    uint16_t     _centerRaw;
};
```

```cpp
AS5600Sensor gunsightSensor(Wire, mux, 2);   // I2C1 through I2cMux channel 2 — same pattern as DrumDisplay
OpenSkyhawk::AngleSensorInput gunsight(DCSIN_GUNSIGHT_KNB, gunsightSensor,
                                        /*centerDeg*/ 180.0f, /*travelDeg*/ 150.0f);
```

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| AnalogInput | `Inputs/AnalogInput` | family base — filtering, emit, routing |
| AngleSensor | `Inputs/AngleSensor` | AS5600 / MT6701 chip drivers |
| I2cMux | `Helpers/I2cMux` | several fixed-address chips on one bus |
