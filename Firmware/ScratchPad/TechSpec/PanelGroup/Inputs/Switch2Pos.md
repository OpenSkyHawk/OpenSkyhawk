# Switch2Pos — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switch2pos-exists--needs-pinref-update`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Debounced 2-position switch. Monitors a single digital `PinRef` (direct GPIO or MCP23017).
Emits a CAN EVT via `CANProtocol::sendBatched()` when the confirmed state changes after a
fixed 20 ms debounce window. Works identically for DCS-BIOS (`DCSIN_*` controlId) and HID
(`CTRL_*` controlId) routing — `controlId` determines how PanelBridge forwards the packet.

Does **not** interpret `controlId` values. Does **not** communicate with PanelBridge directly.
Does **not** apply filtering beyond the debounce period.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
├── Switch2Pos.h
└── Switch2Pos.cpp
```

### Test project

```
Firmware/Tests/Switch2Pos/
├── platformio.ini
└── tests/
    ├── configure.cpp       — configure() calls _pin.configureAsInput(); mock PinRef records
    │                         that configureAsInput() was invoked exactly once; verifies no
    │                         EVT is emitted and _initialized remains false after configure()
    ├── force_report.cpp    — forceReport() emits current state immediately regardless of
    │                         previous state; sets _initialized so poll() becomes active;
    │                         second forceReport() with same state still emits
    ├── debounce.cpp        — change emits EVT only after debounce window stable; bounce
    │                         within window resets the timer; no EVT on sub-window bounce;
    │                         poll() is no-op before forceReport() is called; 20 ms fixed
    │                         debounce threshold is respected
    ├── value_encoding.cpp  — reverse=false: pin LOW → value 1 (active), pin HIGH → value 0;
    │                         controlId passed correctly to CANProtocol::sendBatched()
    ├── reverse.cpp         — reverse=true: pin HIGH → value 1 (active), pin LOW → value 0;
    │                         reverse=false (default constructor) matches active-LOW behaviour;
    │                         forceReport() and poll() both respect reverse flag
    └── no_duplicate_evt.cpp — once EVT emitted, same stable state does not re-emit on
                               subsequent poll() calls
```

Tests use a mock `PinRef` with settable return value and a fake `CANProtocol::sendBatched()`
that records calls. No physical GPIO or CAN bus required.

**`platformio.ini`:**

```ini
[env_base]
platform = ststm32
board = genericSTM32F103C8
framework = arduino
build_flags = -DNODE_ID=1
lib_deps =
    file://../../Libraries/PanelGroup
    file://../../Libraries/CANProtocol
    blemasle/arduino-mcp23017
    adafruit/Adafruit ADS1X15

[env:test_configure]
extends = env_base
build_src_filter = -<*> +<tests/configure.cpp>

[env:test_force_report]
extends = env_base
build_src_filter = -<*> +<tests/force_report.cpp>

[env:test_reverse]
extends = env_base
build_src_filter = -<*> +<tests/reverse.cpp>

[env:test_debounce]
extends = env_base
build_src_filter = -<*> +<tests/debounce.cpp>

[env:test_value_encoding]
extends = env_base
build_src_filter = -<*> +<tests/value_encoding.cpp>

[env:test_no_duplicate_evt]
extends = env_base
build_src_filter = -<*> +<tests/no_duplicate_evt.cpp>
```

---

## Public API

```cpp
// Switch2Pos.h

#pragma once
#include <PanelGroup.h>   // InputBase, PinRef

namespace OpenSkyhawk {

/**
 * @brief Debounced 2-position switch. Self-registers into PanelGroup's InputBase list.
 *
 * VALUE semantics (reverse = false, default):
 *   1 — active   (pin LOW — switch closed, pulling pin to GND via board pull-up)
 *   0 — inactive (pin HIGH)
 *
 * VALUE semantics (reverse = true):
 *   1 — active   (pin HIGH — switch drives pin HIGH; external pull-down holds pin LOW when open)
 *   0 — inactive (pin LOW)
 *
 * Debounce: fixed 20 ms. The pin must hold its new level for the debounce period before
 * the state is confirmed and a CAN EVT is emitted. Any level change during the window
 * restarts the timer.
 *
 * forceReport() emits the current physical state immediately without debounce — called
 * by PanelGroup during the boot EVT burst and on SYNC_REQ.
 */
class Switch2Pos : public InputBase {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;

    /**
     * @brief Construct a 2-position switch with default settings.
     * Active-LOW (reverse = false), 20 ms debounce.
     * @param controlId  DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pin        PinRef for the switch input pin (GPIO or MCP23017).
     */
    Switch2Pos(uint16_t controlId, PinRef pin);

    /**
     * @brief Construct a 2-position switch with explicit polarity.
     * @param controlId   DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pin         PinRef for the switch input pin (GPIO or MCP23017).
     * @param reverse     false (default): active-LOW — board wiring holds HIGH, switch pulls LOW.
     *                    true: active-HIGH — board wiring holds LOW, switch drives HIGH.
     *                    configure() does not enable internal pull-ups; the schematic must
     *                    provide the required pull-up, pull-down, or active drive.
     */
    Switch2Pos(uint16_t controlId, PinRef pin, bool reverse);

    /**
     * @brief Read current pin state, apply debounce, emit EVT if confirmed state changed.
     * Called by PanelGroup::loop() during normal operation. No-op until forceReport()
     * has been called at least once.
     */
    void poll() override;

    /**
     * @brief Read current pin state and emit EVT unconditionally — no debounce.
     * Called by PanelGroup during the boot EVT burst and on SYNC_REQ. Confirms the
     * current reading as the baseline so subsequent poll() calls have a valid reference.
     */
    void forceReport() override;

    /**
     * @brief Configure the input pin. Called by PanelGroup::setup() after chip.begin().
     * Configures the pin as an input. Does not enable internal pull-ups; board wiring
     * supplies the input bias. Typical OpenSkyhawk switch nets use external 10 kΩ
     * pull-ups to +3.3V and switch to GND.
     * Must not be called from the constructor — MCP23017 register writes require the
     * chip to be initialised first.
     */
    void configure() override;

private:
    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;          // true = active-HIGH (external pull-down required)
    bool     _lastConfirmed;    // last emitted state (true = active)
    bool     _pendingRaw;       // raw reading at the last level change
    uint32_t _debounceStartMs;  // millis() when _pendingRaw last changed
    bool     _initialized;      // false until forceReport() is called
};

} // namespace OpenSkyhawk
```

---

## Sketch Usage

```cpp
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <Switch2Pos.h>
#include <A4EC_CmdIds.h>    // DCSIN_* constants
#include <HIDControls.h>    // CTRL_* constants (if any HID buttons on this node)

// ── Expanders ─────────────────────────────────────────────────────────────────
MCP23017 exp1(0x20, Wire);

// ── Wiring map ────────────────────────────────────────────────────────────────
// One named PinRef per physical connection. Name matches schematic net label.
// No inline literals below this section.
const PinRef PIN_MASTER_ARM      = PinRef(exp1, PORT_A, 3);  // MCP23017 input
const PinRef PIN_SEAT_EJECT_SAFE = PinRef(PB5);              // direct STM32 GPIO

// ── Switch declarations ───────────────────────────────────────────────────────
// Each constructor self-registers into PanelGroup's InputBase list.
// controlId determines how PanelBridge routes the EVT — not Switch2Pos's concern.
OpenSkyhawk::Switch2Pos masterArm    (DCSIN_ARM_MASTER,       PIN_MASTER_ARM);       // 20 ms, active-LOW
OpenSkyhawk::Switch2Pos seatEjectSafe(DCSIN_SEAT_EJECT_SAFE, PIN_SEAT_EJECT_SAFE);  // 20 ms, active-LOW

// Explicit polarity override only when the schematic is active-HIGH:
OpenSkyhawk::Switch2Pos trigger(CTRL_TRIGGER, PIN_TRIGGER, /*reverse=*/true);

void setup() {
    Wire.begin();
    PanelGroup::registerExpander(exp1, PB3, PB4);  // INTA→PB3, INTB→PB4
    PanelGroup::setup();   // inits expanders, emits boot EVT burst (calls poll() on all
                           // inputs including both Switch2Pos objects), sends READY frame
}

void loop() {
    PanelGroup::loop();    // checks interrupts, updates cache, calls poll() on all inputs,
                           // drains CAN RX, sends heartbeat — Switch2Pos needs nothing else
}
```

The sketch has no direct interaction with Switch2Pos after construction. `PanelGroup::loop()`
drives everything — the switch is invisible to `loop()` itself.

---

## Key Data Structures

No structs beyond the private members above. All state is per-instance — no statics.

---

## Implementation Notes

### configure()

```cpp
void Switch2Pos::configure() {
    _pin.configureAsInput();
}
```

Delegates entirely to `PinRef::configureAsInput()`, which applies the correct setup for whatever backing source the pin uses:

- **GPIO**: `pinMode(pin, INPUT)`
- **MCP23017**: sets IODIR bit = 1 (input) and GPPU bit = 0 (internal pull-up disabled) for this specific pin only
- **ADS1115 / NC**: no-op

`configure()` is called by `PanelGroup::setup()` after all expander `begin()` calls, so MCP23017 register writes are safe. Never called from the constructor.

---

### Polarity convention

`reverse = false` (default) — active-LOW. The switch connects the pin to GND when closed.
The board wiring holds the pin HIGH when the switch is open, typically through an external
10 kΩ pull-up to +3.3V.

```
pin HIGH → pinRef.read() == true  → inactive → VALUE 0
pin LOW  → pinRef.read() == false → active   → VALUE 1
active = !pinRef.read()
```

`reverse = true` — active-HIGH. Board wiring holds the pin LOW when the switch is open and
the switch drives the pin HIGH when closed. The MCP23017 does not provide internal
pull-downs, so expander-backed active-HIGH switch inputs require an external pull-down or
active drive.

```
pin LOW  → pinRef.read() == false → inactive → VALUE 0
pin HIGH → pinRef.read() == true  → active   → VALUE 1
active = pinRef.read()
```

The single inversion point in both cases:

```cpp
bool active = _reverse ? _pin.read() : !_pin.read();
```

### Debounce algorithm

```cpp
void Switch2Pos::forceReport() {
    // Boot EVT burst and SYNC_REQ. Read pin, confirm immediately, emit unconditionally.
    bool current     = _reverse ? _pin.read() : !_pin.read();
    _lastConfirmed   = current;
    _pendingRaw      = current;
    _debounceStartMs = millis();
    _initialized     = true;
    CANProtocol::sendBatched(canIdEvt(NODE_ID),
                             ControlPacket{_controlId, current ? 1u : 0u});
}

void Switch2Pos::poll() {
    if (!_initialized) return;   // forceReport() not yet called — state uninitialised

    bool raw = _reverse ? _pin.read() : !_pin.read();

    if (raw != _pendingRaw) {
        // Level changed — restart debounce timer.
        _pendingRaw      = raw;
        _debounceStartMs = millis();
    } else if (_pendingRaw != _lastConfirmed &&
               millis() - _debounceStartMs >= DEBOUNCE_MS) {
        // Stable for DEBOUNCE_MS and different from confirmed state — confirm and emit.
        _lastConfirmed = _pendingRaw;
        CANProtocol::sendBatched(canIdEvt(NODE_ID),
                                 ControlPacket{_controlId, _lastConfirmed ? 1u : 0u});
    }
    // pendingRaw == lastConfirmed: already at confirmed state, nothing to do.
}
```

### Why forceReport() skips debounce

Hardware is assumed stable at both call sites: the boot EVT burst fires after the MCU has
been powered for the full startup sequence, and SYNC_REQ arrives after DCS loads a new
mission — not during a switch throw. Applying a 20 ms debounce here would delay the READY
frame and resync response unnecessarily.

`forceReport()` also sets `_initialized = true`, which is what allows `poll()` to run. This
ensures `poll()` always has a valid `_lastConfirmed` baseline to compare against.

### Debounce and poll rate

The debounce timer is wall-clock (`millis()`), not call-count based. It is equally correct
whether `poll()` is called every 1 ms (interrupt-driven path) or every 20 ms (polling
fallback). A bounce that settles in less than the polling interval may not be re-sampled, but
mechanical switches settle well within 10 ms — safely below the fixed 20 ms debounce period.

### EVT packet transmission

When a state change is confirmed, Switch2Pos calls:

```cpp
CANProtocol::sendBatched(canIdEvt(NODE_ID), ControlPacket{_controlId, value});
```

- `canIdEvt(NODE_ID)` computes the CAN frame ID for this node — `0x200 + NODE_ID`.
  `NODE_ID` is a compile-time constant set via `build_flags = -DNODE_ID=x` in
  `platformio.ini`. Switch2Pos never hardcodes a node ID.

- `ControlPacket{_controlId, value}` is the 4-byte payload — the routing key and the
  switch state (0 or 1).

- `CANProtocol::sendBatched()` does not send immediately. It queues the packet into slot A
  of a `ControlPacketPair`. If a second input fires before the deadline, it fills slot B and
  the pair is sent as one 8-byte CAN frame. If slot B never arrives, CANProtocol flushes
  the pair with `controlId = 0x0000` in slot B within two `PanelGroup::loop()` iterations.

Switch2Pos owns nothing beyond the `sendBatched` call — frame construction, queuing,
transmission timing, and CAN mailbox management are all CANProtocol's responsibility.

### MCP23017 pins

For MCP23017-backed PinRefs, `_pin.read()` calls `PanelGroup::readCachedPin()` — no I2C.
The cache is already up to date before `poll()` is called (PanelGroup updates it from INTCAP
before walking the InputBase list). See `PanelGroup.md` — MCP23017 Port Cache.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | InputBase, PinRef, CANProtocol::sendBatched, canIdEvt, NODE_ID |
| CANProtocol | `Firmware/Libraries/CANProtocol` | ControlPacket, sendBatched |
