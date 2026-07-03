/**
 * @file ShiftBus.h
 * @brief Shared SPI shift-register bus — 74HC165 input chain + 74HC595 output chain.
 *
 * @details One ShiftBus instance owns one SPI bus carrying both chains: '165
 * (parallel-in/serial-out) on MISO and '595 (serial-in/parallel-out) on MOSI, sharing SCK.
 * A '165 read clocks the shared SCK and scrambles the '595 shift stage (latched outputs
 * are unaffected), so every transaction is one full-duplex transfer(): pulse LOAD to
 * capture the '165 inputs, shift the complete output frame while receiving the complete
 * input frame, pulse LATCH to publish the '595 outputs. There is no partial read and no
 * partial write.
 *
 * Sketches normally use the pre-defined global instance `ShiftBus1` (the TwoWire/Wire
 * pattern) and perform no setup at all — declaring an SR-backed PinRef is the only opt-in.
 * PinRef::configureAsInput()/configureAsOutput() notify the bus (direction, chain length);
 * PanelGroup::setup() then begin()s every active bus. A bus with no SR pins stays dormant:
 * SPI.begin() is never called.
 *
 * TechSpec: Firmware/ScratchPad/TechSpec/PanelGroup/Helpers/ShiftBus.md (issues #197 / #133).
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <SPI.h>

namespace OpenSkyhawk {

/**
 * @brief One shared SPI shift-register bus ('165 inputs + '595 outputs).
 *
 * @details Constructor stores parameters only — no SPI, no GPIO, no cross-global calls
 * (static-init safe; the PIN_NC lesson). All hardware work happens in begin(), called by
 * PanelGroup::setup() for buses that have at least one configured SR pin.
 */
class ShiftBus {
public:
    static constexpr uint8_t MAX_CHAIN         = 8;   ///< chips per direction (64 in + 64 out)
    static constexpr uint8_t MAX_ISR_CONSUMERS = 16;  ///< sampled encoders per bus

    /**
     * @brief Construct a bus. No hardware activity — begin() runs in PanelGroup::setup().
     * @param spi       SPI peripheral. Dedicated to this bus — the '165 QH output is never
     *                  tristated, so MISO cannot be shared with another SPI reader.
     * @param sckPin    SPI clock pin (must belong to @p spi — e.g. PB3 for SPI1-remap).
     * @param misoPin   SPI MISO pin (← '165 QH).
     * @param mosiPin   SPI MOSI pin (→ '595 DS).
     * @param loadPin   '165 SH/LD̄ strobe (idles HIGH; pulsed LOW to capture inputs).
     * @param latchPin  '595 STCP strobe (pulsed HIGH to publish outputs).
     */
    ShiftBus(SPIClass& spi, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin,
             uint8_t loadPin, uint8_t latchPin);

    // ── PinRef backend — package-internal, mirrors the MCP cache bridge ─────────

    /** @brief configureAsInput() hook: mark active, grow the '165 chain to cover @p chip. */
    void noteInput(uint8_t chip);

    /** @brief configureAsOutput() hook: mark active, grow the '595 chain to cover @p chip. */
    void noteOutput(uint8_t chip);

    /**
     * @brief Cached '165 input bit — no bus traffic.
     * @note With ISR sampling active, a multi-bit consumer reading bit-by-bit can see two
     *       sample instants mixed within one poll (the ISR replaces the whole frame between
     *       reads). Debounced/hold-last consumers (MultiPosInput) absorb this; do not add an
     *       unfiltered multi-bit consumer without considering it.
     */
    bool readBit(uint8_t chip, uint8_t bit) const;

    /** @brief Last written '595 output bit (PinRef::read() on an output pin). */
    bool readOutBit(uint8_t chip, uint8_t bit) const;

    /**
     * @brief Set a '595 stage bit + mark the stage dirty. Published by the next loop-context
     *        transfer()/flushNow() (commit), never mid-group by the sampling ISR.
     *
     * Writes land in a loop-owned stage frame; the ISR ships only the last committed frame.
     * A multi-pin group (StepperMotor's four coils) therefore always reaches the '595
     * outputs as one atomic pattern, regardless of ISR timing between the writes.
     */
    void writeBit(uint8_t chip, uint8_t bit, bool v);

    /** @brief Live '165 read — one transfer(), then the cached bit. */
    bool readLiveBit(uint8_t chip, uint8_t bit);

    /** @brief Pending output changes not yet shifted? */
    bool dirty() const { return _dirty; }

    /** @brief Any SR pin configured on this bus? Dormant buses skip begin() entirely. */
    bool active() const { return _active; }

    // ── Lifecycle — called by PanelGroup ─────────────────────────────────────────

    /**
     * @brief Claim pins + start SPI. Called by PanelGroup::setup() for active buses only.
     *
     * Releases JTAG (SWJ → SWD-only; PB3/PB4 are JTDO/NJTRST) so the SPI1-remap pins are
     * usable, configures LOAD/LATCH strobes, starts SPI, shifts an all-zeros output frame
     * + latch (defined '595 state as early as possible), then runs one transfer() to prime
     * the input cache before the forceReport() boot burst reads it.
     */
    void begin();

    /**
     * @brief One bus transaction: LOAD pulse → full-duplex SPI of the auto-sized frame
     *        (outputs written, inputs captured) → LATCH pulse. Clears the dirty flag.
     *
     * When ISR sampling is active, loop-context callers are wrapped in a short
     * interrupt-masked critical section so the frame buffers stay single-owner.
     */
    void transfer();

    /** @brief transfer() immediately — StepperMotor's per-step flush path. */
    void flushNow() { transfer(); }

    // ── Timer-ISR sampling (encoder-feel mode, -DSHIFTBUS_ISR_HZ) ────────────────

    /**
     * @brief Start a hardware timer that runs transfer() + all registered consumer hooks
     *        every tick. Called by PanelGroup::setup() when SHIFTBUS_ISR_HZ is defined.
     * @param tim       Timer instance (TIM2 by default — TIM3 is backlight PWM).
     * @param sampleHz  Sample rate (e.g. 1000).
     * @note The ISR performs no CAN, no I2C, no allocation. Consumers read cached bits only.
     */
    void beginIsrSampling(TIM_TypeDef* tim, uint16_t sampleHz);

    /**
     * @brief Register a hook called from the sampling ISR after each transfer().
     *        Used by RotaryEncoder::configure() to auto-attach SR-pinned encoders.
     */
    void addIsrConsumer(void (*hook)(void* ctx), void* ctx);

    /** @brief True once beginIsrSampling() has started the timer. */
    bool isrActive() const { return _isrActive; }

#ifdef SHIFTBUS_TEST
    // ── Test seams — chip-less logic tests (no SPI, no strobe pins) ──────────────
    /** @brief Bypass hardware: transfer() copies the injected input frame + counts calls. */
    void testSetBypass(bool on) { _testBypass = on; }
    /** @brief Inject a '165 input byte (delivered to the cache by the next transfer()). */
    void testInjectIn(uint8_t chip, uint8_t value) { if (chip < MAX_CHAIN) _testIn[chip] = value; }
    /** @brief Inspect the committed '595 output frame as last shipped. */
    uint8_t testOutFrame(uint8_t chip) const { return (chip < MAX_CHAIN) ? _outFrame[chip] : 0; }
    /** @brief Inspect the TX wire frame built by the last transfer (reversal + padding). */
    uint8_t testWire(uint8_t i) const { return (i < MAX_CHAIN) ? _wireTx[i] : 0; }
    /** @brief Number of transfer() calls (bypassed or real). */
    uint16_t testTransferCount() const { return _transferCount; }
    /** @brief Auto-sized chain lengths. */
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

/**
 * @brief Pre-defined bus on the standard pins — SPI1-remap SCK=PB3 / MISO=PB4 / MOSI=PB5,
 *        LOAD=PB8, LATCH=PB9 (one contiguous header run with I2C1: PB9..PB3).
 *
 * Override per board with -DSHIFTBUS_SCK=... / _MISO / _MOSI / _LOAD / _LATCH.
 * Dormant (zero cost) unless a sketch declares a PinRef on it.
 */
extern OpenSkyhawk::ShiftBus ShiftBus1;

#endif  // ARDUINO_ARCH_STM32
