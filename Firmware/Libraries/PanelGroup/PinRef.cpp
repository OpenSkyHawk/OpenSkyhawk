#ifdef ARDUINO_ARCH_STM32

#include "PinRef.h"
#include "ADS1115.h"                       // full ADS1115 type for readSingleEnded()
#include <MCP23017.h>                      // full MCP23017 type for reference binding
#include "Helpers/ShiftBus/ShiftBus.h"     // full ShiftBus type for the SR dispatch

#ifdef PINREF_DEBUG
#include <STM32Board.h>
#endif

// ── PanelGroup package-internal functions ─────────────────────────────────────
//
// Declared here so PinRef.cpp can call them without including PanelGroup.h.
// Defined in PanelGroup.cpp when PanelGroup is fully implemented.
// PinRef tests (GPIO and NC paths) do not exercise the MCP path, so the linker
// does not require these definitions during test builds.

namespace PanelGroup {
    bool readCachedPin(const MCP23017& chip, uint8_t port, uint8_t bit);
    void writeCachedPin(MCP23017& chip, uint8_t port, uint8_t bit, bool value);
    void writeCachedPinDeferred(MCP23017& chip, uint8_t port, uint8_t bit, bool value);
    bool readLivePin(MCP23017& chip, uint8_t port, uint8_t bit);
    void noteShiftBus(OpenSkyhawk::ShiftBus& bus);  // configure-time bus auto-collection
}

// ── PIN_NC definition ─────────────────────────────────────────────────────────

const PinRef PIN_NC;

// ── Constructors ──────────────────────────────────────────────────────────────

PinRef::PinRef(uint8_t pin) : _type(Type::GPIO) {
    _src.pin = pin;
}

PinRef::PinRef(MCP23017& chip, uint8_t port, uint8_t bit) : _type(Type::MCP) {
    _src.mcp = { &chip, port, bit };
}

PinRef::PinRef(ADS1115& adc, uint8_t channel) : _type(Type::ADS) {
    _src.ads = { &adc, channel };
    adc.setGain(GAIN_ONE); // ±4.096V FSR — best resolution for 0–3.3V inputs
}

PinRef::PinRef(OpenSkyhawk::ShiftBus& bus, uint8_t chip, uint8_t bit) : _type(Type::SR) {
    // Direction is unknown until configureAsInput()/configureAsOutput(); default to input
    // so a read before configure() hits the (all-zero) '165 cache instead of the out frame.
    _src.sr = { &bus, chip, bit, /*isOut=*/false };
}

// Default no-connect ctor is constexpr, defined inline in PinRef.h (constant-initialized
// so PIN_NC is safe in global array initialisers — no static-init-order hazard).

// ── read ──────────────────────────────────────────────────────────────────────

bool PinRef::read() const {
    switch (_type) {
    case Type::GPIO:
        return digitalRead(_src.pin) == HIGH;
    case Type::MCP:
        return PanelGroup::readCachedPin(*_src.mcp.chip, _src.mcp.port, _src.mcp.bit);
    case Type::ADS:
        return readAnalog() > 32767u;
    case Type::SR:
        return _src.sr.isOut ? _src.sr.bus->readOutBit(_src.sr.chip, _src.sr.bit)
                             : _src.sr.bus->readBit(_src.sr.chip, _src.sr.bit);
    case Type::NC:
    default:
        return false;
    }
}

// ── readLive ──────────────────────────────────────────────────────────────────

bool PinRef::readLive() const {
    switch (_type) {
    case Type::GPIO:
        return digitalRead(_src.pin) == HIGH;          // already live
    case Type::MCP:
        return PanelGroup::readLivePin(*_src.mcp.chip, _src.mcp.port, _src.mcp.bit);
    case Type::ADS:
        return readAnalog() > 32767u;                  // already live
    case Type::SR:
        return _src.sr.isOut ? _src.sr.bus->readOutBit(_src.sr.chip, _src.sr.bit)
                             : _src.sr.bus->readLiveBit(_src.sr.chip, _src.sr.bit);
    case Type::NC:
    default:
        return false;
    }
}

// ── readAnalog ────────────────────────────────────────────────────────────────

uint16_t PinRef::readAnalog() const {
    switch (_type) {
    case Type::GPIO:
        // analogReadResolution(16) set in STM32Board::begin(); framework scales 12-bit → 16-bit
        return static_cast<uint16_t>(analogRead(_src.pin));
    case Type::ADS: {
        // 15-bit single-ended × 2 → 0–65534; clamp negatives (should not occur)
        int16_t raw = _src.ads.adc->readADC_SingleEnded(_src.ads.channel);
        if (raw < 0) raw = 0;
        return static_cast<uint16_t>(raw) << 1;
    }
    case Type::MCP:
#ifdef PINREF_DEBUG
        STM32Board::log("[PinRef] readAnalog on MCP23017 pin — not supported, returns 0");
#endif
        return 0;
    case Type::SR:
#ifdef PINREF_DEBUG
        STM32Board::log("[PinRef] readAnalog on ShiftBus pin — digital only, returns 0");
#endif
        return 0;
    case Type::NC:
    default:
        return 0;
    }
}

// ── write ─────────────────────────────────────────────────────────────────────

void PinRef::write(bool value) {
    switch (_type) {
    case Type::GPIO:
        digitalWrite(_src.pin, value ? HIGH : LOW);
        break;
    case Type::MCP:
        PanelGroup::writeCachedPin(*_src.mcp.chip, _src.mcp.port, _src.mcp.bit, value);
        break;
    case Type::ADS:
#ifdef PINREF_DEBUG
        STM32Board::log("[PinRef] write on ADS1115 pin — input-only, no-op");
#endif
        break;
    case Type::SR:
        if (_src.sr.isOut) {
            // SR writes are inherently deferred: set the frame bit; the next transfer()
            // (loop step, ISR tick, or flushExpanderWrites) publishes it.
            _src.sr.bus->writeBit(_src.sr.chip, _src.sr.bit, value);
        }
#ifdef PINREF_DEBUG
        else STM32Board::log("[PinRef] write on ShiftBus input ('165) pin — no-op");
#endif
        break;
    case Type::NC:
    default:
        break;
    }
}

void PinRef::writeDeferred(bool value) {
    switch (_type) {
    case Type::GPIO:
        digitalWrite(_src.pin, value ? HIGH : LOW);   // native GPIO is already immediate
        break;
    case Type::MCP:
        PanelGroup::writeCachedPinDeferred(*_src.mcp.chip, _src.mcp.port, _src.mcp.bit, value);
        break;
    case Type::SR:
        write(value);   // SR writes are already deferred — same path
        break;
    case Type::ADS:
    case Type::NC:
    default:
        break;
    }
}

// ── writeAnalog ───────────────────────────────────────────────────────────────

void PinRef::writeAnalog(uint16_t val) {
    switch (_type) {
    case Type::GPIO:
        // 16-bit → 8-bit duty cycle (upper byte)
        analogWrite(_src.pin, val >> 8);
        break;
    case Type::MCP:
#ifdef PINREF_DEBUG
        STM32Board::log("[PinRef] writeAnalog on MCP23017 pin — no PWM, no-op");
#endif
        break;
    case Type::SR:
#ifdef PINREF_DEBUG
        STM32Board::log("[PinRef] writeAnalog on ShiftBus pin — no PWM, no-op");
#endif
        break;
    case Type::ADS:
#ifdef PINREF_DEBUG
        STM32Board::log("[PinRef] writeAnalog on ADS1115 pin — input-only, no-op");
#endif
        break;
    case Type::NC:
    default:
        break;
    }
}

// ── configureAsInput ──────────────────────────────────────────────────────────

void PinRef::configureAsInput() {
    switch (_type) {
    case Type::GPIO:
        pinMode(_src.pin, INPUT);
        break;
    case Type::MCP:
        // Flat pin: 0-7 = PORT A, 8-15 = PORT B. INPUT sets IODIR=1, GPPU=0.
        _src.mcp.chip->pinMode(_src.mcp.port * 8 + _src.mcp.bit, INPUT);
        break;
    case Type::SR:
        // Bind to the '165 chain (the MCP IODIR pattern: direction set here, not in the
        // constructor). Also grows the chain length + collects the bus for begin().
        _src.sr.isOut = false;
        _src.sr.bus->noteInput(_src.sr.chip);
        PanelGroup::noteShiftBus(*_src.sr.bus);
        break;
    case Type::ADS:
    case Type::NC:
    default:
        break;
    }
}

// ── configureAsOutput ─────────────────────────────────────────────────────────

void PinRef::configureAsOutput() {
    switch (_type) {
    case Type::GPIO:
        pinMode(_src.pin, OUTPUT);
        break;
    case Type::MCP:
        // Flat pin: 0-7 = PORT A, 8-15 = PORT B. OUTPUT sets IODIR=0, GPPU=0.
        _src.mcp.chip->pinMode(_src.mcp.port * 8 + _src.mcp.bit, OUTPUT);
        break;
    case Type::SR:
        // Bind to the '595 chain.
        _src.sr.isOut = true;
        _src.sr.bus->noteOutput(_src.sr.chip);
        PanelGroup::noteShiftBus(*_src.sr.bus);
        break;
    case Type::ADS:
#ifdef PINREF_DEBUG
        STM32Board::log("[PinRef] configureAsOutput on ADS1115 pin — input-only, no-op");
#endif
        break;
    case Type::NC:
    default:
        break;
    }
}

// ── isNC / isGpio / gpioPin ───────────────────────────────────────────────────

bool PinRef::isNC()   const { return _type == Type::NC; }
bool PinRef::isGpio() const { return _type == Type::GPIO; }

uint8_t PinRef::gpioPin() const {
    if (_type == Type::GPIO) return _src.pin;
#ifdef PINREF_DEBUG
    STM32Board::log("[PinRef] gpioPin called on non-GPIO pin — returns 0");
#endif
    return 0;
}

#endif // ARDUINO_ARCH_STM32
