# LED — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#led-exists`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Digital LED output. Drives a single `PinRef` HIGH (on) or LOW (off) when a matching
CTRL_BCAST `ControlPacket` arrives. A bitmask is applied to the received value before
the threshold check — this handles DCS-BIOS outputs that pack multiple flags into one
16-bit word at a single address. Self-registers into PanelGroup's OutputBase list.

Does **not** apply PWM or dimming — that is `AnalogOutput`'s responsibility. Does **not**
buffer or cache the last-received value. Does **not** communicate with PanelBridge
directly. Does **not** interpret what the DCS-BIOS address means.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
├── LED.h
└── LED.cpp
```

### Test project

```
Firmware/Tests/LED/
├── platformio.ini
└── tests/
    ├── configure/
    │   └── configure.cpp       — reverse=false: configure() drives pin LOW (off);
    │                             reverse=true:  configure() drives pin HIGH (off);
    │                             configureAsOutput() invoked exactly once in both cases
    ├── on_off/
    │   └── on_off.cpp          — reverse=false: value > 0 → pin HIGH; value == 0 → pin LOW;
    │                             consecutive on/off/on cycles all take effect
    ├── reverse/
    │   └── reverse.cpp         — reverse=true: value > 0 → pin LOW (on); value == 0 → pin HIGH (off);
    │                             reverse=false (default) matches non-reversed pin behaviour;
    │                             verifies inversion applies to both on and off transitions
    ├── mask/
    │   └── mask.cpp            — (value & mask) != 0 → on; (value & mask) == 0 → off even
    │                             when value itself is non-zero; mask 0x0001 isolates bit 0 correctly
    └── controlid_filter/
        └── controlid_filter.cpp — non-matching controlId leaves pin unchanged; correct
                                   controlId after a non-matching one still triggers write
```

Tests run on physical STM32 hardware. GPIO reads via `digitalRead()` verify pin state after
each `configure()` and `onControlPacket()` call. No mock PinRef — real GPIO output.

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

[env:test_configure]
extends = env_base
build_src_filter = -<*> +<configure/configure.cpp>

[env:test_on_off]
extends = env_base
build_src_filter = -<*> +<on_off/on_off.cpp>

[env:test_reverse]
extends = env_base
build_src_filter = -<*> +<reverse/reverse.cpp>

[env:test_mask]
extends = env_base
build_src_filter = -<*> +<mask/mask.cpp>

[env:test_controlid_filter]
extends = env_base
build_src_filter = -<*> +<controlid_filter/controlid_filter.cpp>
```

---

## Public API

```cpp
// LED.h

#pragma once
#include <PanelGroup.h>   // OutputBase, PinRef

namespace OpenSkyhawk {

/**
 * @brief Digital LED output. Drives a pin based on a DCS-BIOS state value.
 *
 * Receives state via onControlPacket() — called by PanelGroup when a CTRL_BCAST
 * frame arrives. Ignores packets whose controlId does not match.
 *
 * The mask parameter handles DCS-BIOS bit-packed outputs, where a single 16-bit address
 * carries multiple independent flags. For whole-word binary outputs (value is 0 or 1),
 * use mask = 0xFFFF.
 *
 * The reverse parameter handles LEDs wired with the opposite drive polarity — for
 * example, an indicator LED with its anode tied to VCC through a resistor, where
 * the MCU or MCP23017 output sinks current (LOW = on). reverse = false (default):
 * (value & mask) != 0 → pin HIGH (on). reverse = true: (value & mask) != 0 → pin LOW (on).
 *
 * Pin is driven to the off state during configure() and remains off until a CTRL_BCAST
 * packet with a matching controlId is received.
 *
 * Works with GPIO and MCP23017 PinRefs. ADS1115 PinRefs are rejected at configure()
 * time with a debug assertion — ADS1115 is input-only.
 */
class LED : public OutputBase {
public:
    /**
     * @brief Construct and register an LED output.
     * @param controlId  DCS-BIOS output address for this indicator (A_4E_C_* constant
     *                   from A4EC_OutputIds.h). Must match the incoming CTRL_BCAST
     *                   controlId to trigger a pin write.
     * @param mask       Bitmask for the relevant bits at this address (A_4E_C_*_AM
     *                   constant from A4EC_OutputIds.h).
     *                   (value & mask) != 0 → on; (value & mask) == 0 → off.
     * @param pin        PinRef for the LED output pin (GPIO or MCP23017).
     * @param reverse    false (default): pin HIGH = on. true: pin LOW = on.
     *                   Use true for current-sinking wiring (anode to VCC through
     *                   resistor; MCU/MCP23017 pin sinks current to GND).
     */
    LED(uint16_t controlId, uint16_t mask, PinRef pin, bool reverse = false);

    /**
     * @brief Configure the output pin and drive it to the off state.
     * Called by PanelGroup::setup() after chip.begin(). Sets OUTPUT mode on GPIO
     * pins; sets IODIR=0 and GPPU=0 on MCP23017 pins. Drives the pin to its off level
     * immediately: LOW when reverse = false, HIGH when reverse = true. The LED remains
     * off until a CTRL_BCAST packet arrives with a non-zero masked state.
     */
    void configure() override;

    /**
     * @brief Update LED state from an incoming CTRL_BCAST packet.
     * If controlId matches, evaluates (value & mask): non-zero → drives pin HIGH (on);
     * zero → drives pin LOW (off). Ignores the packet if controlId does not match.
     */
    void onControlPacket(uint16_t controlId, uint16_t value) override;

private:
    uint16_t _controlId;
    uint16_t _mask;
    PinRef   _pin;
    bool     _reverse;    // true = current-sinking wiring (LOW = on)
};

} // namespace OpenSkyhawk
```

---

## Sketch Usage

```cpp
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <LED.h>
#include <A4EC_OutputIds.h>  // A_4E_C_* address and mask constants — generated by gen_a4ec.py
                              // Names match DCS-BIOS published identifiers (Bort-compatible).
                              // See A4ECGenerator.md for generation details.

// ── Expanders ─────────────────────────────────────────────────────────────────
MCP23017 exp1(0x20, Wire);

// ── Wiring map ────────────────────────────────────────────────────────────────
const PinRef PIN_MASTER_CAUTION_LED = PinRef(exp1, PORT_A, 0);  // MCP23017 output
const PinRef PIN_FIRE_WARNING_LED   = PinRef(PB7);               // direct GPIO

// ── LED declarations ──────────────────────────────────────────────────────────
// controlId and mask come from A4EC_OutputIds.h — names match DCS-BIOS published
// identifiers so they can be looked up directly in Bort and other debug tools.
// reverse=false (default): MCU HIGH = on  (current-sourcing: MCU → R → LED → GND)
// reverse=true:            MCU LOW  = on  (current-sinking:  VCC → R → LED → MCU)
// Each constructor self-registers into PanelGroup's OutputBase list.
OpenSkyhawk::LED masterArm  (A_4E_C_ARM_MASTER,  A_4E_C_ARM_MASTER_AM,  PIN_MASTER_ARM_LED);              // sourcing, bit-packed
OpenSkyhawk::LED aoaGreen   (A_4E_C_AOA_GREEN,   A_4E_C_AOA_GREEN_AM,   PIN_AOA_GREEN_LED);               // sourcing, bit-packed
OpenSkyhawk::LED fireWarning(A_4E_C_AOA_RED,     A_4E_C_AOA_RED_AM,     PIN_AOA_RED_LED, /*reverse=*/true); // sinking

void setup() {
    Wire.begin();
    PanelGroup::registerExpander(exp1, PB3, PB4);  // INTA→PB3, INTB→PB4
    PanelGroup::setup();   // inits expanders, calls configure() on all OutputBase
                           // objects (drives all LEDs LOW), emits boot EVT burst,
                           // sends READY frame
}

void loop() {
    PanelGroup::loop();    // drains CAN RX, distributes CTRL_BCAST to OutputBase list —
                           // LED needs nothing else
}
```

The sketch has no direct interaction with LED after construction. `PanelGroup::loop()`
drives everything — each LED is invisible to `loop()` itself.

---

## Key Data Structures

No structs beyond the private members. All state is per-instance — no statics.

---

## Implementation Notes

### configure()

```cpp
void LED::configure() {
    _pin.configureAsOutput();
    _pin.write(_reverse);   // reverse=false → LOW (off); reverse=true → HIGH (off)
}
```

Drives the pin to its off level immediately after configuring the pin direction. Without
this, the GPIO output register may be undefined on first configure, or an MCP23017 output
pin may glitch to its power-on default level before the first CTRL_BCAST arrives.

For current-sinking wiring (`reverse = true`), the off state is HIGH — the pin held HIGH
keeps the LED cathode above GND and prevents current flow.

### onControlPacket()

```cpp
void LED::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    bool on = (value & _mask) != 0;
    _pin.write(_reverse ? !on : on);
}
```

The mask and reverse are both evaluated on every matching packet. There is no hysteresis
or debounce — DCS-BIOS output values are authoritative; whatever PanelBridge sends is
immediately applied.

### Why a mask?

DCS-BIOS packs several related indicators into a single 16-bit output address. For
example, one address might hold gear status bits for all three legs simultaneously.
Each physical LED maps to one bit position within that word.

| Scenario | mask value | When does LED turn on? |
|---|---|---|
| Whole-word binary output (0 or 1) | `0xFFFF` | value != 0 |
| Bit 0 of a packed word | `0x0001` | bit 0 set |
| Bit 9 of a packed word | `0x0200` | bit 9 set |
| Any of bits 0–3 | `0x000F` | any lower nibble bit set |

Using `0xFFFF` for a whole-word output is not a special case — it is algebraically
identical to `value != 0`. No branching on mask value is needed.

### MCP23017 pins

For MCP23017-backed PinRefs, `_pin.write(value)` calls PanelGroup's expander write
path, which updates the internal output latch cache and issues an I2C write. The write
is synchronous — the I2C transaction completes before `write()` returns.

### No state caching

LED does not track the last-written state. If the node power-cycles, all LEDs return
to off (from `configure()`) and remain off until PanelBridge re-sends CTRL_BCAST.
PanelBridge is responsible for re-broadcasting DCS state after detecting a node READY
frame. LED needs no recovery logic.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | OutputBase, PinRef, onControlPacket dispatch |
| CANProtocol | `Firmware/Libraries/CANProtocol` | CTRL_BCAST reception and dispatch handled by PanelGroup; LED calls neither directly |
