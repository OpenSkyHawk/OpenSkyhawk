

# File ShiftBus.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**ShiftBus**](dir_5de82edf055e68e6d2d76fc20b67149e.md) **>** [**ShiftBus.h**](ShiftBus_8h.md)

[Go to the documentation of this file](ShiftBus_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <SPI.h>

namespace OpenSkyhawk {

class ShiftBus {
public:
    static constexpr uint8_t MAX_CHAIN         = 8;   
    static constexpr uint8_t MAX_ISR_CONSUMERS = 16;  

    ShiftBus(SPIClass& spi, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin,
             uint8_t loadPin, uint8_t latchPin);

    // ── PinRef backend — package-internal, mirrors the MCP cache bridge ─────────

    void noteInput(uint8_t chip);

    void noteOutput(uint8_t chip);

    bool readBit(uint8_t chip, uint8_t bit) const;

    bool readOutBit(uint8_t chip, uint8_t bit) const;

    void writeBit(uint8_t chip, uint8_t bit, bool v);

    bool readLiveBit(uint8_t chip, uint8_t bit);

    bool dirty() const { return _dirty; }

    bool active() const { return _active; }

    // ── Lifecycle — called by PanelGroup ─────────────────────────────────────────

    void begin();

    void transfer();

    void flushNow() { transfer(); }

    // ── Timer-ISR sampling (encoder-feel mode, -DSHIFTBUS_ISR_HZ) ────────────────

    void beginIsrSampling(TIM_TypeDef* tim, uint16_t sampleHz);

    void addIsrConsumer(void (*hook)(void* ctx), void* ctx);

    bool isrActive() const { return _isrActive; }

#ifdef SHIFTBUS_TEST
    // ── Test seams — chip-less logic tests (no SPI, no strobe pins) ──────────────
    void testSetBypass(bool on) { _testBypass = on; }
    void testInjectIn(uint8_t chip, uint8_t value) { if (chip < MAX_CHAIN) _testIn[chip] = value; }
    uint8_t testOutFrame(uint8_t chip) const { return (chip < MAX_CHAIN) ? _outFrame[chip] : 0; }
    uint8_t testWire(uint8_t i) const { return (i < MAX_CHAIN) ? _wireTx[i] : 0; }
    uint16_t testTransferCount() const { return _transferCount; }
    uint8_t testChainIn()  const { return _nIn; }
    uint8_t testChainOut() const { return _nOut; }
#endif

private:
    // Unguarded body; callers handle the ISR critical section. commitStage: loop-context
    // paths publish pending writeBit() stage changes; the ISR passes false so it never
    // ships a half-written multi-pin group.
    void doTransfer(bool commitStage);

    SPIClass* _spi;
    uint8_t   _sckPin;
    uint8_t   _misoPin;
    uint8_t   _mosiPin;
    uint8_t   _loadPin;
    uint8_t   _latchPin;

    uint8_t   _nIn  = 0;        // '165 chips in use (auto-sized at configure)
    uint8_t   _nOut = 0;        // '595 chips in use
    bool      _active = false;
    bool      _begun  = false;
    volatile bool _dirty = false;   // stage differs from committed frame

    // Concurrency model (no-LTO assumption): the frames are plain bytes shared between the
    // sampling ISR and loop context. Safe because all cross-context access goes through
    // out-of-line methods in this TU (the compiler cannot cache them across calls) and
    // byte load/store is atomic on Cortex-M3. Revisit if LTO is ever enabled.
    uint8_t   _inFrame[MAX_CHAIN]  = {0};  // '165 cache: [chip] = D7..D0
    uint8_t   _stage[MAX_CHAIN]    = {0};  // '595 staging: loop-owned, writeBit() target
    uint8_t   _outFrame[MAX_CHAIN] = {0};  // '595 committed frame: what transfers ship
    uint8_t   _wire[MAX_CHAIN]     = {0};  // scratch for the in-place full-duplex transfer

    // ISR sampling
    volatile bool _isrActive = false;
    struct IsrConsumer { void (*hook)(void*); void* ctx; };
    IsrConsumer _consumers[MAX_ISR_CONSUMERS];
    uint8_t     _consumerCount = 0;
    static void isrTrampoline();   // static → member via _isrInstance (one ISR bus supported)
    static ShiftBus* _isrInstance;

#ifdef SHIFTBUS_TEST
    bool     _testBypass = false;
    uint8_t  _testIn[MAX_CHAIN]  = {0};
    uint8_t  _wireTx[MAX_CHAIN]  = {0};   // TX wire snapshot (reversal/pad coverage)
    uint16_t _transferCount = 0;
#endif
};

}  // namespace OpenSkyhawk

extern OpenSkyhawk::ShiftBus ShiftBus1;

#endif  // ARDUINO_ARCH_STM32
```


