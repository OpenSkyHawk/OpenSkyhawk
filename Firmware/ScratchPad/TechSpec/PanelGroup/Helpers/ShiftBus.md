# ShiftBus — Technical Specification

**Status:** Contract agreed (2026-07-02) — implementation pending; bench pending
**FirmwarePlan ref:** issues #197 (74HC165 input backend) + #133 (74HC595 output backend)
**Depends on:** `SPI` (STM32duino), `HardwareTimer` (ISR sampling mode only)

---

## Responsibility

One `ShiftBus` instance owns **one shared SPI shift-register bus**: a chain of 74HC165
(parallel-in/serial-out, inputs) on MISO and a chain of 74HC595 (serial-in/parallel-out, outputs)
on MOSI, both clocked by the same SCK. It is a **single coordinator, not two drivers** — a '165
read clocks the shared SCK, which scrambles the '595 *shift* stage (latched outputs unaffected),
so every '595 latch must be preceded by a full-frame shift. `ShiftBus` makes that discipline
structural: **every bus transaction is one full-duplex `transfer()`** that shifts the complete
output frame out while capturing the complete input frame in, then pulses the '595 latch.
There is no partial read and no partial write.

The library provides a **pre-defined global instance `ShiftBus1`** (class `ShiftBus`, instance
`ShiftBus1` — the `TwoWire`/`Wire` pattern) with the standard pins baked in. **Sketches perform
no setup**: declaring an SR-backed `PinRef` is the only opt-in (see *Zero-setup lifecycle*).

`PinRef` gains an `SR` source, mirroring the MCP23017 contract exactly:

- **Constructor:** `PinRef(ShiftBus1, chip, bit)` — same shape as `PinRef(mcpChip, port, bit)`;
  the first-argument type disambiguates, no tags.
- **Direction at configure:** the consuming class calls `configureAsInput()` → the pin resolves
  to the '165 chain, or `configureAsOutput()` → the '595 chain — exactly as MCP direction is set
  via IODIR at configure time, never in the constructor. `chip` = position within that chain,
  0 = nearest the MCU.
- **Input ('165):** `transfer()` refreshes an input cache; `PinRef::read()` returns a cached bit
  (no bus traffic). Mirrors `PanelGroup::readCachedPin()`.
- **Output ('595):** `PinRef::write()`/`writeDeferred()` set a bit in the output frame and mark it
  dirty; the next `transfer()` pushes it. Mirrors `writeCachedPinDeferred()` + `flushExpanderWrites()`.

Input classes (`Switch2Pos`, `SwitchMultiPos`) and output classes (`LED`, `StepperMotor` via
`PinRef`) consume SR pins **unchanged**. `RotaryEncoder` on SR pins auto-attaches to the bus's
**timer-ISR sampling** when enabled (see below) — loop-side API unchanged, A4EC generator
untouched.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Helpers/ShiftBus/ShiftBus.{h,cpp}
```

Exercised by `Firmware/Tests/ShiftBus/` (bench project for the #197/#133 combined gates).

---

## Electrical / topology contract

### Standard pin assignment (baked into `ShiftBus1`)

| Signal | STM32 pin | Role |
|---|---|---|
| SCK | **PB3** (SPI1 remap) | shared shift clock, ~1 MHz |
| MISO | **PB4** (SPI1 remap) | ← first '165 `QH`; cascade prev `SER`←next `QH` |
| MOSI | **PB5** (SPI1 remap) | → first '595 `DS`; cascade `QH'`→next `DS` |
| LOAD | **PB8** | '165 `SH/LD̄` — idles HIGH (shift mode); pulse LOW = capture parallel inputs |
| LATCH | **PB9** | '595 `STCP` — pulse HIGH = copy shift stage → output pins |

- **LOAD/LATCH are strobes, not chip selects** — shift chains have no CS; chips always listen to
  SCK (hence the full-frame discipline). LOAD bounds the capture before a transfer; LATCH
  publishes after it. Per-sub-panel isolation, if ever needed, = separate LOAD/LATCH pairs on
  the same SCK.
- **The standard assignment makes one contiguous Blue Pill header run**
  `PB9 PB8 PB7 PB6 PB5 PB4 PB3` = LATCH · LOAD · I2C1(SDA/SCL) · MOSI · MISO · SCK — the whole
  digital-expansion cluster on one connector edge. (PB8/PB9 were budgeted as MCP INT pins;
  74HC nodes delete their MCPs, freeing them exactly when ShiftBus is present.)
- **SPI1 remap** because the PanelGroup base spends PA6/PA7 on backlight PWM (TIM3_CH1/CH2):
  firmware releases JTAG (`AFIO` SWJ: JTAG-disable, SWD retained — PB3=JTDO, PB4=NJTRST) before
  configuring the pins. SWD debug (PA13/PA14) unaffected. No conflict with I2C1 (PB6/PB7),
  I2C2 (PB10/PB11), CAN (PA11/PA12), or debug UART1 (PA9/PA10).
- **One SPI bus is enough — permanently.** Chains daisy-chain without limit, so I/O count never
  forces a second bus; write-only SPI peripherals could even share MOSI/SCK with their own latch.
  Only an SPI *reader* peripheral would need SPI2, and none exists or is planned (I²C/ADS1115
  covers analog; no SD card ever). **SPI2 pins (PB13–PB15) stay with I2C INT_B + status LEDs —
  no reservation, no PCB change.**
- **The bus is dedicated to the shift chains.** The '165 `QH` output is never tristated, so MISO
  cannot be shared with another SPI reader. This is also what makes PanelGroup-owned
  `SPI.begin()` safe (see *Zero-setup lifecycle*).
- Per-board deviation is possible via build-flag pin overrides (`-DSHIFTBUS_SCK=...` etc.) or a
  sketch-constructed custom `ShiftBus` instance — expected in ~1% of boards.
- Tie-offs: '165 `CLK_INH`→GND, first '165 `SER`→GND; '595 `MR̄`→VCC, `OĒ`→GND (stance (a),
  #133: boot twitch is cosmetic and current-limited; a GPIO-driven `OĒ` is reserved for future
  critical loads). 100 nF per chip; 33 Ω series on SCK/LOAD/LATCH for remote harness legs;
  10 k external pull-up per used '165 input (no internal pull-ups — use resistor arrays).

### '595 output drive — when a MOSFET is needed

74HC595 outputs are push-pull: ~6 mA per pin recommended, 25 mA absolute max, **70 mA total per
chip** (both rails). Rules:

- **Indicator LED ≤ 4 mA** (project standard is dim, high-value series R): **direct drive +
  series R**, sink or source — no MOSFET.
- **> 6 mA, any 5 V / 12 V load (backlight rails), or chip total approaching 70 mA → MOSFET**
  (2N7002 + 100 k gate pulldown, as the APN MEM-LED circuit).
- DRV8833 inputs are µA-scale logic — never count against the budget.
- Per-panel call at B2 schematic time from each lamp's measured current.

### Chain indexing and bit mapping

`chip` = position along the cascade, **0 = nearest the MCU** ('595: `DS` wired to MOSI;
'165: `QH` wired to MISO). With MSB-first transfers:

- '165: input byte `chip` bit *n* = pin **Dn** of that chip.
- '595: output byte `chip` bit *n* = pin **Qn** of that chip.

The bus transmits output bytes in reverse chain order so `outFrame[chip]` lands in `chip`,
and receives input bytes in chain order so `inFrame[chip]` comes from `chip`. Sketches never
see wire order — only (`chip`, `bit`).

### Topology & chaining guidance

- **No required chain order.** Chain order is whatever PCB/harness routing wants; the wiring map
  *describes* the physical order afterward (`chip` = cascade position). Run the cascade in panel
  order (left→right across a gauge row) so chip index correlates with physical position.
- **Full-frame latch = whole-bus atomicity.** The '595 LATCH updates every output on every chip
  simultaneously, so the MCP-era "coils must own their port" rule does **not** apply — a
  stepper's coils may straddle a chip boundary freely. Pack bits however routing likes.
- **Per-gauge bit template (convention, not correctness):** '165 `D0=encA, D1=encB, D2=btn`
  (2 gauges/chip); '595 `Q0–Q3=coils, Q4=nSLEEP` (1 gauge/chip, Q5–Q7 spare) — wiring maps then
  write themselves mechanically. Spare bits cost ~$0.07/chip; clarity wins.
- **Chip placement — hub vs sub-panel (guidance only; per-controller call at B2):**
  - *Hot-swappable / serviceable peripherals (gauge clusters)* lean toward chips on the **hub
    board**, star wiring — gauge connectors carry only dumb signals (coils, encoder lines,
    button, power). Unplugging any gauge leaves the chain intact: no index shift, no reflash,
    other gauges unaffected; the absent gauge's inputs are held by pull-ups (no phantom events).
  - *Fixed remote sub-panels (ARC-51 over ~12″)* lean toward chips on the **sub-panel**, bus
    wiring (5-wire pass-through, 33 Ω series R).
  - Trade-off to weigh: chips-on-peripheral chains break when a link is unplugged — chips beyond
    the gap latch floating-input noise ('595) or read garbage ('165); a bypass jumper closes the
    chain but shifts every downstream chip index (reflash with edited indices). Fine for planned
    reconfiguration; weigh against hub topology where casual unplugging is expected.

---

## Zero-setup lifecycle

Sketch usage, 99% case — **declaring the PinRef is the entire opt-in**:

```cpp
const PinRef PIN_SEL_0   = PinRef(ShiftBus1, 0, 3);  // '165 chip 0, D3 (direction set by consumer)
const PinRef PIN_MEM_LED = PinRef(ShiftBus1, 0, 5);  // '595 chip 0, Q5

OpenSkyhawk::Switch2Pos sel(DCSIN_..., PIN_SEL_0);   // configureAsInput → '165 chain
OpenSkyhawk::LED memLed(A_4E_C_..., PIN_MEM_LED);    // configureAsOutput → '595 chain
// no registration, no begin(), no chain counts — nothing else in the sketch
```

Lifecycle:

1. **Static init:** constructors do nothing. `PinRef` stores `&ShiftBus1` (address of a global is
   link-time constant — safe before construction; same as MCP PinRefs today). `ShiftBus`'s own
   constructor touches no SPI, no GPIO. This is the PIN_NC lesson (constexpr ctor, 0758111)
   applied: **no cross-global calls during static init — no init-order race.**
2. **`PanelGroup::setup()` step 3 — configure:** each SR PinRef's `configureAsInput()`/
   `configureAsOutput()` notifies its bus: sets the pin's direction flag, marks the bus **active**,
   and grows the per-chain size to `max(chip)+1`. PanelGroup collects unique bus pointers (the
   global + any custom instances) — no explicit registration.
3. **After step 3:** for each active bus, PanelGroup calls `begin()`: claim pins, JTAG-release +
   SPI init (MODE0, MSBFIRST, ~1 MHz), shift an all-zeros output frame + latch (defined '595
   state), one `transfer()` to prime the input cache. **A bus with no SR pins stays dormant —
   `SPI.begin()` is never called, zero cost.** This preserves the "only start buses in use" rule
   that keeps `Wire.begin()` sketch-owned — but here PanelGroup can own the begin, because the
   MISO-dedication rule means no other library ever shares this bus (the Wire limitation was a
   shared-resource problem; the shift bus is structurally un-shared).
4. **`PanelGroup::loop()` step 1** (with the MCP cache refresh): `transfer()` once per iteration —
   refreshes the input cache for the InputBase polls *and* pushes output bits dirtied in the
   previous iteration. Loop-rate polling is the base mode; ISR sampling is opt-in on top.
5. **`flushExpanderWrites()`** also flushes a dirty ShiftBus — `StepperMotor`'s batched path
   (4× `writeDeferred()` + one flush per step) works on SR pins unchanged: one SPI burst per step
   instead of one I2C `writePort()`. Expected motor-limited (~600 °/s vs ~490 steps/s on
   MCP@400 kHz — bench gate 2).

Transfers cover only the chips actually declared (auto-sized), so typical nodes move 2–3 bytes
(~25 µs) despite `MAX_CHAIN = 8`.

---

## Public API

```cpp
// Helpers/ShiftBus/ShiftBus.h  (inside #ifdef ARDUINO_ARCH_STM32)
namespace OpenSkyhawk {

class ShiftBus {
public:
    static constexpr uint8_t MAX_CHAIN = 8;  // per direction: 64 inputs + 64 outputs per bus

    /** No SPI or GPIO activity here — begin() runs in PanelGroup::setup().
     *  SPI pins are per-instance (a custom bus on SPI2 passes its own) — only ShiftBus1
     *  bakes in the SHIFTBUS_* standard pins. */
    ShiftBus(SPIClass& spi, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin,
             uint8_t loadPin, uint8_t latchPin);

    void begin();      // JTAG-release + remap/claim pins, SPI init, zero-frame + latch,
                       // prime input cache. Called by PanelGroup for active buses only.

    void transfer();   // ONE bus transaction: pulse LOAD → full-duplex SPI of
                       // max(nIn, nOut) auto-sized bytes → pulse LATCH. Clears dirty.

    // — PinRef backend (package-internal, mirrors the MCP cache bridge) —
    void  noteInput(uint8_t chip);                         // configureAsInput → mark active,
    void  noteOutput(uint8_t chip);                        //   grow chain size (setup only)
    bool  readBit(uint8_t chip, uint8_t bit) const;        // cached, no bus traffic
    void  writeBit(uint8_t chip, uint8_t bit, bool v);     // set frame bit + mark dirty
    bool  dirty() const;                                   // pending output changes?
    bool  active() const;                                  // any SR pin configured?
    void  flushNow();                                      // transfer() immediately —
                                                           // StepperMotor per-step path
    bool  readLiveBit(uint8_t chip, uint8_t bit);          // transfer() then read

    // — timer-ISR sampling (encoder-feel mode; enabled via -DSHIFTBUS_ISR_HZ) —
    void beginIsrSampling(TIM_TypeDef* tim, uint16_t sampleHz);
    void addIsrConsumer(void (*hook)(void* ctx), void* ctx);  // called after each ISR transfer
};

extern ShiftBus ShiftBus1;  // pre-defined: SPI1-remap PB3/PB4/PB5, LOAD=PB8, LATCH=PB9

}  // namespace OpenSkyhawk
```

### PinRef extension

`Type::SR`; union member `struct { ShiftBus* bus; uint8_t chip; uint8_t bit; bool isOutput; } sr;`
(`isOutput` set during configure — a '595 bit never configured stays 0 on the wire, harmless).

| PinRef op | SR input ('165) | SR output ('595) |
|---|---|---|
| `configureAsInput()` | `noteInput(chip)` — binds to the '165 chain | — |
| `configureAsOutput()` | — | `noteOutput(chip)` — binds to the '595 chain |
| `read()` | cached frame bit | last written bit |
| `readLive()` | `readLiveBit()` — one transfer | last written bit |
| `write(v)` / `writeDeferred(v)` | no-op (+debug log) | `writeBit()`; flushed next `transfer()` — SR writes are inherently deferred |
| `readAnalog()` / `writeAnalog()` | 0 / no-op + `PINREF_DEBUG` log via STM32Board diag (same pattern as MCP `readAnalog` today) | same |

A compile-time error for `readAnalog()` on SR pins is not possible — `PinRef` is one
runtime-dispatched type (tagged union), the source is unknown at compile time. Guard is
two-layer instead: the silent no-op + diag log above, **plus construct-time rejection in analog
consumer classes** (`AnalogInput` refuses non-analog-capable PinRefs — the `AnalogOutput`
`isGpio()` precedent; feeds the sketch-validation theme from the 2026-07 review).

### Build flags

| Flag | Default | Meaning |
|---|---|---|
| `SHIFTBUS_SCK/MISO/MOSI/LOAD/LATCH` | PB3/PB4/PB5/PB8/PB9 | per-board pin override (rare) |
| `SHIFTBUS_ISR_HZ` | off (loop-poll) | enable timer-ISR sampling at this rate (e.g. 1000) |
| `SHIFTBUS_ISR_TIM` | TIM2 | sampling timer override (tone=TIM3, servo=TIM4, backlight=TIM3) |

NODE_ID-style: per-node variance lives in `platformio.ini`, the sketch stays wiring-only.

---

## Timer-ISR sampling — encoder-feel mode (#197 gate 6)

**Problem.** Quadrature decode must see every A/B transition. `RotaryEncoder::poll()` samples at
loop rate; an OLED `sendBuffer` flush blocks the loop 20–30 ms, and a fast hand-spin (EC11,
24 PPR ≈ 96 edges/rev, ~3 rev/s burst ≈ 3.5 ms/edge) loses 6–8 edges per flush. A faster *cache*
alone does not help — the cache is 1-deep, so a stalled loop still sees only the latest state.
**Decode must consume every sample.**

**Severity: feel, not desync.** No encoder in the build drives a physical indicator — REL knobs
(ASN-41) and DIR selectors (ARC-51) read back through *sim-driven* displays (OLED drums / DCS),
so the human closes the loop and a missed count is one extra click, never a permanent
physical-vs-sim offset. The cost of loop-poll is responsiveness: dead clicks and occasional
reverse-blips clustered during OLED flushes — which on ASN-41 coincide with cranking, since the
drums repaint as values are entered. ISR sampling is therefore **cheap insurance for feel**, not
a correctness requirement; gate 6 decides whether loop-poll feel is acceptable as-shipped.

**Design: decode in the ISR, emit in the loop.**

- `-DSHIFTBUS_ISR_HZ=1000` starts a HardwareTimer. Each tick: `transfer()` (~25 µs — SPI-in-ISR
  is clean, unlike I²C; a timer ISR preempting a blocking `Wire` wait is safe, SPI is a separate
  peripheral) then each registered consumer hook.
- **Layering: input classes know nothing about ShiftBus.** `InputBase` gains a generic
  high-rate hook `virtual void sampleTick() {}` (ISR-safe contract: cached reads only, no
  CAN/I²C). **PanelGroup owns the wiring**: at setup step 3b it registers one bus-ISR consumer
  that walks the input list calling `sampleTick()`. `RotaryEncoder` overrides the hook —
  decode one sample, accumulate **pending detents** (`volatile int8_t`) — without knowing who
  samples it, at what rate, or from where. No sketch code, no constructor change,
  **A4EC generator output untouched**.
- `poll()` (loop context, API unchanged) atomically drains pending detents and emits the CAN
  EVTs exactly as today (REL coalesced ±n×step / DIR ±1 per detent). CAN traffic never
  originates in the ISR. The first `sampleTick()` latches sampler ownership — `poll()` stops
  decoding and only drains.
- Level-sampled classes (`Switch2Pos`, `SwitchMultiPos`) keep the default no-op hook — the
  1-deep cache is correct for level inputs, and ISR refreshes only make it fresher.

**Sample rate — what actually sets it.** The UART leg to the sim runs at ms cadence, but that
bounds *latency*, not sampling. Sampling exists to not *lose counts*; sampling faster changes
nothing downstream — emits stay one-per-detent, CAN/UART traffic unchanged; the only cost is CPU
(1 kHz × ~25 µs ≈ 2.5 %).

- *Average* edge rate is mild: fast hand-spin ≈ 290 edges/s ⇒ ~3.5 ms/edge.
- *Worst case* is the detent snap: the spring slams the wiper through the 2–4 intra-detent
  edges in a ~1 ms-class burst, at any hand speed. Direction information lives **only** in
  those intermediate states (rest states repeat), so the snap, not the spin rate, sets the floor.

Default **1 kHz** (agreed); gate 6 sweeps **250 / 500 / 1000 Hz** under the artificial loop
stall to confirm the floor empirically.

**Concurrency rules:**
- ISR and loop share the frame buffers → loop-context `transfer()`/`flushNow()` run inside a
  timer-IRQ-masked critical section (µs-scale; bounded, no CAN/I²C inside).
- **Stage/commit output frames:** `writeBit()` targets a loop-owned *stage*; loop-context
  transfers *commit* stage → wire frame; the ISR ships only the last committed frame. A
  multi-pin group (stepper's four coils, staged across four calls) therefore always reaches
  the '595 outputs atomically — the ISR can never latch a half-written group.
- Pending-detent counters are `volatile`; drain is read-and-clear with IRQs masked. DIR-mode
  pending is clamped ±8 (one CAN frame per DIR detent — an absurd backlog would flood the
  16-slot TX ring); REL coalesces, so it accumulates freely.
- The ISR performs **no CAN, no I²C, no allocation**.

---

## Health / diagnostics — deferred

SPI shift chains have no ACK: a dead or absent '165 reads garbage, a dead '595 silently drives
nothing — there is no I2cHealth-style breaker equivalent. A loopback self-test exists (last '595
`QH'` → a spare '165 input; shift a test pattern, verify it emerges — validates both chains
end-to-end) but requires **both** chip families present, which not every node will have.
**Deferred** — noted as a future diagnostics hook for node health reporting (#163).

---

## Grouping guidance (feeds panel-pipeline)

| Panel I/O profile | Backend |
|---|---|
| All-input dense (ARC-51: 16 in), encoder-heavy (ASN-41: 7 encoders) | '165 bus (+ISR sampling for encoders) |
| Mixed in/out, stepper coils (APN-153) | '165 + '595 on one bus — replaces the MCP |
| Sparse I/O, no encoders, I²C already present | MCP23017 remains fine |
| **Mixed backends on one node** (GPIO + MCP + ShiftBus — the PinRef contract) | fully supported; caches all refresh before input polls; one pin-budget rule: MCP INT lines move to PB12/PB13 (PB3–PB5/PB8/PB9 are the bus) |
| Remote sub-panel leg | SPI tolerates ~12″ with 33 Ω series R; I²C legs then carry only OLEDs at 100 kHz |

---

## Bench mapping (#197 combined-gate coverage)

| Gate | What | ShiftBus path exercised |
|---|---|---|
| 1 | simulated multipos (jumper-to-GND on D0–D4) → '165 → `SwitchMultiPos` | cached read + one-hot decode |
| 2 | '595 → DRV8833 → X27 6-state | `flushNow()` per step; rate vs MCP |
| 3 | '595 → LED **direct drive** (series R; MOSFET variant optional) | `LED` on an SR output pin |
| 4 | '595 boot twitch (OĒ=GND) | `begin()` zero-frame timing |
| 5 | shared-bus non-interference | full-frame transfer discipline |
| 6 | EC11 fast-spin, zero missed counts | loop-poll vs ISR @250/500/1000 Hz — **with a ~25 ms artificial loop stall** (or a live OLED flush) to reproduce the real failure mode |
| 7 | pull-up count/layout accepted | hardware only |

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| SPI | STM32duino core | SPI1 remap PB3/PB4/PB5; MODE0, MSBFIRST, ~1 MHz |
| HardwareTimer | STM32duino core | ISR sampling mode only; pick a timer unused by PWM zones (TIM3 is backlight) |
| PinRef / PanelGroup | this library | cache-bridge pattern reused |

---

## Agreed decisions (2026-07-02 review)

1. Name **`ShiftBus`**, global instance **`ShiftBus1`**, lives in `Helpers/ShiftBus/`.
2. `MAX_CHAIN = 8` per direction.
3. Contract mirrors MCP: `PinRef(bus, chip, bit)`, direction at configure — no SrIn/SrOut tags.
4. Zero-setup lifecycle (auto-detect at configure); build flags only for pin overrides + ISR rate.
5. Standard pins PB3/PB4/PB5 + LOAD=PB8/LATCH=PB9 (contiguous header run with I2C1).
6. SPI2 never reserved — status LEDs stay on PB14/PB15; no PCB change; no RightNav impact.
7. ISR default 1 kHz when enabled; ships off pending gate 6.
8. A4EC generator untouched (encoder ISR auto-attach).
9. `readAnalog()` on SR: runtime no-op + diag log + `AnalogInput` construct-time rejection
   (compile-time error impossible with a tagged-union PinRef).
10. LED drive: direct from '595 for ≤4 mA indicators; MOSFET above 6 mA / rail loads / chip-total
    limits — per-panel call at B2.
