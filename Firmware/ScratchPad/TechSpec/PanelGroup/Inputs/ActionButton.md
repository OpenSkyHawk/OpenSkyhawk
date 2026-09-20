# ActionButton — Technical Specification

**Status:** Implemented (#116)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#actionbutton`
**Depends on:** `PinRef.md`, `PanelGroup.md`, `CANProtocol.md`

---

## Responsibility

Momentary push button driving a control that **latches in the sim**. Emits exactly one
`EVT_ACTION_n` frame (`canIdEvtAction(NODE_ID)`, `0x700+n`) on the press edge; nothing on release.

The payload value is a **selector, not a magnitude**: `0` means `TOGGLE`, the only argument
DCS-BIOS's `action` interface defines in practice. PanelBridge renders it as the DCS-BIOS string —
no DCS-BIOS text exists node-side.

---

## Why this class exists

`Switch2Pos` sends absolute 0/1, so it needs a physical switch that latches. `ActionButton` lets a
*momentary* part drive a control that latches in the sim.

DCS-BIOS `TOGGLE` reads the control's current value, flips it, and writes it back
(`Module.lua`, `defineTumb` input processor). Two consequences:

- each press toggles and the state **persists** — no physical latching part required
- the panel **cannot desync**, because the sim supplies the current value. A physical latching
  switch driven by `Switch2Pos` can disagree with the sim after a cold start, a keyboard binding,
  or a mission script, and then physically lies about state until cycled

The trade-off is that a momentary button shows no state at a glance.

**Applicable only to controls DCS-BIOS declares with `defineToggleSwitch`.** Pointing this class at
a `definePushButton` control is a defect: those are `api_variant = momentary_last_position`, the sim
tracks the button's physical position, and a `TOGGLE` latches them on until the next press. Use
`Switch2Pos` for those.

---

## Behaviour

| Event | Emits |
|---|---|
| press edge (debounced) | one `EVT_ACTION_n`, value `0` |
| held, any duration | nothing further |
| release | nothing |
| next press | fires again |
| `forceReport()` | **nothing** — see below |

### No repeat while held

The guard is structural, not a repeat-rate limit: `poll()` compares the current reading against
`_lastPressed` and does nothing when they match. There is no interval at which repetition could
begin. Release refreshes `_lastPressed`, which is what re-arms the class.

### Debounce — post-fire suppression

20 ms, applied **after** the send rather than as a stability window before it. An action's first
edge *is* the event, so waiting for the level to settle would only add latency; suppressing
afterwards costs none.

This matters more than for a level-tracking class. Every `TOGGLE` flips sim state, so an even number
of bounces cancels and an odd number flips — without suppression the switch's final position would
depend on bounce parity. `Switch2Pos` is immune, since a bounce merely re-sends the same absolute
value.

The comparison is elapsed-time (`millis() - _lastFireMs`), not a deadline, so it is correct across
the `millis()` wrap.

### `forceReport()` — silent, but not a no-op

`InputBase::forceReport()` runs on the boot EVT burst and on **every `SYNC_REQ`**. Every other input
class emits its current position there, which is idempotent for a switch: re-sending position 1
leaves the sim at 1. An action is not idempotent — emitting would flip the sim switch on every
resync — so this class emits nothing.

It still seeds `_lastPressed` and sets `_initialized`. That is what prevents a button **physically
held at boot** from reading as a press edge on the first `poll()` and firing an unwanted `TOGGLE`.

---

## Pin sources

All reads go through `PinRef::read()`, never `digitalRead()`, so the class works unchanged on:

| `PinRef::Type` | Path |
|---|---|
| `GPIO` | `digitalRead()` |
| `MCP` | `PanelGroup::readCachedPin()` — MCP23017 expander |
| `SR` | `ShiftBus::readBit()` — '165 input |

Polarity is interpreted in one place, `readPressed()`, used by both `poll()` and `forceReport()`.
Default is active-LOW (button to ground against board pull-up); `reverse = true` inverts it.
`configure()` does not enable internal pull-ups — board wiring supplies the bias.

`sampleTick()` is deliberately not overridden. `RotaryEncoder` overrides it because quadrature
transitions can be lost between loop iterations; a button press lasts 50–100 ms, far longer than any
loop period, so the cached read is always current enough.

---

## API

```cpp
class ActionButton : public InputBase {
public:
    static constexpr uint32_t DEBOUNCE_MS = 20;
    ActionButton(uint16_t controlId, PinRef pin, bool reverse = false);
    void configure()   override;
    void poll()        override;
    void forceReport() override;   // silent by design
};
```

---

## Tests

`Firmware/Tests/ActionButton/` — all envs need a **PB0→PA0 jumper**: PB0 is driven as an output to
act as the button, PA0 is the input. Driving it from software makes press, hold and bounce timing
exact while the read still goes through `PinRef::read()`, which a state-injection seam would have
bypassed.

| Env | Covers |
|---|---|
| `test_press_edge` | one EVT per press · held across 50 polls still one · release silent · re-arm |
| `test_force_report` | silent while released *and* pressed · boot-held seeds without firing · repeated `SYNC_REQ` silent · `poll()` inert before init |
| `test_debounce` | 8-transition bounce → one EVT · suppression not sticky |
| `test_reverse` | inverted active edge, held and release behaviour under `reverse` |

Transport and dispatch are covered outside this project, since the class alone cannot prove the
route: `Tests/CANProtocol/protocol_layout` (compile-time frame-ID assertions),
`Tests/CANProtocol/tx_batching` (the 5th `_batches[]` slot exists — a frame without one is silently
discarded), and `Tests/PanelBridge/input_dispatch_dcs` (selector `0` → `TOGGLE`, malformed selector
dropped, HID-range dropped, plus a real `0x700+n` frame fed through `onCanRx()` so the CAN range
branch itself is exercised).
