# PanelGroup — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md`, `FirmwarePlan/09-startup-resync-diagnostics.md`, `FirmwarePlan/02-can-protocol.md`
**Depends on:** `STM32Board.md`, `CANProtocol.md`, `PinRef.md`

---

## Responsibility

STM32 firmware library for all cockpit panel nodes. Owns: MCP23017 expander registration and
management (interrupt configuration, INTCAP reads, port state cache, polling fallback); CAN EVT
dispatch of input events via `CANProtocol::sendBatched()`; reception and distribution of
`CTRL_BCAST` output packets to registered output objects; `SYNC_REQ` response (re-poll all
inputs); `READY_n` frame on boot completion; and the 500 ms `HB_n` heartbeat timer.
`TEST_SEQ` echo is owned by `CANProtocol::drain()` and is not forwarded to PanelGroup
callbacks.

Does **not** run DCS-BIOS. Does **not** interpret DCS-BIOS addresses or control names. Does
**not** own CAN frame IDs (CANProtocol). Does **not** handle UART or USB CDC (PanelBridge /
SimGateway). Has no knowledge of how PanelBridge routes `controlId` values — it emits
`ControlPacket` structs and lets PanelBridge decide.

Platform: **STM32 only** (STM32F103Cx — C8T6 64 KB or CBT6 128 KB). Not used on RP2040.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
├── PanelGroup.h    ← PanelGroup namespace, InputBase, OutputBase
├── PanelGroup.cpp  ← expander management, interrupt dispatch, CAN RX, heartbeat
├── PinRef.h        ← PinRef class (specified in PinRef.md)
├── PinRef.cpp      ← PinRef backend dispatch (specified in PinRef.md)
└── library.json    ← platform: ststm32
```

Individual input and output class files (Switch2Pos.h / .cpp, LED.h / .cpp, etc.) also live
in this directory and are covered by their own TechSpec files under `PanelGroup/Inputs/` and
`PanelGroup/Outputs/`.

### library.json

```json
{
  "name": "PanelGroup",
  "version": "0.1.0",
  "frameworks": "arduino",
  "platforms": "ststm32",
  "dependencies": {
    "blemasle/MCP23017": "^2.0.0",
    "adafruit/Adafruit ADS1X15": "^2.0.0"
  }
}
```

### Sketch layout

```
Firmware/<PanelName>/
├── platformio.ini
└── src/
    └── main.cpp    ← expander instances, wiring map, input/output declarations,
                       setup() + loop()
```

### Test project

```
Firmware/Tests/PanelGroup/
├── platformio.ini
└── tests/
    ├── boot_sequence/
    │   └── boot_sequence.cpp    — setup order: Wire.begin before setup(); expanders inited;
    │                              baseline port read; interrupts attached; CANProtocol started;
    │                              EVT burst emitted; READY_n sent
    ├── interrupt_dispatch/
    │   └── interrupt_dispatch.cpp — interrupt flag set by ISR; loop() reads INTCAP; port cache
    │                                updated; InputBase::poll() called on inputs referencing
    │                                changed bits; unchanged inputs not polled; polling fallback
    │                                at ~20 ms when registerExpander(chip) (no-pin overload) used
    ├── sync_req/
    │   └── sync_req.cpp         — SYNC_REQ received in loop(); all inputs re-polled; EVTs emitted;
    │                              flushBatched(canIdEvt(NODE_ID)) called before returning to normal loop
    ├── heartbeat/
    │   └── heartbeat.cpp        — HB_n sent at 500 ms intervals; payload fields set correctly;
    │                              no HB before first 500 ms after setup()
    ├── ctrl_bcast/
    │   └── ctrl_bcast.cpp       — CTRL_BCAST received; DLC guard rejects short frames; each non-null
    │                              ControlPacket dispatched to all OutputBase objects; null slot skipped
    └── can_drain/
        └── can_drain.cpp        — loop() calls CANProtocol::drain(); TEST_SEQ echo remains
                                   CANProtocol-owned
```

Tests use fakes for `CANProtocol` send paths, `STM32Board`, and a mock `MCP23017` shim that
exposes controllable `readPort()` / `getLastInterruptPin()` return values. No physical I2C bus
or CAN hardware required.

**PanelGroup's reconnect/disconnect contract** (verified in `PanelBridge/tests/dual_integration/`
rather than in a PanelGroup-only test, because it requires two boards on a live CAN bus):

- PanelGroup boots in any order relative to PanelBridge — see `FirmwarePlan/09` Timelines A and B.
- PanelGroup sends initial EVT burst + READY frame once on every boot, before the heartbeat timer starts.
- PanelGroup responds to SYNC_REQ at any time after boot by re-polling all registered inputs and
  flushing all pending EVT batches via `flushBatched(canIdEvt(NODE_ID))`.
- PanelGroup does not need to know whether PanelBridge is alive — it simply broadcasts on the CAN bus
  and trusts that PanelBridge will re-request via SYNC_REQ whenever it reconnects.
- Heartbeat continues regardless of whether PanelBridge is alive. PanelBridge's watchdog detects
  dropout when HB is missing for 3 s and triggers recovery SYNC_REQ on reconnect (Timeline C).

**`platformio.ini`:**

```ini
[platformio]
src_dir = tests

[env_base]
platform = ststm32
board = genericSTM32F103C8
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

[env:test_boot_sequence]
extends = env_base
build_src_filter = -<*> +<boot_sequence/boot_sequence.cpp>

[env:test_interrupt_dispatch]
extends = env_base
build_src_filter = -<*> +<interrupt_dispatch/interrupt_dispatch.cpp>

[env:test_sync_req]
extends = env_base
build_src_filter = -<*> +<sync_req/sync_req.cpp>

[env:test_heartbeat]
extends = env_base
build_src_filter = -<*> +<heartbeat/heartbeat.cpp>

[env:test_ctrl_bcast]
extends = env_base
build_src_filter = -<*> +<ctrl_bcast/ctrl_bcast.cpp>

[env:test_can_drain]
extends = env_base
build_src_filter = -<*> +<can_drain/can_drain.cpp>
```

---

## Public API

```cpp
// PanelGroup.h

#pragma once
#include <Arduino.h>
#include <MCP23017.h>  // blemasle/MCP23017
#include "ADS1115.h"   // thin wrapper over Adafruit_ADS1X15; see ADS1115.h
#include <PinRef.h>

// ── Internal base classes ─────────────────────────────────────────────────────
// Defined here so both PanelGroup.cpp and all input/output class files share the
// same declaration without a separate internal header.

namespace OpenSkyhawk {

/**
 * @brief Abstract base for all input objects. Self-registers into a static linked list
 *        at construction.
 */
class InputBase {
public:
    /**
     * @brief Configure hardware pins for this input. Called by PanelGroup::setup()
     *        after all chip init() calls, before the boot EVT burst.
     *
     * Implementations call _pin.configureAsInput() (or equivalent per pin). This is
     * the correct place for pin mode setup — MCP23017 register writes require
     * chip.init() to have run first, which rules out the constructor.
     *
     * Default is a no-op. Override in every input class that owns a PinRef.
     */
    virtual void configure() {}

    /**
     * @brief Read current hardware state and emit a CAN EVT if state changed.
     *
     * Called by PanelGroup::loop() during normal operation. Implementation reads from
     * PinRef (GPIO, MCP23017 cache, or ADS1115), applies debounce or filtering, and
     * calls CANProtocol::sendBatched() only if a confirmed state change occurred.
     *
     * No-op if forceReport() has not yet been called (state is uninitialised).
     */
    virtual void poll() = 0;

    /**
     * @brief Read current hardware state and emit a CAN EVT unconditionally.
     *
     * Called by PanelGroup during the boot EVT burst (setup()) and on every SYNC_REQ.
     * Bypasses debounce — hardware is assumed stable at these call sites. Confirms the
     * current reading as the new last-known state so subsequent poll() calls use it as
     * the baseline. Also initialises internal state so poll() becomes active.
     */
    virtual void forceReport() = 0;

    static InputBase* head();  ///< Head of the registered input linked list.
    InputBase*        next() const;  ///< Next input; nullptr at end.

protected:
    InputBase();  ///< Registers this instance into the linked list.

private:
    static InputBase* _head;
    InputBase*        _next;
};

/**
 * @brief Abstract base for all output objects. Self-registers into a static linked list.
 *        PanelGroup::loop() walks the list for each received CTRL_BCAST ControlPacket.
 */
class OutputBase {
public:
    /**
     * @brief Configure hardware pins for this output. Called by PanelGroup::setup()
     *        after all chip init() calls, before the boot EVT burst.
     *
     * Implementations call _pin.configureAsOutput(). Default is a no-op.
     * Override in every output class that owns a PinRef.
     */
    virtual void configure() {}

    /**
     * @brief Called for every non-null ControlPacket in a CTRL_BCAST frame.
     *        Implementation checks controlId against its own address and applies the value
     *        if it matches. No-op on mismatch.
     * @param controlId  DCS-BIOS output address.
     * @param value      16-bit value from DCS-BIOS.
     */
    virtual void onControlPacket(uint16_t controlId, uint16_t value) = 0;

    /**
     * @brief Called every PanelGroup::loop() iteration.
     *        Default no-op. Override for stepper motor step generation (SwitecX25Output,
     *        AccelStepperOutput).
     */
    virtual void update() {}

    /**
     * @brief Node-health fault code for this output (#163). 0 = healthy; a NodeFaultId
     *        (CANProtocol.h) otherwise. Rolled up into the HEALTH_n frame's faultId.
     *        Default: never faults. Must be const + cheap (cached state, no I2C op).
     */
    virtual uint8_t faultCode() const { return 0; }

    /**
     * @brief Human fault detail for the local DiagSerial tap only (#163) — never on the
     *        wire. "" = none. Overridden by I2C-backed outputs (e.g. DrumDisplay) to name
     *        the failing hop; GPIO outputs inherit the default.
     */
    virtual const char* faultDetail() const { return ""; }

    static OutputBase* head();  ///< Head of the registered output linked list.
    OutputBase*        next() const;  ///< Next output; nullptr at end.

protected:
    OutputBase();  ///< Registers this instance into the linked list.

private:
    static OutputBase* _head;
    OutputBase*        _next;
};

} // namespace OpenSkyhawk

// ── PanelGroup namespace — sketch-facing API ──────────────────────────────────

namespace PanelGroup {

    /**
     * @brief Register an ADS1115 ADC. Must be called before setup().
     *
     * PanelGroup calls adc.begin() during setup(). Call once per ADS1115 instance —
     * multiple AnalogInput objects may share the same ADS1115, but it must be registered
     * only once so begin() is not called redundantly.
     *
     * @param adc   ADS1115 instance. Must outlive PanelGroup.
     * @param addr  I2C address (0x48–0x4B via ADDR pin). Default 0x48.
     * @param wire  I2C bus. Default Wire (I2C1 on STM32).
     *
     * @note Wire.begin() / Wire1.begin() must be called by the sketch before setup().
     */
    void registerADC(ADS1115& adc, uint8_t addr = 0x48, TwoWire& wire = Wire);

    /**
     * @brief Register an MCP23017 expander. Must be called before setup().
     *
     * PanelGroup stores the chip reference and interrupt pin assignments. During setup(),
     * PanelGroup initialises the chip, reads baseline port state, and calls
     * attachInterrupt() on the specified STM32 pins.
     *
     * If intaPin == intbPin, MIRROR mode is used: IOCON.MIRROR is set so either port
     * change fires the single shared line.
     *
     * If two or more chips share the same intaPin / intbPin value (wired-OR), PanelGroup
     * sets IOCON.ODR = 1 on each chip on that shared line so outputs are open-drain.
     *
     * @param chip     blemasle/MCP23017 instance. Must outlive PanelGroup.
     * @param intaPin  STM32 GPIO pin connected to chip INTA (PB3, PA4, etc.).
     * @param intbPin  STM32 GPIO pin connected to chip INTB. Pass same as intaPin for
     *                 MIRROR mode.
     *
     * @note Do not use PB14/PB15 (reserved for status LED) or PC13/PC14/PC15
     *       (RTC/tamper/oscillator quirks). See FirmwarePlan/05 for full constraint list.
     */
    void registerExpander(MCP23017& chip, uint8_t intaPin, uint8_t intbPin);

    /**
     * @brief Initialise PanelGroup. Call from sketch setup().
     *
     * Performs the 8-step PanelGroup boot sequence:
     *   1. STM32Board::begin() — status LED, DiagSerial, CAN HAL.
     *   2. For each registered expander: init chip, read baseline port state into cache,
     *      configure IOCON (MIRROR / open-drain as needed), call attachInterrupt().
     *   3. Register CAN callbacks.
     *   4. CANProtocol::start(). Mandatory CTRL_BCAST, TEST_SEQ, and SYNC_REQ filters
     *      are included by CANProtocol automatically.
     *   5. Poll all registered InputBase objects; submit EVTs via
     *      CANProtocol::sendBatched(canIdEvt(NODE_ID), ...).
     *   6. CANProtocol::flushBatched(canIdEvt(NODE_ID)) — flush any trailing odd packet
     *      before READY.
     *   7. Send READY_n frame (canIdReady(NODE_ID), DLC=0).
     *   8. Arm heartbeat timer (first HB_n 500 ms after boot).
     *
     * @note Wire.begin() / Wire1.begin() must be called by the sketch before setup().
     *       PanelGroup never calls begin() on any I2C bus. Only start buses in use.
     */
    void setup();

    /**
     * @brief Run PanelGroup work. Call once per loop() iteration.
     *
     * Each call:
     *   1. Check all registered interrupt pins. For each flagged pin: iterate all chips
     *      on that pin, read INTFLAG_A/INTFLAG_B. For each chip with non-zero flag, read
     *      INTCAP to get port state at interrupt time; update port cache.
     *   2. Polling fallback: every ~20 ms, read actual port state for any chip that has
     *      no interrupt pin registered; update port cache.
     *   3. Call poll() on all registered InputBase objects — unconditionally, every
     *      iteration. Each poll() reads from its PinRef (which reads from the cache for
     *      MCP23017 pins, or directly from hardware for GPIO/ADS1115), applies its own
     *      debounce or filtering logic, and calls CANProtocol::sendBatched() if a new
     *      value is ready to emit.
     *      Digital inputs (Switch2Pos, etc.) use wall-clock debounce timers that only
     *      fire after 20 ms of stable signal — calling them every loop iteration is
     *      harmless and required for accurate timing.
     *      Analog inputs (AnalogInput, AnalogMultiPos, AngleSensorInput) manage their
     *      own 8 ms poll timer inside poll() — PanelGroup always calls them; they decide
     *      whether enough time has passed to take a new reading.
     *   4. Drain CANProtocol RX:
     *        0x010 CTRL_BCAST  → walk OutputBase list; call onControlPacket() for each
     *                            non-null slot (controlId != 0x0000).
     *        0x012 SYNC_REQ   → call forceReport() on all InputBase objects; call
     *                            flushBatched(canIdEvt(NODE_ID)).
     *        0x011 TEST_SEQ   → handled internally by CANProtocol::drain(); PanelGroup
     *                            does not receive this callback.
     *        anything else    → ignored by PanelGroup's receive callback.
     *   5. Call update() on all OutputBase objects.
     *   6. Check heartbeat timer; send HB_n (canIdHb(NODE_ID)) every 500 ms.
     */
    void loop();

    // ── Package-internal — MCP23017 cache bridge for PinRef ──────────────────
    //
    // The sketch never calls these functions. They are called exclusively by
    // PinRef::read() and PinRef::write() inside PinRef.cpp, which input and output
    // classes invoke through their stored PinRef members:
    //
    //   Input class ::poll()         → pinRef.read()  → readCachedPin()
    //   Output class ::onControlPacket() → pinRef.write() → writeCachedPin()
    //
    // Direct STM32 GPIO and ADS1115 PinRefs do not use this path — they call
    // digitalRead() / adc.readADC_SingleEnded() directly and never touch the cache.
    //
    // MCP23017 pins require a cache because:
    //   • A live I2C readPort() on every poll() call is slow (~100 µs at 400 kHz).
    //   • The interrupt path reads INTCAP, capturing port state at the exact moment
    //     the pin changed. A subsequent live read may return an already-transitioned
    //     value, bypassing the interrupt capture entirely.
    //   • All poll() calls after an interrupt must see the same captured snapshot.
    //
    // readCachedPin() returns the bit from the ExpanderEntry cache last updated by
    // INTCAP (interrupt path) or a full port read (polling fallback / baseline).
    // No I2C occurs inside this call.
    //
    // writeCachedPin() writes the pin via I2C and updates the cache so that a
    // subsequent readCachedPin() on the same bit returns the written value rather
    // than stale interrupt-captured state.

    /**
     * @brief Read a cached MCP23017 pin state. Called by PinRef::read(). Not sketch API.
     * @param chip  Chip reference — used as key to locate the ExpanderEntry.
     * @param port  PORT_A (0) or PORT_B (1).
     * @param bit   Bit index 0–7.
     * @return      Cached logical level (true = HIGH). No I2C transaction.
     */
    bool readCachedPin(const MCP23017& chip, uint8_t port, uint8_t bit);

    /**
     * @brief Write an MCP23017 pin and update the cache. Called by PinRef::write(). Not sketch API.
     * @param chip   Chip reference.
     * @param port   PORT_A (0) or PORT_B (1).
     * @param bit    Bit index 0–7.
     * @param value  Logical level to write.
     */
    void writeCachedPin(MCP23017& chip, uint8_t port, uint8_t bit, bool value);

} // namespace PanelGroup
```

---

## Sketch Contract

```cpp
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <Inputs/Switch2Pos/Switch2Pos.h>
#include <Outputs/LED/LED.h>
#include <A4EC_CmdIds.h>    // DCSIN_* constants for DCS-BIOS inputs
#include <A4EC_OutputIds.h> // A_4E_C_* output address/mask constants
#include <HIDControls.h>    // CTRL_* constants for HID inputs (if any on this node)

// ── Expanders ─────────────────────────────────────────────────────────────────
MCP23017 exp1(0x20, Wire);

// ── Wiring map — one named constant per physical connection ───────────────────
// Name matches schematic net label. No inline literals below this section.
const PinRef PIN_MASTER_ARM  = PinRef(exp1, PORT_A, 3);
const PinRef PIN_CAUTION_LED = PinRef(PB0);

// ── Inputs ────────────────────────────────────────────────────────────────────
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER,  PIN_MASTER_ARM);

// ── Outputs ───────────────────────────────────────────────────────────────────
OpenSkyhawk::LED masterCaution(A_4E_C_MASTER_CAUTION_A, 0x4000, PIN_CAUTION_LED);

void setup() {
    Wire.begin();                                       // I2C buses before PanelGroup
    PanelGroup::registerExpander(exp1, PB12, PB13);    // INTA→PB12, INTB→PB13
    PanelGroup::setup();                                // boot sequence steps 1–8
}

void loop() {
    PanelGroup::loop();
}
```

`PanelGroup::setup()` calls `STM32Board::begin()` internally — the sketch does not call it.
All input and output objects are declared at global scope so they are constructed (and
self-registered) before `setup()` runs.

---

## Key Data Structures

### ADCEntry

```cpp
struct ADCEntry {
    ADS1115*  adc;  ///< Pointer to the registered ADS1115 instance.
    uint8_t   addr; ///< I2C address (0x48–0x4B), passed to adc->begin() in setup().
    TwoWire*  wire; ///< I2C bus pointer, passed to adc->begin() in setup().
};
```

Maximum registered ADCs: 8 (four per I2C bus × two buses). Storage is a fixed array; no heap
allocation. `registerADC()` stores the pointer and deduplicates — registering the same instance
twice is a no-op.

### ExpanderEntry

```cpp
struct ExpanderEntry {
    MCP23017* chip;          ///< Pointer to the registered MCP23017 instance.
    uint8_t   intaPin;       ///< STM32 GPIO pin for INTA.
    uint8_t   intbPin;       ///< STM32 GPIO pin for INTB (== intaPin if MIRROR mode).
    bool      mirrored;      ///< IOCON.MIRROR set — intaPin == intbPin at registration.
    bool      openDrain;     ///< IOCON.ODR set — shared line with other chips.
    uint8_t   portAcache;    ///< Last-known state of GPA0–GPA7.
    uint8_t   portBcache;    ///< Last-known state of GPB0–GPB7.
};
```

Maximum registered expanders: 8 (one full I2C address space per bus). Storage is a fixed
array sized `MAX_EXPANDERS` (default 8); no heap allocation.

### Interrupt Flags

```cpp
volatile bool _intFlags[MAX_INT_PINS];   // indexed by STM32 interrupt pin position
```

Each ISR is a minimal one-liner: `_intFlags[idx] = true`. No I2C inside an ISR.
`PanelGroup::loop()` checks all flags at the top of each iteration.

### Poll Timers

```cpp
uint32_t _lastAnalogPollMs;    // last time analog inputs were polled (8 ms interval)
uint32_t _lastFallbackPollMs;  // last time polling-fallback chips were scanned (~20 ms)
uint32_t _lastHeartbeatMs;     // last time HB_n was sent (500 ms interval)
```

All timers use `millis()` directly. No shared clock.

---

## Implementation Notes

### MCP23017 Initialisation (setup())

For each `ExpanderEntry`, during `setup()`:

1. Call `chip.init()` — initialises IOCON, sets all pins to input, enables all pull-ups (GPPU=0xFF). Pull-ups are cleared per-pin by `configureAsInput()` calls in step 2c.
2. Configure only the pins claimed by registered inputs/outputs. `PinRef::configureAsInput()`
   sets claimed input pins to IODIR=1 and leaves GPPU=0; OpenSkyhawk switch nets are
   hardware-biased by the schematic. **GPA7 and GPB7 must not be configured as inputs**
   (MCP23017 silicon bug, Microchip datasheet Rev D June 2022) — treat as output-only or
   leave unconfigured.
3. Set `IOCON.INTPOL = 0` (active-LOW interrupts).
4. Set `IOCON.ODR = 1` if this chip shares an interrupt line with another chip (open-drain).
5. Set `IOCON.MIRROR = 1` if `mirrored == true` (INTA == INTB).
6. Read GPA and GPB via `chip.readPort(MCP23017Port::A)` / `chip.readPort(MCP23017Port::B)` to
   establish baseline; store in `portAcache` / `portBcache`.
7. Configure the STM32 interrupt pin as `INPUT_PULLUP`.
8. `attachInterrupt(digitalPinToInterrupt(intaPin), isrForPin, FALLING)`.
   If `intbPin != intaPin`, attach a separate ISR for `intbPin`.

### Interrupt Dispatch (loop())

Each `loop()` iteration:

1. Scan `_intFlags[]`. For each `true` flag, clear it and iterate **all** `ExpanderEntry`
   records whose `intaPin` or `intbPin` matches the flagged STM32 pin.

   On a wired-OR shared line, multiple chips may have fired simultaneously — all of them
   hold the line LOW (open-drain) until their INTCAP is read. The scan must visit every
   chip on the line, not stop at the first one with a non-zero flag. Skipping a chip leaves
   its interrupt uncleared and the STM32 line permanently asserted.

2. For each matching entry:
   a. Call `chip.interruptedBy(intfA, intfB)` — reads INTF registers for both ports simultaneously.
      `intfA` is non-zero if any GPA pin changed; `intfB` is non-zero if any GPB pin changed.
   b. If both `intfA` and `intfB` are zero, the chip did not fire — skip to the next chip on
      the shared line. No INTCAP read, no cache update.
   c. Call `chip.clearInterrupts(capA, capB)` — reads INTCAP for both ports (capturing port
      state at the moment of interrupt) and clears the interrupt line. Must be called whenever
      `intfA || intfB` is non-zero, even if only one port fired — INTCAP must be read to
      deassert the open-drain interrupt line and allow the STM32 pin to go HIGH again.
   d. If `intfA != 0`: update `portAcache = capA`.
      If `intfB != 0`: update `portBcache = capB`.
      Unchanged ports keep their existing cache value — `clearInterrupts()` returns both
      INTCAP bytes but we only apply the ones flagged by INTF.

3. Once all chips on all flagged pins have been scanned and their caches updated, call
   `poll()` on all registered `InputBase` objects. Each `poll()` reads the cached state
   via its `PinRef`, compares against its own last-known state, and emits an EVT only if
   the value changed after debounce/filtering. Inputs whose pins did not change return
   immediately with no work done.

### Polling Fallback

A chip is in polling-fallback mode if no interrupt pin is registered for it (registered with
`intaPin == 0xFF` sentinel, or not registered via `registerExpander()` at all — the latter
applies to chips whose objects use direct GPIO PinRefs only).

Every ~20 ms, `loop()` reads both ports of each fallback chip and compares against cache.
Changed bits trigger the same InputBase dispatch as the interrupt path.

If a chip has an interrupt pin but the ISR never fires (wired interrupt line), the polling
fallback ensures switches are never permanently stuck. This is a safety net, not the primary
path.

### CAN RX Handling

PanelGroup drains all available CAN frames in `loop()` via `CANProtocol::drain()`. CANProtocol
owns special-frame interception before PanelGroup callbacks fire:

| CAN ID | Handling |
|--------|----------|
| `0x010` CTRL_BCAST | Delivered to PanelGroup's receive callback. Interpret payload as `ControlPacketPair`; walk `OutputBase` list; call `onControlPacket(controlId, value)` on every object for each non-null slot (controlId != 0x0000). |
| `0x012` SYNC_REQ | Intercepted by CANProtocol and delivered via `onSyncReq()`. PanelGroup calls `forceReport()` on all `InputBase` objects, then `CANProtocol::flushBatched(canIdEvt(NODE_ID))`. |
| `0x011` TEST_SEQ | Intercepted by CANProtocol and echoed automatically as `ECHO_n`; PanelGroup is not called. |
| Anything else | Delivered to PanelGroup's receive callback and ignored unless future specs claim it. |

### SYNC_REQ Response

On `SYNC_REQ`, `forceReport()` is called on every registered `InputBase` object by walking
the linked list from head to tail:

```cpp
for (InputBase* p = InputBase::head(); p; p = p->next()) {
    p->forceReport();
}
CANProtocol::flushBatched(canIdEvt(NODE_ID));
```

Every object that self-registered at construction is visited — none are skipped. Each
`forceReport()` reads current hardware state and emits unconditionally, regardless of whether
state changed since the last EVT. This produces a full-state snapshot identical in content to
the boot EVT burst. `flushBatched()` is called after the loop so no trailing half-full batch
is left pending.

### READY Frame

Sent once during `setup()`, after the initial EVT burst and `flushBatched()`. DLC = 0. No
payload. CAN ID = `canIdReady(NODE_ID)`. PanelBridge marks the node online and broadcasts
`SYNC_REQ` on receipt.

### Heartbeat Timer

`HB_n` is sent every 500 ms using `millis()` comparison. First heartbeat is sent 500 ms after
`setup()` completes (the `_lastHeartbeatMs` is set to `millis()` at the end of `setup()`).
The heartbeat payload is constructed by `CANProtocol::makeHeartbeatPayload(NODE_ID, rxCount)` —
PanelGroup does not read HAL CAN registers directly. `rxCount` is `uint16_t`; it wraps at
65535 (~131 s at full bus load). Rollover is intentional — the counter is a diagnostic liveliness
indicator, not an exact packet counter.

### EVT Batching

PanelGroup never forms `ControlPacketPair` directly. It calls `CANProtocol::sendBatched()` once
per changed input. CANProtocol owns the slot-A / slot-B pairing and the half-full batch
deadline (slot A queued but no slot B arrives within two `loop()` iterations → flush with
slot B `controlId = 0x0000`). During boot and `SYNC_REQ` passes, `flushBatched()` is called
explicitly so the tail packet goes out before `READY` or before returning to normal operation.

---

## Boot Sequence

From `FirmwarePlan/09-startup-resync-diagnostics.md#panelgroup-boot-sequence`:

```cpp
// In PanelGroup::setup():

// 1. STM32Board::begin() — status LED, DiagSerial, CAN HAL
STM32Board::begin();

// 2a. For each registered ADC: begin(addr, wire)
for (auto& entry : _adcs) { entry.adc->begin(entry.addr, entry.wire); }

// 2b. Detect wired-OR shared interrupt lines → set openDrain flag on affected chips.

// 2c. For each registered expander: init(), configure IOCON (MIRROR, ODR).
for (auto& entry : _expanders) {
    entry.chip->init();
    // set IOCON.MIRROR if intaPin == intbPin; set IOCON.ODR if shared line detected in 2b
}

// 3. Configure each pin via its owning input or output object.
//    Each configure() calls pin.configureAsInput() or configureAsOutput(),
//    which sets IODIR and disables GPPU on MCP23017 pins or calls pinMode() on GPIO pins.
for (InputBase*  p = InputBase::head();  p; p = p->next()) { p->configure(); }
for (OutputBase* p = OutputBase::head(); p; p = p->next()) { p->configure(); }

// 3b (step 2d). Enable interrupt-on-change on input pins only; read baseline; attach ISRs.
//     Must run AFTER configure() so IODIR is set correctly.
//     GPA7/GPB7 excluded from GPINTEN (MCP23017 silicon erratum Rev D — SDA corruption).
//     If MAX_INT_PINS (8) is exceeded, chip falls back to 20 ms polling and a warning
//     is logged via STM32Board::log().
for (auto& entry : _expanders) {
    uint8_t gpintenA = entry.chip->readRegister(MCP23017Register::IODIR_A) & 0x7Fu;
    uint8_t gpintenB = entry.chip->readRegister(MCP23017Register::IODIR_B) & 0x7Fu;
    entry.chip->writeRegister(MCP23017Register::GPINTEN_A, gpintenA);
    entry.chip->writeRegister(MCP23017Register::GPINTEN_B, gpintenB);
    entry.portAcache = entry.chip->readPort(MCP23017Port::A);
    entry.portBcache = entry.chip->readPort(MCP23017Port::B);
    // attachInterrupt() for intaPin and (if not mirrored) intbPin
}

// 3. Register CAN callbacks. CANProtocol::start() automatically accepts mandatory
//    CTRL_BCAST, TEST_SEQ, and SYNC_REQ frames; no separate filter-list helper is used.
CANProtocol::onReceive(onCanReceive);
CANProtocol::onSyncReq(onSyncReq);

// 4. Start CAN
CANProtocol::start();

// 5. Initial EVT burst — force-report every registered input (no debounce)
for (InputBase* p = InputBase::head(); p; p = p->next()) { p->forceReport(); }

// 6. Flush trailing batch
CANProtocol::flushBatched(canIdEvt(NODE_ID));

// 7. READY frame
CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);

// 8. Arm heartbeat timer
_lastHeartbeatMs = millis();
```

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| CANProtocol | `Firmware/Libraries/CANProtocol` | Frame IDs, sendBatched, flushBatched, drain, send, makeHeartbeatPayload, onReceive, onSyncReq, filterAcceptId, start |
| STM32Board | `Firmware/Libraries/STM32Board` | STM32Board::begin(), DiagSerial, status LED, CAN HAL init |
| blemasle/MCP23017 | PlatformIO registry | `^2.0.0`; `init()`, `readPort(MCP23017Port)`, `writePort()`, `interruptedBy()`, `clearInterrupts()`, `writeRegister()` |
| adafruit/Adafruit ADS1X15 | PlatformIO registry | via `ADS1115.h` wrapper; `begin()`, `readADC_SingleEnded()` |
| STM32duino Arduino core | PlatformIO `framework = arduino` | `Wire`, `attachInterrupt`, `millis()`, `digitalRead`, `analogRead`, `analogWrite` |
| A4EC generated headers | `tools/gen_a4ec` output | `A4EC_CmdIds.h` included by sketches; not included by PanelGroup library itself |
