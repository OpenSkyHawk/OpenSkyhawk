

# File ShiftBus.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**ShiftBus**](dir_5de82edf055e68e6d2d76fc20b67149e.md) **>** [**ShiftBus.cpp**](ShiftBus_8cpp.md)

[Go to the documentation of this file](ShiftBus_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include "ShiftBus.h"
#include <STM32Board.h>

// ── Standard pin assignment for the pre-defined ShiftBus1 (override via build flags) ──

#ifndef SHIFTBUS_SCK
#define SHIFTBUS_SCK   PB3   // SPI1 remap (JTDO — JTAG released in begin(), SWD unaffected)
#endif
#ifndef SHIFTBUS_MISO
#define SHIFTBUS_MISO  PB4   // SPI1 remap (NJTRST)
#endif
#ifndef SHIFTBUS_MOSI
#define SHIFTBUS_MOSI  PB5   // SPI1 remap
#endif
#ifndef SHIFTBUS_LOAD
#define SHIFTBUS_LOAD  PB8   // '165 SH/LD̄
#endif
#ifndef SHIFTBUS_LATCH
#define SHIFTBUS_LATCH PB9   // '595 STCP
#endif

// ~1 MHz: comfortable for 74HC at 3.3 V and for a ~12" remote harness leg (33 Ω series R).
static const SPISettings kShiftBusSettings(1000000, MSBFIRST, SPI_MODE0);

namespace OpenSkyhawk {

ShiftBus* ShiftBus::_isrInstance = nullptr;

ShiftBus::ShiftBus(SPIClass& spi, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin,
                   uint8_t loadPin, uint8_t latchPin)
    : _spi(&spi), _sckPin(sckPin), _misoPin(misoPin), _mosiPin(mosiPin),
      _loadPin(loadPin), _latchPin(latchPin) {}
    // Nothing else: constructors of global buses run during static init, before HAL init.

// ── configure-time notification (PinRef::configureAsInput/Output) ──────────────

void ShiftBus::noteInput(uint8_t chip) {
    if (chip >= MAX_CHAIN) return;
    if (chip + 1 > _nIn) _nIn = chip + 1;
    _active = true;
}

void ShiftBus::noteOutput(uint8_t chip) {
    if (chip >= MAX_CHAIN) return;
    if (chip + 1 > _nOut) _nOut = chip + 1;
    _active = true;
}

// ── Frame access ────────────────────────────────────────────────────────────────

bool ShiftBus::readBit(uint8_t chip, uint8_t bit) const {
    if (chip >= MAX_CHAIN || bit > 7) return false;
    return (_inFrame[chip] >> bit) & 1u;
}

bool ShiftBus::readOutBit(uint8_t chip, uint8_t bit) const {
    if (chip >= MAX_CHAIN || bit > 7) return false;
    return (_stage[chip] >> bit) & 1u;   // last written (staged) value
}

void ShiftBus::writeBit(uint8_t chip, uint8_t bit, bool v) {
    if (chip >= MAX_CHAIN || bit > 7) return;
    // Stage is loop-owned — the ISR only ships the committed _outFrame, so a multi-pin
    // group (four stepper coils) staged across several calls can never reach the outputs
    // half-written. No masking needed here; the commit in doTransfer() is the sync point.
    const uint8_t mask = (uint8_t)(1u << bit);
    if (v) _stage[chip] |=  mask;
    else   _stage[chip] &= ~mask;
    _dirty = true;
}

bool ShiftBus::readLiveBit(uint8_t chip, uint8_t bit) {
    transfer();
    return readBit(chip, bit);
}

// ── Lifecycle ───────────────────────────────────────────────────────────────────

void ShiftBus::begin() {
    if (_begun || !_active) return;
    _begun = true;

#if defined(STM32F1xx)
    // Release JTAG so PB3 (JTDO) / PB4 (NJTRST) become GPIO/SPI. SWD (PA13/PA14) retained.
    // (The core's pin_DisconnectDebug does this too when the pins map; explicit is cheap
    // insurance and self-documenting.)
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
#endif

    // Strobes idle: LOAD high ('165 shift mode), LATCH low ('595 waits for a rising edge).
    pinMode(_loadPin, OUTPUT);
    digitalWrite(_loadPin, HIGH);
    pinMode(_latchPin, OUTPUT);
    digitalWrite(_latchPin, LOW);

#ifdef SHIFTBUS_TEST
    if (_testBypass) { doTransfer(true); return; }  // no SPI in chip-less logic tests
#endif

    _spi->setSCLK(_sckPin);
    _spi->setMISO(_misoPin);
    _spi->setMOSI(_mosiPin);
    _spi->begin();

    // Defined '595 state as early as possible: the power-on shift/storage registers are
    // undefined (OĒ is tied to GND — stance (a), boot twitch is cosmetic). The stage is
    // all zeros here, so the first transfer publishes an all-off frame, then primes the
    // '165 cache for the forceReport() boot burst.
    doTransfer(true);

    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[ShiftBus] begin: in="));  d.print(_nIn);
        d.print(F(" out="));                  d.print(_nOut);
        d.print(F(" frame="));                d.print((int)max(_nIn, _nOut));
        d.println(F("B"));
    }
}

void ShiftBus::transfer() {
    if (!_begun) return;
    // Loop-context call while the sampling ISR owns the cadence: mask interrupts for the
    // µs-scale transaction so ISR and loop never interleave on the frame buffers. The
    // window is bounded (≤ MAX_CHAIN bytes at 1 MHz ≈ 64 µs + strobes); no CAN/I2C inside.
    if (_isrActive) {
        noInterrupts();
        doTransfer(true);
        interrupts();
    } else {
        doTransfer(true);
    }
}

void ShiftBus::doTransfer(bool commitStage) {
    // Commit: publish staged writeBit() changes into the frame the wire ships. Only
    // loop-context paths commit (transfer/flushNow — masked when the ISR runs); the ISR
    // never commits, so it can never latch a half-staged multi-pin group.
    if (commitStage && _dirty) {
        for (uint8_t i = 0; i < MAX_CHAIN; i++) _outFrame[i] = _stage[i];
        _dirty = false;
    }

    const uint8_t n = max(_nIn, _nOut);
    if (n == 0) return;

#ifdef SHIFTBUS_TEST
    _transferCount++;
#endif

    // Build the wire frame. MSB-first, so byte chip bit n = pin Dn/Qn.
    // '595: the LAST byte shifted lands in chip 0 (nearest the MCU) → transmit
    // outFrame[nOut-1] … outFrame[0], padded in FRONT when the '165 chain is longer
    // (leading pad bits fall off the far end of the '595 chain).
    // '165: the FIRST byte received is chip 0 → inFrame[i] = wire[i] straight.
    const uint8_t pad = n - _nOut;
    for (uint8_t i = 0; i < pad; i++)  _wire[i] = 0;
    for (uint8_t i = 0; i < _nOut; i++) _wire[pad + i] = _outFrame[_nOut - 1 - i];

#ifdef SHIFTBUS_TEST
    for (uint8_t i = 0; i < MAX_CHAIN; i++) _wireTx[i] = _wire[i];
    if (_testBypass) {
        for (uint8_t i = 0; i < MAX_CHAIN; i++) _inFrame[i] = _testIn[i];
        return;
    }
#endif

    // '165 capture: pulse SH/LD̄ low. Data-sheet minimum pulse width is tens of ns; two
    // digitalWrite() calls at 72 MHz are comfortably above it.
    digitalWrite(_loadPin, LOW);
    digitalWrite(_loadPin, HIGH);

    _spi->beginTransaction(kShiftBusSettings);
    _spi->transfer(_wire, n);   // in-place full-duplex: TX consumed, RX replaces
    _spi->endTransaction();

    for (uint8_t i = 0; i < _nIn; i++) _inFrame[i] = _wire[i];

    // '595 publish: STCP rising edge copies the (freshly re-shifted) frame to the outputs.
    digitalWrite(_latchPin, HIGH);
    digitalWrite(_latchPin, LOW);
}

// ── Timer-ISR sampling ──────────────────────────────────────────────────────────

void ShiftBus::isrTrampoline() {
    ShiftBus* bus = _isrInstance;
    if (!bus) return;
    bus->doTransfer(false);  // ISR ships the committed frame only — never commits stage
    for (uint8_t i = 0; i < bus->_consumerCount; i++)
        bus->_consumers[i].hook(bus->_consumers[i].ctx);
}

void ShiftBus::beginIsrSampling(TIM_TypeDef* tim, uint16_t sampleHz) {
    if (!_begun || _isrActive || sampleHz == 0) return;
    if (_isrInstance && _isrInstance != this) {
        STM32Board::log("[ShiftBus] ISR sampling already claimed by another bus — skipped");
        return;
    }
    _isrInstance = this;

    // Setup-time allocation only (never in the ISR); the timer lives for the node's uptime.
    HardwareTimer* timer = new HardwareTimer(tim);
    timer->setOverflow(sampleHz, HERTZ_FORMAT);
    timer->attachInterrupt(isrTrampoline);
    _isrActive = true;   // set before resume so the first tick sees a consistent flag
    timer->resume();

    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[ShiftBus] ISR sampling @")); d.print(sampleHz); d.println(F(" Hz"));
    }
}

void ShiftBus::addIsrConsumer(void (*hook)(void* ctx), void* ctx) {
    if (_consumerCount >= MAX_ISR_CONSUMERS) {
        STM32Board::log("[ShiftBus] MAX_ISR_CONSUMERS exceeded — consumer dropped");
        return;
    }
    if (_isrActive) noInterrupts();
    _consumers[_consumerCount] = { hook, ctx };
    _consumerCount++;
    if (_isrActive) interrupts();
}

}  // namespace OpenSkyhawk

// ── Pre-defined global instance ────────────────────────────────────────────────
// Static-init safe: the constructor stores the SPI pointer + pin numbers and nothing else.

OpenSkyhawk::ShiftBus ShiftBus1(SPI, SHIFTBUS_SCK, SHIFTBUS_MISO, SHIFTBUS_MOSI,
                                SHIFTBUS_LOAD, SHIFTBUS_LATCH);

#endif  // ARDUINO_ARCH_STM32
```


