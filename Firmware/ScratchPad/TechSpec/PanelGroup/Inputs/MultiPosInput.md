# MultiPosInput — Technical Specification

**Status:** Done (hardware-verified — 10/10 scenarios PASS 2026-06-22)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switchmultipos-new`, `#analogmultipos-new`
**Depends on:** `PanelGroup.md`

> `MultiPosInput` is the shared base factored from the MULTIPOS input family. It is not a
> separate FirmwarePlan entry — the FirmwarePlan defines the per-class behaviour
> (`SwitchMultiPos`, `AnalogMultiPos`); this base captures what they have in common.

---

## Responsibility

Base class for the **MULTIPOS input family** — selector inputs that emit an absolute position
index 0..N-1 over CAN. Owns the contract these inputs share:

- a subclass reports the instantaneous resolved position via `readRaw()`;
- this base **debounces** the index (configurable window) and emits a CAN EVT **only when the
  confirmed position changes** (emit-on-change);
- the emitted value is the **absolute** index, never a delta — a jump from any position to any
  other (even one that skips intermediates) emits the new index directly. There is no ±1 or
  adjacency assumption, and nothing is lost on a skip.

`readRaw()` returns `NO_POSITION` when nothing currently resolves (e.g. a non-shorting rotary
mid-throw with no contact closed); the base then **holds the last confirmed position** — no
spurious EVT.

Subclasses: `SwitchMultiPos` (one-hot digital pins), `AnalogMultiPos` (resistor ladder, #114).

Does **not** read hardware, own any `PinRef`, interpret `controlId`, or talk to PanelBridge.

---

## File Layout

```
Firmware/Libraries/PanelGroup/Inputs/MultiPosInput/
├── MultiPosInput.h
└── MultiPosInput.cpp
```

No standalone test project — exercised through its concrete subclasses (`Tests/SwitchMultiPos/`).

---

## Public API

```cpp
class MultiPosInput : public InputBase {
public:
    static constexpr uint16_t NO_POSITION = 0xFFFF;  // readRaw() → "nothing active, hold last"

    void poll() override;          // resolve, debounce, emit on confirmed change
    void forceReport() override;   // resolve + emit unconditionally (no debounce); set baseline
    uint16_t position() const;     // last confirmed index 0..N-1 (query + test assert)
#ifdef MULTIPOS_TEST
    uint16_t emitCount() const;    // test seam — count of EVTs emitted
#endif

protected:
    MultiPosInput(uint16_t controlId, uint8_t numPositions, uint16_t debounceMs);
    virtual uint16_t readRaw() = 0;   // resolved index 0..N-1, or NO_POSITION

    uint16_t _controlId;
    uint8_t  _numPositions;

private:
    void emit(uint16_t pos, bool init = false);
    uint16_t _debounceMs, _lastPos, _pendingPos;
    uint32_t _debounceStartMs;
    bool     _initialized;
};
```

---

## Key Data Structures

`NO_POSITION = 0xFFFF` — the `readRaw()` sentinel meaning "no position is currently active".
A real index is always `< _numPositions` (≤ ~10 in practice), so the value can never collide.
It shares the same numeric value as `AnalogMultiPos`'s `ANALOG_NC` by design.

`poll()` and `forceReport()` defensively coerce any **out-of-range** `readRaw()` result
(`≥ _numPositions`, e.g. from a buggy subclass) to `NO_POSITION`, so a garbage index never reaches
CAN — the family invariant is enforced once, in the base.

All state is per-instance; no statics.

---

## Implementation Notes

### Debounce + emit-on-change (the whole contract)

```cpp
void MultiPosInput::forceReport() {
    uint16_t raw = readRaw();
    if (raw == NO_POSITION) raw = 0;   // nothing active at boot → default to position 0
    _lastPos = _pendingPos = raw;
    _debounceStartMs = millis();
    _initialized = true;
    emit(raw, true);                   // unconditional baseline emit
}

void MultiPosInput::poll() {
    if (!_initialized) return;
    uint16_t raw = readRaw();
    if (raw == NO_POSITION) raw = _lastPos;            // hold last confirmed position

    if (raw != _pendingPos) {                          // index changed → (re)start the window
        _pendingPos = raw;
        _debounceStartMs = millis();
    } else if (_pendingPos != _lastPos &&
               millis() - _debounceStartMs >= _debounceMs) {
        _lastPos = _pendingPos;                        // stable ≥ window AND different → confirm
        emit(_lastPos);
    }
}
```

Two layers act together: the **debounce** decides *what counts as the position* (an index must
hold steady for `_debounceMs` before it is accepted), and **emit-on-change** decides *whether to
send* (only when the accepted index differs from the last one emitted). A steady selector emits
nothing; one move emits exactly one frame.

### Hold-last, jump-safe, fast-sweep

- **Hold-last:** `readRaw()` → `NO_POSITION` during a contact gap is mapped to `_lastPos`, so a
  non-shorting rotary mid-throw produces no EVT.
- **Jump-safe / skip-safe:** the value is the absolute index. A direct move from position *p* to
  *q* (intermediates never read) confirms and emits *q* — no walking, no adjacency assumption.
- **Fast sweep:** spinning through intermediate detents each held `< _debounceMs` never confirms
  them; only the settled position emits. Dwelling on a detent for the window emits it.

### Configurable debounce window

`_debounceMs` is set by the subclass: `SwitchMultiPos` passes 20 ms (absorb contact bounce /
throw transients); `AnalogMultiPos` (#114) passes 0 because its analog deadband already provides
hysteresis. `0` means "confirm on the next poll where the index is unchanged".

### EVT transmission

`emit()` calls `CANProtocol::sendBatched(canIdEvt(NODE_ID), ControlPacket{_controlId, pos})` —
identical mechanism to `Switch2Pos`. PanelBridge routes the EVT as `MULTIPOS`
(`sendDcsBiosMessage(name, itoa(pos))`); SimGateway needs no declaration. Debug builds log
`[MUL] 0x<id>: <pos>` (with `(init)` on the boot burst).

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | InputBase, canIdEvt, NODE_ID |
| CANProtocol | `Firmware/Libraries/CANProtocol` | ControlPacket, sendBatched |
