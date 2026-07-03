/**
 * @file PinRef.h
 * @brief Hardware pin abstraction for OpenSkyhawk panel controls.
 *
 * @details Abstracts four hardware pin sources behind a single interface:
 * direct STM32 GPIO, MCP23017 expander GPIO, ADS1115 ADC channel, and a
 * ShiftBus bit (74HC165 input / 74HC595 output over SPI).
 * Used by all input and output classes so control declarations are identical
 * regardless of where the physical pin lives.
 *
 * Contains no logic beyond read/write dispatch — no debounce, no filtering, no state.
 * PinRef objects are value types: live on the stack or as class members, never heap-allocated.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>

// Forward declarations — PinRef.h stores pointers; full types are not needed here.
// Complete definitions: MCP23017 via <MCP23017.h>, ADS1115 via <ADS1115.h>,
// ShiftBus via <Helpers/ShiftBus/ShiftBus.h>.
class MCP23017;  ///< blemasle/arduino-mcp23017 library class
class ADS1115;   ///< Thin wrapper over Adafruit_ADS1115; see ADS1115.h
namespace OpenSkyhawk { class ShiftBus; }  ///< 74HC165/'595 SPI shift-register bus

static constexpr uint8_t PORT_A = 0;  ///< MCP23017 GPA port constant for constructors
static constexpr uint8_t PORT_B = 1;  ///< MCP23017 GPB port constant for constructors

/**
 * @brief Hardware pin abstraction used by all OpenSkyhawk input and output classes.
 *
 * @details Provides read, write, readAnalog, and writeAnalog behind a uniform interface
 * regardless of whether the backing pin is a direct STM32 GPIO, an MCP23017 expander
 * bit, or an ADS1115 ADC channel. Size: ~12 bytes (1-byte type tag + largest union member).
 */
class PinRef {
public:

    // ── Constructors ──────────────────────────────────────────────────────────────

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
     * @brief Shift-register bus bit ('165 input or '595 output).
     *
     * @details Direction is set at configure time by the consuming class —
     * configureAsInput() binds the pin to the '165 chain, configureAsOutput() to the
     * '595 chain (the MCP IODIR pattern). @p chip means "position within that chain".
     *
     * @param bus   ShiftBus instance — normally the pre-defined global `ShiftBus1`.
     * @param chip  Position along the cascade, 0 = nearest the MCU.
     * @param bit   Dn ('165) or Qn ('595) pin of that chip, 0–7.
     */
    PinRef(OpenSkyhawk::ShiftBus& bus, uint8_t chip, uint8_t bit);

    /**
     * @brief No-connect sentinel — represents a position with no physical pin.
     *
     * @details All reads return false / 0. All writes are no-ops.
     * Equivalent to PIN_NC. Provided for use in array initialisers.
     *
     * @note constexpr so `PIN_NC` — and any default-constructed NC PinRef — is
     * constant-initialized. This makes it safe to place in a global wiring-map array
     * without the static-initialization-order hazard a dynamically-initialized global
     * would have.
     */
    constexpr PinRef() : _type(Type::NC), _src{} {}

    // ── Interface ─────────────────────────────────────────────────────────────────

    /**
     * @brief Digital read.
     *
     * GPIO: digitalRead(pin) — true when the pin is HIGH.
     * MCP23017: cached bit from PanelGroup's last INTCAP or port read. No I2C.
     * ADS1115: true if readAnalog() > 32767 (half-scale threshold).
     * ShiftBus: cached '165 frame bit (input) or last written '595 bit (output). No SPI.
     * NC: always false.
     *
     * @return true = HIGH, false = LOW.
     */
    bool read() const;

    /**
     * @brief Live digital read — bypasses any cache.
     *
     * GPIO: digitalRead(pin) (already live).
     * MCP23017: a fresh readPort() over I2C, also refreshing PanelGroup's cache.
     * ADS1115: live readAnalog() > half-scale. NC: false.
     *
     * @return true = HIGH, false = LOW.
     * @note For time-critical reads before PanelGroup::loop() refreshes the cache — e.g. blocking
     *       homing on an MCP-backed sensor. Costs one I2C transaction per call on MCP pins.
     */
    bool readLive() const;

    /**
     * @brief Analog read, normalised to 16-bit (0–65535).
     *
     * GPIO: analogRead(pin) × 16 → 0–65520 (12-bit ADC scaled to 16-bit).
     * ADS1115: readADC_SingleEnded(channel) × 2 → 0–65534 (15-bit single-ended scaled).
     * MCP23017: always 0; debug assertion fires if PINREF_DEBUG is defined.
     * NC: always 0.
     *
     * @return Normalised 16-bit ADC value.
     * @note Do not call from an ISR on ADS1115 pins — blocks ~8 ms per conversion.
     */
    uint16_t readAnalog() const;

    /**
     * @brief Digital write.
     *
     * GPIO: digitalWrite(pin, value ? HIGH : LOW).
     * MCP23017: sets the output bit via PanelGroup's expander write path and cache.
     * ADS1115: no-op; debug assertion fires if PINREF_DEBUG is defined.
     * NC: no-op.
     *
     * @param value true = HIGH, false = LOW.
     */
    void write(bool value);

    /**
     * @brief Like write(), but MCP writes only update the cache (no I2C) — the caller then
     *        invokes PanelGroup::flushExpanderWrites() to push each port in one writePort().
     *
     * GPIO writes stay immediate (already cheap). Lets a multi-pin output batch its expander
     * writes: one I2C transaction per port instead of one read-modify-write per pin.
     *
     * @param value true = HIGH, false = LOW.
     */
    void writeDeferred(bool value);

    /**
     * @brief Analog write (PWM). GPIO only.
     *
     * GPIO: analogWrite(pin, val >> 8) — maps 16-bit value to 8-bit duty cycle.
     * MCP23017: no-op; debug assertion fires if PINREF_DEBUG is defined.
     * ADS1115: no-op; debug assertion fires if PINREF_DEBUG is defined.
     * NC: no-op.
     *
     * @param val 16-bit value (0–65535); upper 8 bits used as PWM duty cycle.
     * @note Pin must be a PWM-capable STM32 GPIO. No runtime check is performed.
     */
    void writeAnalog(uint16_t val);

    /**
     * @brief Configure this pin as a digital input.
     *
     * GPIO:     calls pinMode(pin, INPUT). Bias is provided by board wiring.
     * MCP23017: sets IODIR bit to 1 (input) and GPPU bit to 0 (pull-up disabled)
     *           for this pin only via PanelGroup's expander management path.
     * ADS1115:  no-op — always an analog input.
     * NC:       no-op.
     *
     * @note Must be called after chip.begin() — i.e. from an InputBase::configure()
     *       override called by PanelGroup::setup(), not from a constructor.
     */
    void configureAsInput();

    /**
     * @brief Configure this pin as a digital output.
     *
     * GPIO:     calls pinMode(pin, OUTPUT).
     * MCP23017: sets IODIR bit to 0 (output) and GPPU bit to 0 (pull-up disabled)
     *           for this pin only via PanelGroup's expander management path.
     * ADS1115:  no-op; debug assertion fires if PINREF_DEBUG is defined.
     * NC:       no-op.
     *
     * @note Must be called after chip.begin() — i.e. from an OutputBase::configure()
     *       override called by PanelGroup::setup(), not from a constructor.
     */
    void configureAsOutput();

    /**
     * @brief Returns true if this is the NC (no-connect) sentinel.
     */
    bool isNC() const;

    /**
     * @brief Returns true if this PinRef wraps a direct STM32 GPIO pin.
     *
     * @details Used by direct-only output classes (AnalogOutput, ServoOutput) to reject
     * MCP23017, ADS1115, and NC pins at construction time.
     */
    bool isGpio() const;

    /**
     * @brief True when this pin's cached state is refreshed by a high-rate sampler that
     *        runs independently of PanelGroup::loop() (today: ShiftBus timer-ISR sampling).
     *
     * @details Lets an input class decide whether InputBase::sampleTick() should own its
     * decode: a pin whose cache only refreshes at loop rate (GPIO/MCP/ADS) gains nothing
     * from sampler ownership and keeps its loop-rate path. The class never needs to know
     * which sampler exists — only what its own pin promises.
     */
    bool isSampledSource() const;

    /**
     * @brief Return the raw Arduino pin number for GPIO PinRefs.
     *
     * @details Used by output classes that must call APIs requiring a raw pin number,
     * such as Servo::attach().
     *
     * @return Arduino pin number (e.g. PA0, PB9), or 0 for non-GPIO PinRefs.
     * @note Debug builds log if called on non-GPIO PinRefs.
     */
    uint8_t gpioPin() const;

private:
    /** @brief Backing source type. */
    enum class Type : uint8_t {
        GPIO,  ///< Direct STM32 GPIO pin
        MCP,   ///< MCP23017 expander GPIO
        ADS,   ///< ADS1115 ADC channel
        SR,    ///< ShiftBus bit — '165 input or '595 output (direction set at configure)
        NC,    ///< No-connect sentinel
    };

    Type _type;  ///< Discriminator for the union below.

    union {
        uint8_t pin;                                                ///< GPIO pin number
        struct { MCP23017* chip; uint8_t port; uint8_t bit; } mcp; ///< MCP23017 source
        struct { ADS1115*  adc;  uint8_t channel;           } ads; ///< ADS1115 source
        struct { OpenSkyhawk::ShiftBus* bus; uint8_t chip;
                 uint8_t bit; bool isOut;                   } sr;  ///< ShiftBus source
    } _src;
};

/**
 * @brief No-connect sentinel. Pass where no physical pin exists.
 *
 * @details All reads return false / 0; all writes are no-ops.
 * Provided as a named constant for readability in wiring maps.
 *
 * Usage: `PinRef pins[] = { PinRef(exp1, PORT_A, 0), PIN_NC, PinRef(exp1, PORT_A, 2) };`
 *
 * See also: ANALOG_NC (0xFFFF) for AnalogMultiPos unused voltage levels.
 */
extern const PinRef PIN_NC;

#endif // ARDUINO_ARCH_STM32
