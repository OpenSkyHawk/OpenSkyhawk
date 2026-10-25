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

**Clock — the 8 MHz crystal must be actively selected (issue #245):** the STM32duino core
default runs SYSCLK on internal HSI-PLL at 64 MHz, which drops APB1 to 32 MHz and skews CAN to
444 kbps — even though the crystal is fitted and `-DHSE_VALUE=8000000` is set (that flag only
declares the frequency to HAL, it does not select HSE). `STM32Board` supplies a strong
`SystemClock_Config` that selects HSE → PLL ×9 → **72 MHz** (APB1 36 MHz → CAN 500 kbps) and
verifies the configured tree. A **detectable** clock fault — dead crystal / cold joint (HSERDY
timeout) or a self-inconsistent config — latches and shows the **WARNING** status-LED pattern (top
precedence) plus a `CLOCK FAULT` diag line, rather than a misleading CAN error. All STM32 nodes
inherit this by linking `STM32Board`. **Every board must use an 8 MHz crystal** — a wrong value
still oscillates, so it is **NOT** runtime-detected (the readback uses the compile-time `HSE_VALUE`,
not a measurement): a 12 MHz part would run 108 MHz / CAN 750 kbps while logging `CLOCK OK`. Enforce
8 MHz at the BOM/build level.

**ADC clock — the same override owns `ADCPRE` (issue #263):** APB2 runs undivided at **72 MHz**, and
`ADCPRE` resets to `/2`, so ADCCLK booted at **36 MHz against the F103's 14 MHz maximum**. Nobody was
setting it: on F1 the STM32duino core deliberately defers the ADC prescaler to `SystemClock_Config`
(its `__HAL_RCC_ADC_CONFIG` call is guarded `!defined(STM32F1xx)`), and the `variant_generic.cpp`
that our strong override displaces never set it either. `STM32Board` now configures
`RCC_ADCPCLK2_DIV6` → **ADCCLK 12 MHz** on **both** exit paths of `SystemClock_Config` (`/4` = 18 MHz
is still out of spec; the fault path's 8 MHz APB2 gives 1.33 MHz, above the 0.6 MHz minimum). It runs
before `setup()`, so the framework's own `analogRead()` inherits it. Conversions take 3× longer
(26 cycles: 0.72 µs → 2.17 µs), which is irrelevant against `AnalogInput::POLL_MS` = 8 ms. Nothing
but the ADCs consumes ADCCLK, so CAN, the UARTs, SPI/ShiftBus and the timers are untouched.

> **This fixed nothing observable, and the docs should not imply otherwise.** Axis noise, PA2, and
> die-temperature telemetry were each predicted to improve and each measured *unchanged* on the
> assembled PanelGroup Rev 1 — the figures and the unresolved potentiometer question are in
> `00-decisions.md` **D15**. The justification is that the part was out of spec, plus
> high-impedance sources in general — which has **not** been measured on an actual pot.

**Why UART2 (PA2/PA3) on PanelBridge:** Remapping `Serial` to `Serial1` (PA9/PA10) causes
"multiple definition of Serial2" compile errors or runtime failures with STM32duino. With no
CDC flag, `Serial` maps natively to UART2 on PA2/PA3 — use that.

**SWD programming header:** expose PA13, PA14, NRST, GND, and 3.3V on a 5-pin header on
every board. Full JTAG pins (PA15, PB3, PB4) are not needed — SWD is sufficient, and
`STM32Board::begin()` releases JTAG at boot (SWD kept) so those three are ordinary GPIO (#299).

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

The `StepperMotor` driver (composed by `NeedleGauge`) owns the `~SLEEP` / enable line.
`PanelGroup::setup()` → `NeedleGauge::configure()` → `motor.configure()` drives it HIGH before the
homing sequence (`motor.home()`).

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
- Raw single-ended reading: 0–32767 (15-bit). At `GAIN_ONE` 3.3V ≈ 26400; firmware maps 0–3.3V → 0–65534 (#325)
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

## PDU Monitoring Node — Analog Front End & Calibration Page (#202 / #216)

The PDU (`PCB/Base/Power/PDU`, Rev 0.2.0) is an STM32F103C8 node running the `PowerMonitor` library
(`Firmware/Panels/PDU_Left|Right|Center`, NODE_IDs 3/4/5). Pin map — verified against the Rev 0.2.0
netlist; overridable as `PDU_PIN_*`:

| Pin | Net | Front end | Firmware conversion |
|---|---|---|---|
| `PA0` | `12V_READ_ADC` | 33k / 10k divider (0.1 %) → 1k → 100 nF | rail = pin × 43 / 10 |
| `PA1` | `5V_READ_ADC` | 10k / 13k divider (0.1 %) → 1k → 100 nF | rail = pin × 23 / 13 |
| `PA2` | `I_12V_ADC` | 10 mΩ shunt → INA180A2 (×50) → 1k → 100 nF | mA = 2 × pin mV |
| `PA3` | `I_5V_ADC` | 10 mΩ shunt → INA180A2 (×50) → 1k → 100 nF | mA = 2 × pin mV |
| `PA6` | `NTC_ADC` | 10k from +3V3, NTC 10k B3450 ±1 % to GND → 1k → 100 nF | ratiometric Beta equation |
| — | Vrefint (ch17) | internal | VDDA correction once calibrated |

- **VDDA.** The ADC reference is VDDA (the AMS1117 3V3 through FB1). Uncalibrated, the firmware assumes
  **3300 mV**; the F103 Vrefint is untrimmed (±~3 %) and worse than that nominal until anchored. A
  one-time `CAL VDDA <mV>` (DMM reading of the 3V3 rail) stores this chip's Vrefint; VDDA then
  tracks it. The NTC divider runs from the same rail, so temperature is ratiometric and needs no VDDA.
- **Current zero.** `CAL ZERO` (no load connected) stores each rail's INA180 offset. The PDU's own
  regulator, CAN transceiver and LED sit **after the 5 V shunt** (~30–50 mA), so the 5 V zero
  subtracts the node's own draw and the 5 V reading is *downstream load only*; the refusal guard is
  therefore 120 mA on 5 V vs 75 mA on 12 V.
- **Filtering.** The 1 kΩ / 100 nF on each channel (τ ≈ 100 µs) is the anti-alias filter for the
  1 kHz rail scan; reported peak current is after it.
- **A blown 5 V fuse powers the node down** (the AMS1117 is fed from the fused 5 V), so only the 12 V
  rail has a fuse-open check; the host sees a dead 5 V fuse as the node going offline.
- **Calibration flash page.** The last 1 KB page, `0x0800FC00`, holds an append-only log of 32-byte
  CRC-checked records (newest valid wins; erased only when full). The PDU builds set
  `board_upload.maximum_size = 64512`, so the build fails before code can grow into the page, and an
  OpenOCD upload erases only the sectors it writes — a reflash keeps the calibration. A full-chip
  erase (e.g. STM32CubeProgrammer) wipes it; re-run the calibration afterwards.
- **LEDs.** Rail faults are reported to the host only (`TELEM_n` flags + `HEALTH_n.faultId`). The
  MCU status LEDs (D6 red / D7 green on PB14 / PB15) behave as on every node — CAN / link + the
  master-loss WARNING — and don't change for rail faults (D14). The power LEDs D3 (+12 V), D4 (+5 V)
  and D5 (+3V3) are passive, resistor-fed from their rails and not firmware-driven: they show rail
  presence only — dark on a dead rail or blown fuse, lit through OV / OC / overheat.

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
`canIdHb(0)` every 500 ms. Every other node accepts it (CANProtocol adds it to the filter) and,
once it has seen a master, raises WARNING if `HB_0` stops for > 1500 ms — both sides run in
`CANProtocol::serviceNodeStatus()` (#340). This is independent of
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
(arduino-pico `SerialUART` surfaces no error flags), latched for ≥2 s, and clears when clean UART
data resumes after the fault — or, on a silent bus, after 3 s by restarting the UART (#355). Full
behaviour:
`07-simgateway-api.md` → *Status LEDs*.

---

## Cross-Document Issues

See `11-open-issues.md` for the full list. Active hardware/firmware discrepancy:

| Issue | Location | Status |
|-------|----------|--------|
| `~SLEEP` strategy (HIGH from setup vs. HIGH after DCS) | `hardware-standards.md` vs. `08-hardware-firmware-contracts.md` §DRV8833 | ✅ Fixed — `hardware-standards.md` updated |
