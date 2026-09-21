# AngleSensor — Technical Specification

**Status:** Not started — future `PinRef` backend, **not** part of `AngleSensorInput`'s first cut
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md` (AngleSensorInput)
**Depends on:** `PinRef.md`

> **Direction change (2026-09-21, PR #292 review).** Every control class takes a `PinRef`, so a
> digital angle chip is **not** an object handed to `AngleSensorInput`. `AngleSensorInput` reads the
> sensor's *analog output* through an ordinary analog `PinRef` (STM32 ADC or ADS1115). Reading the
> angle *register* over I²C, when wanted, becomes a **new `PinRef` backend** — `PinRef(as5600)`,
> the way `PinRef(adc, ch)` wraps the ADS1115 — so `AngleSensorInput` (and plain `AnalogInput`)
> accept it unchanged.
>
> That backend must carry the I²C fault contract itself: mix in `I2cHealth` (a cheap
> `i2cProbe()` — the chip ACKs, plus the mux when behind an `I2cMux` — gating every read, one
> retry every `I2C_RETRY_MS` while tripped) and report through `FaultSource` (`I2C_PERIPHERAL`
> while tripped, as `DrumDisplay` does), holding the last good reading rather than returning a
> bogus one.
>
> The chip details below (addresses, registers, resolution, conversions) remain valid reference for
> that backend; the class hierarchy and API sections describe the superseded approach.

---

## Responsibility

Abstract base class that unifies hall-effect magnetic angle sensor chips behind a
single two-method interface. Concrete subclasses (`AS5600Sensor`, `MT6701Sensor`)
implement chip-specific I²C reads and convert raw counts to a normalised 16-bit
value. `AngleSensorInput` consumes this interface and is unaware of the underlying
chip.

---

## Class Hierarchy

```
AngleSensor           (abstract base — this spec)
├── AS5600Sensor      (12-bit, fixed I²C 0x36)
└── MT6701Sensor      (14-bit, fixed I²C 0x06 in I²C mode)
```

`AngleSensorInput` (separate spec) holds a reference to an `AngleSensor` instance.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
├── AngleSensor.h          ← abstract base + concrete subclasses
└── AngleSensor.cpp        ← concrete subclass implementations
```

Both concrete classes are small enough to share a single file pair.

---

## Public API

```cpp
class AngleSensor {
public:
    virtual bool     begin()     = 0;
    virtual uint16_t readAngle() = 0;
};
```

| Method | Return | Description |
|--------|--------|-------------|
| `begin()` | `bool` | Initialises chip over I²C. Returns `false` if chip not found (address not ACK'd). Called by `PanelGroup::setup()` after `Wire.begin()`. |
| `readAngle()` | `uint16_t` | Returns current angle as a 16-bit value (0–65535 maps to 0°–360° linearly). Called through `AngleSensorInput::readRaw()` at the instance's `pollMs` (inherited from `AnalogInput`). |

---

## Concrete Subclasses

### AS5600Sensor

```cpp
class AS5600Sensor : public AngleSensor {
public:
    explicit AS5600Sensor(TwoWire& wire);
    bool     begin()     override;
    uint16_t readAngle() override;
};
```

| Property | Value |
|----------|-------|
| Raw resolution | 12-bit (0–4095) |
| I²C address | Fixed 0x36 |
| 16-bit conversion | raw × 16 → 0–65520 |
| Register | `0x0C` (RAW_ANGLE high byte), `0x0D` (low byte) |

### MT6701Sensor

```cpp
class MT6701Sensor : public AngleSensor {
public:
    explicit MT6701Sensor(TwoWire& wire);
    bool     begin()     override;
    uint16_t readAngle() override;
};
```

| Property | Value |
|----------|-------|
| Raw resolution | 14-bit (0–16383) |
| I²C address | Fixed 0x06 (I²C mode — chip must be in I²C mode, not SSI) |
| 16-bit conversion | raw × 4 → 0–65532 |
| Register | `0x03` (angle high byte), `0x04` (low byte, bits 7:2) |

---

## I²C Init Ordering

Constructors store the `TwoWire&` reference but do not touch I²C. The sketch
must call `Wire.begin()` (and `Wire1.begin()` if used) **before** `PanelGroup::setup()`.
`setup()` calls `begin()` on each registered `AngleSensorInput`, which calls
`sensor.begin()`. If `begin()` returns `false`, `PanelGroup::setup()` sets the status
LED to the warning pattern and the sensor is marked inactive.

Both chips have fixed addresses, so two sensors on one bus need an `I2cMux` (TCA9548A) — the
constructors take an optional `I2cMux&` + channel, the same pattern `DrumDisplay` uses. Without a
mux, two sensors need two I²C buses:

```cpp
Wire.begin();   // I2C1
Wire1.begin();  // I2C2

AS5600Sensor  rollSensor(Wire);
MT6701Sensor  pitchSensor(Wire1);
```

---

## Dependencies

- `TwoWire` (STM32duino / Arduino Wire library)
- No PlatformIO registry deps — chip reads are direct I²C register reads, no third-party driver needed
