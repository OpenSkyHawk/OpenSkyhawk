# I2cMux — Technical Specification

**Status:** Firmware authored + compile-gated (hardware bench pending)
**FirmwarePlan ref:** issue #113 (OLED drum readout — multi-panel support)
**Depends on:** `Wire` (Arduino I²C)

---

## Responsibility

Selects one downstream channel of a **TCA9548A** 1-to-8 I²C multiplexer. One `I2cMux` instance ==
one TCA9548A chip on one I²C bus. A `DrumDisplay` (or any future muxed I²C output) holds a pointer
to an `I2cMux` plus a channel index and calls `select()` before every I²C transaction so the right
downstream branch is live. This lets many same-address devices (e.g. SSD1306/SH1106 OLEDs, which
only offer addresses 0x3C/0x3D) share one bus — eight per mux, chained for more.

The mux is a **passive switch**: the device driver (U8g2) still owns the device address; the mux
only routes SDA/SCL. `I2cMux` performs **no I²C in its constructor** and never calls `Wire.begin()`
— the sketch owns bus startup.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Helpers/I2cMux/I2cMux.{h,cpp}
```

Exercised by `Firmware/Tests/DrumDisplay/tests/mux/` (two panels on channels 0/1).

---

## Public API

```cpp
// Helpers/I2cMux/I2cMux.h  (inside #ifdef ARDUINO_ARCH_STM32)
namespace OpenSkyhawk {

class I2cMux {
public:
    explicit I2cMux(uint8_t addr = 0x70, TwoWire& wire = Wire);  // no I²C here
    bool select(uint8_t channel, bool force = false);  // route to 0–7; force = uncached write (recovery)
    void disableAll();              // control byte 0x00 (optional bus quiescing)
    bool deviceAcks(uint8_t addr7); // probe a device on the CURRENTLY selected channel
};

}  // namespace OpenSkyhawk
```

---

## Implementation Notes

### Last-channel cache

```cpp
bool I2cMux::select(uint8_t channel) {
    if (channel > 7) channel = 7;
    if (static_cast<int8_t>(channel) == _lastChannel) return true;  // skip redundant write
    _wire->beginTransmission(_addr);
    _wire->write(static_cast<uint8_t>(1u << channel));
    bool ok = (_wire->endTransmission() == 0);
    if (ok) _lastChannel = static_cast<int8_t>(channel);
    return ok;
}
```

`select()` writes the 1-of-8 channel bitmask only when the requested channel differs from the last
written one, so repeated `select()` of the same channel costs no I²C. `_lastChannel` starts at −1
(nothing selected). Callers sharing one mux across several devices **must** call `select()`
immediately before each downstream I²C op — an interleaved driver can change the live channel.

### Health probes + recovery (circuit breaker)

For the I2C circuit breaker (`I2cHealth`, #164), a muxed output probes reachability with a **forced**
`select(channel, /*force=*/true)` then `deviceAcks(deviceAddr)`:

- **`select(channel, force=true)`** writes the channel byte unconditionally (bypassing the
  last-channel cache) and returns the mux's ACK — this confirms the mux is alive **and** actually
  re-routes the branch. Critical for recovery: a TCA9548A that reset / power-glitched comes back with
  no channel selected while the cache still matches, so a plain cached `select()` would skip the write
  and the branch would stay dark forever. On a NAK, `select()` also **invalidates the cache**
  (`_lastChannel = -1`) so the next call re-writes.
- **`deviceAcks(addr7)`** then probes the device on the now-selected branch.

A NAK on the forced select ⇒ the mux is gone; a NAK on `deviceAcks` ⇒ the device is gone — the order
classifies *which* hop failed, for node health reporting (#163). Pair with a short `-DI2C_TIMEOUT_TICK`
build flag: the breaker bounds probe *frequency*, the timeout bounds each *transaction* so a
stuck/floating bus can't block the loop.

### Why a separate helper

DrumDisplay holds an `I2cMux*` (nullptr for direct-bus instances) and a channel. The direct-bus
constructor omits the mux entirely, so single-panel nodes pay nothing. Keeping the selector
separate means any future muxed I²C peripheral reuses it.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| Wire | Arduino core | I²C transport; bus startup owned by the sketch |
