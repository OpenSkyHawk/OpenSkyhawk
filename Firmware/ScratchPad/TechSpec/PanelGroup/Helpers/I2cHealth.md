# I2cHealth — Technical Specification

**Status:** Firmware authored + compile-gated (hardware bench pending)
**FirmwarePlan ref:** issue #164 (non-blocking I2C — circuit breaker)
**Depends on:** `Arduino` (`millis()`)

---

## Responsibility

A **circuit-breaker mixin** ("trait") for I2C-backed device classes. A blocking I2C transaction to an
absent or dead device stalls `PanelGroup::loop()` and starves the node heartbeat — the bridge then
flaps the node online/offline (#164). Any OpenSkyhawk class that drives an I2C device **mixes this in**,
implements `i2cProbe()` (its own reachability check), and gates every I2C op behind `i2cReachable()`.

A dead device drops from *block every loop* to *one probe every `I2C_RETRY_MS`* (2 s), and the breaker
auto-recovers when the device returns. The class's data/decode path must stay I2C-free, so values stay
current while the device is down and the next reachable frame catches up — a clean **degrade**, not a
freeze.

This is the C++ analog of a trait: shared behaviour lives in the mixin; the contract (`i2cProbe()`) is
pure-virtual, so a mixing class cannot compile without honouring it.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Helpers/I2cHealth/I2cHealth.h     (header-only, all inline)
```

First consumer: `DrumDisplay` (mux + OLED). Exercised by `Firmware/Tests/DrumDisplay/tests/breaker/`
(forced-NAK seam — trip / back-off / recover, no hardware).

---

## Public / Protected API

```cpp
// Helpers/I2cHealth/I2cHealth.h  (inside #ifdef ARDUINO_ARCH_STM32)
class I2cHealth {
public:
    bool i2cHealthy() const;                 // breaker state — true while last probe reachable
protected:
    virtual bool i2cProbe() = 0;             // CONTRACT: probe reachability (impl per device)
    bool i2cReachable();                     // GATE: rate-limited probe; trips/heals; gate every I2C op
    static constexpr uint32_t I2C_RETRY_MS = 2000;   // back-off between retries while tripped
};
```

Usage:

```cpp
class Foo : public OutputBase, public I2cHealth {
    bool i2cProbe() override { /* mux ACKs && device ACKs */ }
    void update() override { /* … */ if (!i2cReachable()) return; /* … I2C … */ }
};
```

---

## Implementation Notes

```cpp
bool i2cReachable() {
    const uint32_t now = millis();
    if (!_i2cHealthy && (now - _i2cLastAttempt) < I2C_RETRY_MS) return false;  // tripped → back off
    _i2cLastAttempt = now;
    return (_i2cHealthy = i2cProbe());
}
```

- **Healthy** → probes on every call (cheap vs the gated payload, e.g. a 1 KB `sendBuffer`).
- **Tripped** → re-probes at most once per `I2C_RETRY_MS`; all other calls return `false` instantly, so
  a dead device costs ~one address probe every 2 s, not a blocking transaction every loop.
- The dtor is **protected, non-virtual** — a mixin is never deleted through this type.

`i2cProbe()` should be cheap (a single bounded address probe) and record any fault detail the device
wants to report (e.g. mux vs device) for node health (#163).

### Bounding each transaction

The breaker bounds how *often* a dead device is probed, not how long a single transaction blocks. On a
stuck or floating bus one `endTransmission()` can still block on the STM32 core's default I2C timeout
(`I2C_TIMEOUT_TICK`, 100 ms). Pair the breaker with a build-flag override on any I2C node —
**`-DI2C_TIMEOUT_TICK=10`** — so each transaction is bounded to ~10 ms. Healthy OLED/MCP/ADS transfers
are well under that; a dead device is skipped by the breaker, not transacted.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| millis() | Arduino core | back-off timing; each device keeps no extra clock |
