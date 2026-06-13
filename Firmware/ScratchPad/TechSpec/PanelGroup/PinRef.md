# PinRef — Technical Specification

**Status:** Done

**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#pinref--hardware-abstraction`

**Depends on:** *(none — PinRef is the base layer; MCP23017/ADS1115 objects are external library instances passed by reference)*

---

## Responsibility

Value type that abstracts three hardware pin sources behind one interface: direct STM32 GPIO,
MCP23017 expander GPIO, and ADS1115 ADC channel. Used by all input and output classes so that
control declarations are identical regardless of where the physical pin lives.

Contains no logic beyond read/write dispatch — no debounce, no filtering, no state.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
├── PinRef.h        ← public class declaration, constants, small inline accessors only
└── PinRef.cpp      ← read/write/readAnalog/writeAnalog backend dispatch
```

`PinRef` is not header-only. The header defines the control-facing API and stores only enough
state to identify the backing pin source. Method bodies that call Arduino GPIO functions,
ADS1115 methods, or PanelGroup's MCP23017 cache/write path live in `PinRef.cpp`.

This keeps `PinRef.h` lightweight and allows it to forward-declare external device classes
instead of including every expander/ADC library in each file that declares a control.
PinRef objects are value types — they live on the stack or as class members and are never
heap-allocated.

`PinRef.h` is part of the PanelGroup library. Include directly in lower-level files that need
PinRef but not the full PanelGroup API:

```cpp
#include <PinRef.h>
```

It is also included transitively by `PanelGroup.h`.

### Test project

```
Firmware/Tests/PinRef/
├── platformio.ini
└── tests/
    ├── nc_sentinel.cpp     — PIN_NC.read() == false; PIN_NC.readAnalog() == 0;
    │                         PIN_NC.write() and writeAnalog() compile and are no-ops;
    │                         PIN_NC.isGpio() == false
    ├── gpio_digital.cpp    — PinRef(PA0) read/write via known loopback pin pair on STM32
    ├── gpio_identity.cpp   — PinRef(PA0).isGpio() == true and gpioPin() returns PA0;
    │                         verifies direct-only output classes have a safe raw-pin path
    ├── gpio_analog.cpp     — PinRef(PA1) readAnalog() returns 16-bit scaled value;
    │                         STM32Board::begin() sets analogReadResolution(16) so the
    │                         framework scales 12-bit hardware → 0–65520 internally;
    │                         verified against known mid-rail voltage
    ├── ads1115_analog.cpp  — PinRef(adc, channel) readAnalog() returns 15-bit×2 scaled
    │                         value (GAIN_ONE, ±4.096V FSR, 3.3V→~52800); read() threshold
    │                         at 32767; write() is no-op; isGpio() == false.
    │                         Hardware: ADS1115 @ 0x48 on I2C1.
    └── i2c_scan/           — Diagnostic utility: scans I2C1 (PB6/7), I2C1-remap (PB8/9),
                              and I2C2 (PB10/11) for device addresses. Not a formal
                              pass/fail test; used for bench bring-up wiring verification.
```

MCP23017 cached-read behavior is exercised by PanelGroup tests because PanelGroup owns
the MCP23017 state cache and interrupt/polling refresh path; those tests also verify
isGpio() is false for expander pins.

ADS1115 PinRef behavior is tested directly in `ads1115_analog` — this is the correct
location. AnalogInput is a separate DCS-mapping layer that uses PinRef; it is not a
dependency of PinRef and does not own the raw ADS1115 read path.

`nc_sentinel`, `gpio_identity`, and `gpio_digital` require only a bare STM32 board.
`gpio_analog` requires a resistor divider on PA1.
`ads1115_analog` requires an ADS1115 dev board on I2C1 (bench: I2C1 remap PB8/PB9;
production PCB uses Wire.begin() default PB6/PB7).

**`platformio.ini`:**

```ini
[platformio]
src_dir = tests

[env_base]
platform = ststm32
board = genericSTM32F103CB
framework = arduino
build_flags =
    -DNODE_ID=1
    -DHAL_CAN_MODULE_ENABLED
lib_deps =
    file://../../Libraries/PanelGroup
    file://../../Libraries/CANProtocol
    file://../../Libraries/STM32Board
    file://../../Libraries/HIDControls
    blemasle/MCP23017@^2.0.0
    adafruit/Adafruit ADS1X15@^2.0.0

[env:test_nc_sentinel]
extends = env_base
build_src_filter = -<*> +<nc_sentinel/nc_sentinel.cpp>

[env:test_gpio_digital]
extends = env_base
build_src_filter = -<*> +<gpio_digital/gpio_digital.cpp>

[env:test_gpio_identity]
extends = env_base
build_src_filter = -<*> +<gpio_identity/gpio_identity.cpp>

[env:test_gpio_analog]
extends = env_base
build_src_filter = -<*> +<gpio_analog/gpio_analog.cpp>

[env:test_ads1115_analog]
extends = env_base
build_src_filter = -<*> +<ads1115_analog/ads1115_analog.cpp>

[env:test_i2c_scan]
extends = env_base
build_src_filter = -<*> +<i2c_scan/i2c_scan.cpp>
lib_deps =
    file://../../Libraries/STM32Board
    file://../../Libraries/CANProtocol
    file://../../Libraries/HIDControls
```

---

## Public API

```cpp
// PinRef.h

// Forward declarations — full definitions provided by their respective libraries.
// PinRef.h stores pointers; it does not need the complete type at this point.
class MCP23017;  // blemasle/arduino-mcp23017 library class
class ADS1115;   // Adafruit ADS1X15 library (Adafruit_ADS1115) or thin wrapper

// Port constants for MCP23017 constructor
static constexpr uint8_t PORT_A = 0;
static constexpr uint8_t PORT_B = 1;

class PinRef {
public:

    // ── Constructors ──────────────────────────────────────────────────────────

    /**
     * @brief Direct STM32 GPIO pin.
     * @param pin Arduino pin number (e.g. PA0, PB5).
     */
    explicit PinRef(uint8_t pin);

    /**
     * @brief MCP23017 expander GPIO.
     * @param chip  Reference to the registered MCP23017 instance.
     * @param port  PORT_A (0) or PORT_B (1).
     * @param bit   Bit within the port, 0–7.
     */
    PinRef(MCP23017& chip, uint8_t port, uint8_t bit);

    /**
     * @brief ADS1115 ADC channel.
     * @param adc      Reference to the ADS1115 instance.
     * @param channel  Channel number, 0–3.
     */
    PinRef(ADS1115& adc, uint8_t channel);

    /**
     * @brief No-connect sentinel — represents a position with no physical pin.
     *
     * All reads return false / 0. All writes are no-ops. Use as PIN_NC.
     * See also: ANALOG_NC (0xFFFF) for AnalogMultiPos unused positions.
     */
    PinRef();

    // ── Interface ─────────────────────────────────────────────────────────────

    /**
     * @brief Digital read.
     *
     * GPIO: digitalRead(pin) — returns true when the pin is HIGH.
     * MCP23017: reads the raw bit from the last INTCAP or full port read via PanelGroup.
     * ADS1115: returns true if readAnalog() > 32767 (half-scale threshold).
     * NC: always returns false.
     *
     * @return true = HIGH, false = LOW.
     */
    bool read() const;

    /**
     * @brief Analog read, normalised to 16-bit (0–65535).
     *
     * GPIO: analogRead(pin) → 0–65520 (12-bit hardware, framework-scaled to 16-bit via
     *        analogReadResolution(16) set in STM32Board::begin(); PinRef does no shifting).
     * ADS1115: readADC_SingleEnded(channel) × 2 → 0–65534. GAIN_ONE (±4.096V FSR) is set
     *        at PinRef construction — best resolution for 0–3.3V inputs. Returns the true
     *        16-bit ADC range; callers (DCS-BIOS output classes, HID layer) normalize to
     *        their own domain. 3.3V input ≈ 52800; 0V = 0.
     * MCP23017: always returns 0 — MCP23017 has no ADC. Debug-mode assertion fires.
     * NC: always returns 0.
     *
     * @return Normalised 16-bit ADC value.
     */
    uint16_t readAnalog() const;

    /**
     * @brief Digital write.
     *
     * GPIO: digitalWrite(pin, value ? HIGH : LOW).
     * MCP23017: sets the output bit via PanelGroup's expander write path.
     * ADS1115: no-op — ADS1115 is input-only. Debug-mode assertion fires.
     * NC: no-op.
     *
     * @param value true = HIGH, false = LOW.
     */
    void write(bool value);

    /**
     * @brief Analog write (PWM).
     *
     * GPIO only: analogWrite(pin, val >> 8) — 16-bit value mapped to 8-bit duty cycle.
     * MCP23017: no-op — MCP23017 cannot do PWM. Debug-mode assertion fires.
     * ADS1115: no-op — ADS1115 is input-only. Debug-mode assertion fires.
     * NC: no-op.
     *
     * @note Must be a PWM-capable STM32 GPIO pin. No runtime check is performed.
     * @param val 16-bit value (0–65535); upper 8 bits used as duty cycle.
     */
    void writeAnalog(uint16_t val);

    /**
     * @brief Configure this pin as a digital input.
     *
     * GPIO:     calls pinMode(pin, INPUT). Board wiring must provide the required bias
     *           (OpenSkyhawk switch inputs typically use external pull-ups to +3.3V
     *           with the switch closing to GND).
     * MCP23017: sets the IODIR bit to 1 (input) and GPPU bit to 0 (internal pull-up
     *           disabled) for this specific pin only. The MCP23017 provides weak pull-ups
     *           only, not pull-downs; OpenSkyhawk boards should not rely on GPPU when
     *           the schematic already provides an external bias resistor.
     * ADS1115:  no-op — always an analog input; no mode to configure.
     * NC:       no-op.
     *
     * @note Must be called during PanelGroup::setup() after chip.begin() — MCP23017
     *       register writes require the chip to be initialised first. Input classes
     *       call this from their configure() override; do not call from a constructor.
     */
    void configureAsInput();

    /**
     * @brief Configure this pin as a digital output.
     *
     * GPIO:     calls pinMode(pin, OUTPUT).
     * MCP23017: sets the IODIR bit to 0 (output) and GPPU bit to 0 (pull-up disabled)
     *           for this specific pin only. No other pins on the chip are affected.
     * ADS1115:  no-op — input-only. Debug-mode assertion fires.
     * NC:       no-op.
     *
     * @note Must be called during PanelGroup::setup() after chip.begin(). Output classes
     *       call this from their configure() override; do not call from a constructor.
     */
    void configureAsOutput();

    /**
     * @brief Returns true if this is the NC (no-connect) sentinel.
     */
    bool isNC() const;

    /**
     * @brief Returns true if this PinRef wraps a direct STM32 GPIO pin.
     *
     * Direct-only output classes such as AnalogOutput and ServoOutput use this to reject
     * MCP23017, ADS1115, and NC pins. This does not prove the GPIO supports PWM or servo
     * timer output; sketches must still choose pins from the board wiring map intentionally.
     */
    bool isGpio() const;

    /**
     * @brief Return the raw Arduino pin number for direct STM32 GPIO PinRefs.
     *
     * Used by output classes that must call APIs requiring a raw pin, such as Servo.attach().
     *
     * @return Arduino pin number (e.g. PA0, PB9), or 0 for non-GPIO PinRefs in production.
     * @note Debug builds should assert/log if called on MCP23017, ADS1115, or NC PinRefs.
     */
    uint8_t gpioPin() const;

} // class PinRef

/**
 * @brief No-connect sentinel. Pass as a PinRef where no physical pin exists.
 *
 * Equivalent to PinRef() default constructor. Provided as a named constant
 * for readability in wiring maps and input declarations.
 *
 * Usage: PinRef pins[] = { PinRef(exp1, PORT_A, 0), PIN_NC, PinRef(exp1, PORT_A, 2) };
 */
extern const PinRef PIN_NC;
```

---

## Key Data Structures

### Internal representation — tagged union

```cpp
private:
    enum class Type : uint8_t { GPIO, MCP, ADS, NC };

    Type _type;

    union {
        uint8_t pin;                                              // GPIO
        struct { MCP23017* chip; uint8_t port; uint8_t bit; } mcp;
        struct { ADS1115*  adc;  uint8_t channel;           } ads;
    } _src;
```

`read()`, `readAnalog()`, `write()`, and `writeAnalog()` are implemented in `PinRef.cpp`.
Each switches on `_type`. No virtual dispatch — no vtable, no heap allocation, no pointer
indirection beyond the stored chip/adc pointer. Size: 1 byte (type tag) + largest union
member (~10 bytes for MCP) = ~12 bytes.

---

## Implementation Notes

### Usage examples

PinRef is intentionally the only abstraction used in control declarations. It does not wrap or
own MCP23017/ADS1115 objects; sketches create the library objects directly and pass references
into PinRef.

```cpp
// Direct STM32 GPIO:
const PinRef PIN_MASTER_ARM(PB5);

// MCP23017 expander from blemasle/arduino-mcp23017:
MCP23017 exp1(0x20, Wire);
const PinRef PIN_EJECT_SAFE(exp1, PORT_A, 3);

// ADS1115 ADC instance:
ADS1115 adc(0x48, Wire);
const PinRef PIN_RUDDER(adc, 0);

OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PIN_MASTER_ARM);
OpenSkyhawk::Switch2Pos ejectSafe(DCSIN_SEAT_EJECT_SAFE, PIN_EJECT_SAFE);
OpenSkyhawk::AnalogInput rudder(CTRL_RUDDER, PIN_RUDDER);
```

PanelGroup owns MCP23017 registration, setup, interrupt/polling refresh, and cached port
state. PinRef only stores `{chip, port, bit}` and asks PanelGroup for the cached bit value
when `read()` is called.

### Raw digital level

`PinRef::read()` returns the raw logical level of the backing pin: `true` means HIGH and
`false` means LOW. It does not apply active-low switch semantics.

Most GPIO and MCP23017 switch inputs are wired active-low (pulled up, closed to GND). Input
classes such as `Switch2Pos` and `ActionButton` own that polarity decision, typically by
treating LOW as active when their `reverse` flag is false and HIGH as active when `reverse`
is true. This keeps `PinRef` as a hardware abstraction and keeps control behavior inside the
input classes.

### ADS1115 read path

`PinRef(ADS1115& adc, uint8_t channel)` stores a pointer to the ADS1115 instance and calls
`adc.setGain(GAIN_ONE)` (±4.096V FSR) at construction — best resolution for 0–3.3V pot
inputs. If multiple PinRefs share the same ADS1115 instance, each construction calls
`setGain(GAIN_ONE)`, which is idempotent.

`readAnalog()` calls `adc.readADC_SingleEnded(channel)` which initiates a single-ended
conversion and blocks for one conversion period (~8 ms at default 128 SPS). Polling rate
for `AnalogInput` and `AnalogMultiPos` is 8 ms — this matches the conversion time. Do not
call `readAnalog()` on an ADS1115 PinRef from an ISR.

Raw single-ended result: 0–32767 (15-bit). Multiplied by 2 → 0–65534. At 3.3V input,
returns ≈ 52800. Callers (DCS-BIOS output classes, HID layer) normalize to their domain.

### MCP23017 read path

`PinRef(MCP23017& chip, uint8_t port, uint8_t bit).read()` does **not** trigger an I2C read.
It returns the cached port state maintained by PanelGroup's interrupt dispatch and polling
fallback. PanelGroup is responsible for keeping the cache current; PinRef just extracts
the relevant bit.

This means PinRef::read() for MCP23017 pins is effectively free — no I2C, no blocking.

### Debug-mode assertions

Invalid combinations (readAnalog on MCP23017, writeAnalog on MCP23017/ADS1115) fire a
`STM32Board::log()` message and return 0 / no-op in production. Define `PINREF_DEBUG` to
enable assertions at compile time during development.

### PIN_NC definition

`PIN_NC` is declared in `PinRef.h` and defined once in `PinRef.cpp`:

```cpp
// PinRef.h
extern const PinRef PIN_NC;

// PinRef.cpp
const PinRef PIN_NC;
```

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| STM32duino Arduino core | PlatformIO `framework = arduino` | `digitalRead`, `digitalWrite`, `analogRead`, `analogWrite` for GPIO path |
| MCP23017 | blemasle/arduino-mcp23017 library | Forward-declared in PinRef.h; object lifetime owned by sketch |
| ADS1115 | Adafruit ADS1X15 library | Forward-declared in PinRef.h; object lifetime owned by sketch |
| `NODE_ID` define | `platformio.ini` `build_flags` | Required by PanelGroup library; PinRef does not use it directly |
