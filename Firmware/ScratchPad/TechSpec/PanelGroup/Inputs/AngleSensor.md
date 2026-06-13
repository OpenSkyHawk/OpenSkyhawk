# AngleSensor — Technical Specification

**Status:** Not Started (Phase 4)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#anglesensorinput-new`
**Depends on:** `PinRef.md`

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
| `readAngle()` | `uint16_t` | Returns current angle as a 16-bit value (0–65535 maps to 0°–360° linearly). Called by `AngleSensorInput::poll()` every 8 ms. |

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

Two axes on one sub-node require two I²C buses because both chips have fixed addresses:

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
