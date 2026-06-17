# 08 — Hardware / Firmware Contracts

**Owns:** STM32 pin assignments, peripheral conflict constraints, DRV8833 ~SLEEP contract,
ADS1115 wiring assumptions, cross-document discrepancy log.
**Does not own:** component package selection or full LED design (→ `hardware-standards.md`),
boot sequence (→ 09), CAN protocol (→ 02).

---

## STM32F103C8 — Reserved Pin Table

The following pins are reserved across **all** STM32 boards. No panel-specific schematic may
assign these pins to other functions.

| Pins | Function | Active on |
|------|----------|-----------|
| PA2 / PA3 | UART2 (`Serial`) — RP2040 serial link | PanelBridge only |
| PA9 / PA10 | USART1 (`Serial1`) — DiagSerial | All STM32 boards |
| PA11 / PA12 | CAN bus → SN65HVD230 | All STM32 boards |
| PA13 / PA14 | SWD programming (SWDIO / SWCLK) | All STM32 boards |
| PB6 / PB7 | I2C1 — `Wire` (SCL / SDA) | All STM32 boards |
| PB10 / PB11 | I2C2 — `Wire1` (SCL / SDA) | All STM32 boards |
| PB14 | Status LED — Red | All STM32 boards |
| PB15 | Status LED — Green | All STM32 boards |
| PD0 / PD1 | 8 MHz crystal (OSC_IN / OSC_OUT) | All STM32 boards |
| PC13 / PC14 / PC15 | Unusable — RTC tamper / 32kHz oscillator / current limits | All STM32 boards |

**ADC pins (PA0–PA7, PB0–PB1):** reserved as a scarce resource for panel-specific analog
inputs. Do not assign to digital functions — each board's schematic allocates them as needed.

**PA11/PA12 shared between CAN and USB:** USB is disabled at firmware init and CAN takes the
pins — they do not conflict at runtime. USB may be enabled for firmware flashing/debug only.

**Why UART2 (PA2/PA3) on PanelBridge:** Remapping `Serial` to `Serial1` (PA9/PA10) causes
"multiple definition of Serial2" compile errors or runtime failures with STM32duino. With no
CDC flag, `Serial` maps natively to UART2 on PA2/PA3 — use that.

**SWD programming header:** expose PA13, PA14, NRST, GND, and 3.3V on a 5-pin header on
every board. Full JTAG pins (PA15, PB3, PB4) are not needed — SWD is sufficient.

---

## UART Pin Assignments (PanelBridge ↔ SimGateway)

| Signal | STM32 pin | RP2040 pin |
|--------|-----------|-----------|
| TX (STM32 → RP2040) | PA2 | GP1 (UART0 RX) |
| RX (STM32 ← RP2040) | PA3 | GP0 (UART0 TX) |
| GND | shared GND | shared GND |

Both sides are 3.3 V — no level shifter required.

---

## DRV8833PW — ~SLEEP Pin Contract

`~SLEEP` must be **driven HIGH from `setup()`**, before the homing sequence runs. The motor
requires active driver output for homing torque. `~SLEEP` remains HIGH permanently after that
point.

The `SwitecX25Output` and `AccelStepperOutput` constructors accept a `SLEEP_PIN` argument.
`PanelGroup::setup()` drives it HIGH before calling `motor.reset()` or starting the homing
sequence.

**Correct behaviour:** HIGH from `setup()` for homing torque. `hardware-standards.md` has
been updated to reflect this.

---

## MCP23017 — Electrical Constraints

For wired-OR interrupt lines (topology B — see `05-panelgroup-api.md`):

- `IOCON.ODR = 1` (open-drain output) — set by PanelGroup automatically for shared-pin chips
- `IOCON.INTPOL = 0` (active-LOW) — default
- STM32 GPIO: `INPUT_PULLUP` (~40 kΩ internal; external 10 kΩ if > 4 chips share one line)
- 100 Ω series resistors on each INTA/INTB line (specified in `hardware-standards.md`
  MCP23017 circuit block)

**Interrupt pin exclusions:** PB14/PB15 are reserved by STM32Board for the bi-color status LED.
PC13/PC14/PC15 are not assignable on OpenSkyhawk STM32 boards. Do not use any of these pins
for MCP23017 interrupt lines.

---

## ADS1115 — Wiring Assumptions

- I²C addresses: 0x48–0x4B (ADDR pin to GND, VDD, SDA, SCL respectively)
- 4 channels per chip, up to 4 chips per I²C bus (16 channels/bus)
- STM32F103 has two I²C buses (`Wire` I2C1 PB6/PB7; `Wire1` I2C2 PB10/PB11)
- Raw single-ended reading: 0–32767 (15-bit). Firmware multiplies ×2 → 16-bit (0–65534)
- Input filter required per channel: 1 kΩ series + 100 nF to GND (specified in `hardware-standards.md`)

---

## CAN Transceiver — SN65HVD230

- SOIC-8, 3.3 V logic
- Connects to STM32 CAN controller: PA11 (RX) / PA12 (TX)
- CANH/CANL route to Molex Mini-Fit Jr main bus connector (pins 5/6)
- Bus termination: 120 Ω across CANH/CANL at each **end node** only; omit on intermediate nodes
- Compatible 3.3 V clones (e.g. VP230) acceptable for JLCPCB assembly

---

## DiagSerial — USART1

**Standard across all STM32 boards:** USART1, `Serial1`, PA9 (TX) / PA10 (RX), 115200 baud.

Every PCB exposes a 3-pin header (GND / RX / TX) for a USB-to-TTL converter. Connect a
USB-to-TTL adapter to observe a board's diagnostic output independently of the DCS data path.

USART1 is chosen because PanelBridge already uses UART2 (`Serial`, PA2/PA3) for the RP2040
link — the two ports never conflict.

When `STM32Board::setDebug(true)` is called in `setup()`, `Serial1` emits human-readable debug
output. Production builds use `setDebug(false)` (default) — pins remain available but silent.

---

## Status LED — Bi-Color (Red/Green)

Each custom OpenSkyhawk STM32 board has a bi-color red/green status LED driven by two dedicated
GPIO pins. The pins are fixed across all STM32 boards and defined inside `STM32Board`:

| Pin | LED channel |
|-----|-------------|
| PB14 | Red |
| PB15 | Green |

No sketch configuration is needed — `STM32Board::begin()` initialises both pins automatically.
Existing Blue Pill / PC13 status LED behaviour is legacy prototype-only and must not be used
for custom OpenSkyhawk controller boards.

| Red | Green | Visible | Meaning |
|-----|-------|---------|---------|
| OFF | OFF | Off | Not powered |
| Slow blink | OFF | Red blinking | Booting / initialising |
| OFF | Slow blink | Green blinking | Normal — CAN healthy, no data flowing |
| OFF | Solid on | Green solid | **Connected — CAN healthy and data flowing** |
| Fast blink | OFF | Red fast | TEC > 0 — transmit errors accumulating |
| ON | OFF | Red solid | Bus-off — CAN controller halted |
| Alternating | Alternating | Amber flicker | Warning / degraded state |

The state machine and animations are **shared** in `STM32Board`; PanelBridge and PanelGroup
differ only in the role triggers that drive `setLinkActive()` / `setWarning()`:

| State | PanelBridge trigger | PanelGroup trigger |
|-------|---------------------|--------------------|
| Connected (green solid) | a DCS-BIOS export seen → `setLinkActive(true)` | a `CTRL_BCAST` received → `setLinkActive(true)` |
| Warning (amber) | a tracked PanelGroup node died | master heartbeat (`HB_0`) lost (timeout) |

**CONNECTED & decay:** `setLinkActive(true)` enters the green-solid CONNECTED state; it decays
back to NORMAL (green slow) after ~500 ms with no further data. CONNECTED shows only while the
bus is healthy — a CAN fault masks it and it re-engages automatically on recovery.

**Master heartbeat (`HB_0`):** PanelBridge transmits an unconditional liveness beacon on
`canIdHb(0)` every 500 ms. PanelGroup accepts it (`filterAcceptId(canIdHb(0))`) and, once it
has seen a master, raises WARNING if `HB_0` stops for > 1500 ms. This is independent of
`CTRL_BCAST` (which only moves when DCS export values change), so an idle/paused sim does not
trigger a false no-master warning.

**Triggering WARNING:** call `STM32Board::setWarning(true)` for any non-CAN fault (dead node,
lost master, SYNC timeout, I²C hang) and `setWarning(false)` once it recovers. `CanStatus` has
no WARNING value; CAN bus faults are communicated via `onCanStatus()` only.

---

## Status LED — SimGateway (RP2040)

The `Gateway_Bridge` board also carries two SimGateway status LEDs on the RP2040, separate from
the STM32 bi-color LED above. Pins are fixed and configured by `SimGateway::statusLedBegin()`
(called from `setup()`); ticked by `SimGateway::statusTick()` from `loop()`. Active-high. The
RP2040 module's onboard WS2812 is not used.

| Pin | LED channel |
|-----|-------------|
| GP2 | Green |
| GP3 | Red |

| State | Trigger | Red | Green | Visible |
|-------|---------|-----|-------|---------|
| `FAULT` | uart0 RX hardware error (RSR overrun/framing/parity/break) | Fast blink | OFF | Red fast (4 Hz) |
| `NO_HOST` | USB not enumerated, or unplugged after a mount | ON | OFF | Red solid |
| `STREAMING` | DCS-BIOS bytes from the PC within last 500 ms | OFF | ON | Green solid |
| `USB_IDLE` | USB mounted, no recent DCS data | OFF | Slow blink | Green slow (1 Hz) |
| `INIT` | booted, pre-USB-mount (brief) | Slow blink | OFF | Red slow |

Priority is highest-first (`FAULT` > `NO_HOST` > `STREAMING` > `USB_IDLE` > `INIT`). This is a
**SimGateway-local** engine (issue #94); it shares the animation vocabulary (OFF / SOLID / SLOW
1 Hz / FAST 4 Hz / ALT / PULSE) with `STM32Board` (issue #93) but not the code — the shared
`StatusLedAnimator` was not adopted. `FAULT` is read from the `uart0` PL011 `RSR` register
(arduino-pico `SerialUART` surfaces no error flags), latched for ≥2 s, and clears only when clean
UART data resumes after the fault — a silent bus holds it. Full behaviour:
`07-simgateway-api.md` → *Status LEDs*.

---

## Cross-Document Issues

See `11-open-issues.md` for the full list. Active hardware/firmware discrepancy:

| Issue | Location | Status |
|-------|----------|--------|
| `~SLEEP` strategy (HIGH from setup vs. HIGH after DCS) | `hardware-standards.md` vs. `08-hardware-firmware-contracts.md` §DRV8833 | ✅ Fixed — `hardware-standards.md` updated |
