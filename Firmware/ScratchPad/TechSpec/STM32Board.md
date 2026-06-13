# STM32Board — Technical Specification

**Status:** Done

**FirmwarePlan ref:** `FirmwarePlan/08-hardware-firmware-contracts.md#status-led--bi-color-redgreen`,
`FirmwarePlan/08-hardware-firmware-contracts.md#diagserial--usart1`,
`FirmwarePlan/02-can-protocol.md#can-bus-configuration`,
`FirmwarePlan/09-startup-resync-diagnostics.md`

**Depends on:** `CANProtocol.md` (for the `CanStatus` event type)

---

## Responsibility

Initialises and manages all shared hardware present on every STM32 board: bi-color status LED
(PB14/PB15), DiagSerial (USART1/Serial1), and CAN peripheral hardware configuration. Exposes
`NODE_ID` (compile-time constant from build_flags) with range validation at compile time.
Owns the LED state machine and tick-based animation driver.

Reacts to CAN bus status events via `onCanStatus()` — it does not poll CANProtocol and is
not directly controlled by the sketch or by CANProtocol. `CanStatus` is owned by
CANProtocol; STM32Board only translates those events into its private LED state machine.

Does **not** start the CAN peripheral or configure receive filters — that belongs to
`CANProtocol::start()`. Does not own any panel-specific I/O.

---

## File Layout

```
Firmware/Libraries/STM32Board/
├── STM32Board.h
├── STM32Board.cpp
└── library.json
```

Included in every STM32 sketch:
```cpp
#include <STM32Board.h>
```

Implemented as a `namespace` — there is always exactly one STM32 board per firmware build,
so a class with instances would be a fiction. Internal state variables are defined in
`STM32Board.cpp` only (not exposed in the header), keeping the public API clean.

### Test project

```
Firmware/Tests/STM32Board/
├── platformio.ini
└── tests/
    ├── led_state_machine/
    │   └── led_state_machine.cpp   — exercises all 6 LedStates via onCanStatus() (4 states) and
    │                                 setWarning() (WARNING); OFF shown pre-begin(). Verifies pin
    │                                 output matches the animation map (blink period, colour, solid/off)
    ├── diag_serial/
    │   └── diag_serial.cpp         — setDebug(false): log() produces no output; setDebug(true):
    │                                 log() emits the expected string on Serial1 at 115200 baud
    └── can_status_wiring/
        └── can_status_wiring.cpp   — calls onCanStatus() with each CanStatus value; verifies
                                      the correct LedState is entered (checks pin behaviour via tick())
```

`can_status_wiring.cpp` verifies the mapping from CANProtocol's `CanStatus` values to
STM32Board's private LED states, so both libraries must be in `lib_deps`:

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
    -DUSB_NONE
    -DHSE_VALUE=8000000
lib_extra_dirs = ${PROJECT_DIR}/../../Libraries
lib_deps =
    STM32Board
    CANProtocol

[env:test_led_state_machine]
extends = env_base
build_src_filter = -<*> +<led_state_machine/led_state_machine.cpp>

[env:test_diag_serial]
extends = env_base
build_src_filter = -<*> +<diag_serial/diag_serial.cpp>

[env:test_can_status_wiring]
extends = env_base
build_src_filter = -<*> +<can_status_wiring/can_status_wiring.cpp>
```

CAN bus integration testing is out of scope here — see CANProtocol TechSpec.

---

## Public API

```cpp
// STM32Board.h

enum class CanStatus;

namespace STM32Board {

    /**
     * @brief Initialise all shared hardware. Call once at the top of setup().
     *
     * Configures PB14 (Red) and PB15 (Green) as outputs and enters BOOTING state.
     * Starts DiagSerial (USART1, PA9/PA10, 115200 baud) — silent until setDebug(true).
     * Calls analogReadResolution(16) — framework scales 12-bit ADC output to 16-bit range
     * (0–65520) for all subsequent analogRead() calls; PinRef::readAnalog() relies on this.
     * Configures the CAN peripheral at 500 kbps on PA11/PA12 but does NOT start it —
     * call CANProtocol::start() after filter setup.
     * NODE_ID range (0–63) is validated at compile time via static_assert.
     */
    void begin();

    /**
     * @brief Enable or disable DiagSerial output.
     *
     * DiagSerial is always initialised by begin(); this flag gates all log() calls.
     *
     * @param on True to emit output on USART1; false for silence (default).
     */
    void setDebug(bool on);

    /**
     * @brief Drive LED animations. Call once per loop() iteration.
     *
     * Advances blink state using millis(). Fully non-blocking — no busy-wait or delay.
     */
    void tick();

    /**
     * @brief CAN bus status event handler.
     *
     * Called when CANProtocol fires a status change.
     * Maps CanStatus to an internal LedState — the sketch and CANProtocol never
     * access the LED directly.
     *
     * @param status New CAN bus status reported by CANProtocol.
     * @note CanStatus is defined by CANProtocol.h and forward-declared here to keep
     *       STM32Board.h light. STM32Board.cpp includes CANProtocol.h for the enum values.
     */
    void onCanStatus(CanStatus status);

    /**
     * @brief Enter WARNING LED state — red/green alternating at 500 ms.
     *
     * Call for any non-CAN fault: SYNC timeout, missing heartbeat, I²C bus hang,
     * or application-layer fault. CanStatus has no WARNING value — this is the
     * only public entry point for that state.
     */
    void setWarning();

    /**
     * @brief Returns true when debug output is enabled.
     *
     * Guard multi-field formatted print blocks with this to avoid string formatting
     * overhead when debug is off.
     */
    bool isDebug();

    /**
     * @brief Print a line to DiagSerial if debug is enabled; no-op otherwise.
     * @param msg Null-terminated string to print.
     */
    void log(const char* msg);

    /**
     * @brief Access DiagSerial directly for multi-field formatted output.
     *
     * Guard with isDebug() to avoid formatting overhead when debug is off.
     *
     * @returns Reference to the USART1 HardwareSerial instance.
     */
    HardwareSerial& diagSerial();

    static constexpr uint8_t PIN_LED_RED   = PB14;  ///< Red LED pin — same on all STM32 boards
    static constexpr uint8_t PIN_LED_GREEN = PB15;  ///< Green LED pin — same on all STM32 boards

} // namespace STM32Board
```

NODE_ID compile-time validation, placed at file scope in `STM32Board.h`:
```cpp
static_assert(NODE_ID <= 63,
    "NODE_ID must be 0–63. 0 = PanelBridge (reserved); 1–63 = PanelGroup nodes.");
```

### Sketch wiring

The sketch is the integration point:

```cpp
// setup()
STM32Board::begin();
CANProtocol::onStatusChange(STM32Board::onCanStatus);
CANProtocol::start();

// loop()
STM32Board::tick();
```

`CANProtocol::onStatusChange()` is defined in the CANProtocol TechSpec.

---

## Key Data Structures

### LedState (private enum)

```cpp
enum class LedState {
    OFF,        // both LEDs off — pre-begin() only
    BOOTING,    // red slow blink  (1000 ms) — initialising
    NORMAL,     // green slow blink (1000 ms) — CAN bus healthy
    CAN_ERROR,  // red fast blink  (250 ms)  — TEC > 0, errors accumulating
    BUS_OFF,    // red solid                  — CAN controller halted
    WARNING     // red/green alternating (500 ms) — degraded state
};
```

`LedState` is an internal type — not exposed in the public API.

### LedState → Animation Map

| State | Red | Green | Blink period |
|-------|-----|-------|-------------|
| `OFF` | off | off | — |
| `BOOTING` | blink | off | 1000 ms |
| `NORMAL` | off | blink | 1000 ms |
| `CAN_ERROR` | blink | off | 250 ms |
| `BUS_OFF` | solid on | off | — |
| `WARNING` | alternating | alternating | 500 ms |

For `WARNING`: phase 0 → red on, green off; phase 1 → red off, green on.

### CanStatus → LedState mapping (inside onCanStatus)

| CanStatus | LedState |
|-----------|----------|
| `STARTING` | `BOOTING` |
| `NORMAL` | `NORMAL` |
| `TX_ERROR` | `CAN_ERROR` |
| `BUS_OFF` | `BUS_OFF` |

`CanStatus` is defined in `CANProtocol.h`. See CANProtocol TechSpec for the full enum.

---

## Implementation Notes

### tick() — blink state machine

```cpp
void STM32Board::tick() {
    uint32_t now    = millis();
    uint16_t period = _blinkPeriodFor(_state);  // 0 = solid or off

    if (period > 0 && (now - _ledLastToggleMs) >= (uint32_t)(period / 2)) {
        _blinkPhase      = !_blinkPhase;
        _ledLastToggleMs = now;
        _applyLed();  // only called when phase actually changes
    }
    // solid/off states: pins already set by setLedState() — nothing to do here
}
```

`_applyLed()` is called in two places only:
- Inside `setLedState()` — applies the new state immediately on transition
- Inside `tick()` — only when `_blinkPhase` toggles

`digitalWrite()` is called exactly twice per blink cycle for blinking states, and once per
state transition for solid and off states. Pins are never written redundantly.

### NODE_ID — PanelBridge reservation

`NODE_ID=0` is reserved for PanelBridge. PanelBridge is a STM32 board and a valid CAN bus
participant, but it is the master node — it does not have a sub-node address in the
0x101–0x43F frame ID ranges used by PanelGroup nodes.

The `static_assert` uses `<= 63` (no lower bound) so PanelBridge passes with `NODE_ID=0`:

```cpp
static_assert(NODE_ID <= 63,
    "NODE_ID must be 0–63. 0 is reserved for PanelBridge; 1–63 for PanelGroup nodes.");
```

SimGateway is an RP2040 — it does not use STM32Board and has no NODE_ID concern.

### CAN peripheral configuration

`begin()` configures the STM32 CAN peripheral (baud rate prescaler, segment timing for
500 kbps, PA11/PA12 alternate function) but does **not** call the HAL start function.
The peripheral is left configured-but-stopped — ready for `CANProtocol::start()`.

**Library choice:** direct STM32duino HAL (`CAN_HandleTypeDef`, `HAL_CAN_Init`) — no
wrapper library. Confirmed by the prototype CAN stress test (Experiment B).

**Clock tree:** 8 MHz crystal × PLL ×9 = 72 MHz SYSCLK; APB1 prescaler /2 = **36 MHz APB1**
(STM32F103 APB1 max is 36 MHz; CAN1 is on APB1).

**Confirmed HAL init struct** — validated in Experiment B (21-min soak, 1,257 frames, 0 lost, TEC=0):

```cpp
_hcan.Instance               = CAN1;
_hcan.Init.Prescaler         = 4;              // 36 MHz APB1 / 4 / 18TQ = 500 kbps
_hcan.Init.Mode              = CAN_MODE_NORMAL;
_hcan.Init.SyncJumpWidth     = CAN_SJW_4TQ;   // must be 4TQ — Blue Pill clone tolerance
_hcan.Init.TimeSeg1          = CAN_BS1_13TQ;
_hcan.Init.TimeSeg2          = CAN_BS2_4TQ;   // total: 1+13+4 = 18TQ
_hcan.Init.TimeTriggeredMode = DISABLE;
_hcan.Init.AutoBusOff        = ENABLE;         // hardware recovery ~3 ms, no firmware action needed
_hcan.Init.AutoWakeUp        = DISABLE;
_hcan.Init.AutoRetransmission = DISABLE;       // prevent runaway bus-off — see CANProtocol HAL notes
_hcan.Init.ReceiveFifoLocked  = DISABLE;       // overwrite oldest on FIFO full (consistent with drop-oldest policy)
_hcan.Init.TransmitFifoPriority = DISABLE;     // TX priority by message ID (standard CAN arbitration)
```

> **Note:** the comment `72 MHz / 4 / (1+13+4) = 500 kbps` in the source is a simplification.
> The actual CAN clock is APB1 = 36 MHz. The HAL uses the real APB1 clock at init time, so
> the parameters produce 500 kbps correctly regardless of the comment.

**Scope boundary:** STM32Board's responsibility ends at configuring the peripheral.
Verifying that the bus comes up at 500 kbps and that frames flow correctly is out of scope
here — that belongs to the CANProtocol breadboard test.

### DiagSerial — always initialised, gated by flag

`Serial1.begin(115200)` is called unconditionally in `begin()` so the pins (PA9/PA10) are
always claimed. All `diagPrint*` calls check `_debugEnabled` and return immediately if false.
No `#ifdef DEBUG` guards — zero runtime cost when disabled.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| STM32duino Arduino core | PlatformIO `framework = arduino` | `millis()`, `pinMode()`, `digitalWrite()`, `Serial1` |
| STM32duino HAL CAN | STM32duino Arduino core | Direct `CAN_HandleTypeDef` / `HAL_CAN_Init` — no wrapper library needed |
| `CanStatus` enum | `CANProtocol.h` | Owned by CANProtocol; forward-declared in `STM32Board.h`, included by `STM32Board.cpp` for value mapping |
| `NODE_ID` define | `platformio.ini` `build_flags` | Must be present; compile fails otherwise |
