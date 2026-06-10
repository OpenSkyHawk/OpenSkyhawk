# Hardware Standards

## Component Package Rules

**Constraint: all packages must be visually inspectable after reflow.** T962 reflow oven is available; the limitation is inspection, not soldering.

| Acceptable | Not acceptable |
|------------|---------------|
| SOIC, SSOP, TSSOP, HTSSOP | QFN, DFN, WSON |
| LQFP | BGA, LGA |
| SOT-23, SOT-223 | Any fully-bottom-terminated package |
| Through-hole | |

HTSSOP (exposed thermal pad on underside) is acceptable — side leads are the critical joints; the GND pad is verified by continuity check.

## MCU

**Selected: STM32F103CBT6** — LQFP48, 128 KB flash, 20 KB RAM.

- Requires **external 8 MHz crystal** for reliable CAN bus timing (internal RC oscillator is not accurate enough for CAN bit-rate lock)
- **PA11/PA12** are shared between USB and CAN — the two peripherals cannot be used simultaneously; pick one at firmware init
- CAN bus is the primary inter-board protocol; USB used only for initial flashing/debug
- Use the bare die (not Blue Pill module) on MCU boards — fewer passive conflicts, smaller footprint

## RP2040 HID Controllers

**Role:** USB HID devices connecting directly to the PC — flight stick, throttle, rudder pedals, button boxes. No custom PCB; use off-the-shelf modules.

### Module Selection

| Module | Notes |
|--------|-------|
| Raspberry Pi Pico 2 | RP2350; preferred for new builds |
| Raspberry Pi Pico | RP2040; proven, widely available |
| Tiny2040 (Pimoroni) | Compact; USB-C; good for tight enclosures |
| RP2040 Zero (Waveshare) | Smallest footprint |

Choose by mechanical fit. All are equivalent for HID use.

### ADC Inputs

- 4 usable ADC channels (GPIO26–29); GPIO29 is used for VSYS monitoring on Pico modules — prefer GPIO26–28 for axes
- 12-bit ADC, 0–3.3V range
- Apply same RC input filter as STM32: 1 kΩ series + 100 nF to GND per axis input

**Analog expansion via ADS1115 (worth prototyping):** When more than 3 axes are needed, add ADS1115 breakout(s) over I²C — the same part already used on STM32 breakout boards.

- 4 channels per chip, 16-bit, I²C
- Up to 4 chips per bus (addresses 0x48–0x4B via ADDR pin) = 16 channels per I²C bus
- RP2040 has two I²C buses; effectively unlimited axes in practice
- Library: Adafruit ADS1X15 (same as STM32 side — no new dependency)
- Use same RC filter (1 kΩ + 100 nF) on each ADS1115 input
- Prototype note: verify latency vs. onboard ADC for axis polling at HID report rate (~1 ms)

### UART (Gateway Role Only)

The RP2040 in the flight stick also bridges USB CDC serial to the CAN gateway STM32 via UART. See "Dual-MCU Architecture" in `architecture.md` for pin assignments and packet format. Non-gateway RP2040 boards (throttle, rudder, button boxes) do not use UART.

### Power

Bus-powered from USB. No 12V supply needed. If co-located with STM32 CAN hardware, share GND only — do not share 3.3V rails between the RP2040 module and STM32 board.

### Toolchain

PlatformIO with `earlephilhower/arduino-pico` platform (preferred) or Arduino IDE with Arduino-Pico core. TinyUSB is bundled with Arduino-Pico; use it for USB HID + CDC composite.

## Power Supply

**Architecture: PC ATX PSU distributes 12 V and 5 V on the main bus. Each board generates 3.3 V locally.**

| Stage | Part | Package | Notes |
|-------|------|---------|-------|
| Main bus | PC ATX PSU | — | 12 V and 5 V distributed via Molex Mini-Fit Jr bus connectors |
| 5 V → 3.3 V | AMS1117-3.3 | SOT-223 | LDO on every MCU and breakout board; 5→3.3 V drop (1.7 V) acceptable at ≤175 mA load |
| 5 V rail | also feeds | — | DRV8835 VM (stepper driver motor supply) |
| 12 V → 5 V | AP63205WU | SOT-23-6 | **Only on high-5V-current boards** (future actuator/high-power boards). Not used on standard MCU or breakout boards. Typical BOM: C_in 10 µF, C_bypass 100 nF, L 4.7 µH, C_out 2×22 µF |

Local decoupling required on every board: 100 nF + 10 µF per rail, placed close to each IC. Never use a linear regulator for 12 V → 5 V conversion.

## Stepper Driver

**Selected: DRV8833PW (TI)** — dual H-bridge, HTSSOP-16.

DRV8835 was considered but is only available in WSON-12 (fully bottom-terminated, not inspectable after reflow). DRV8833PW is the same electrical capability in an inspectable HTSSOP-16 package.

- Drives one X27.589 Switec stepper (bipolar, ~600 steps/315°, 180–300 Ω coils, ~15–30 mA at 5 V)
- VM supply: 2–10.8 V (use 5 V); VINT: 3.3 V output (bypass with 100 nF to GND — do not drive with external supply)
- VCP (charge pump): 100 nF cap to VM — required for internal charge pump
- **~SLEEP pin (active LOW)**: driven HIGH from `setup()` before the homing sequence runs — motor requires active driver output for homing torque. Remains HIGH permanently after that. Anti-twitch is achieved by homing to position 0 before DCS connects, not by disabling the driver.
- AISEN / BISEN: current sense inputs — tie to GND (no current regulation needed; X27.589 coil resistance limits current naturally at 5 V)
- ~FAULT: open-drain fault output — leave NC in this revision
- No current-regulation passives needed; X27.589 coil resistance limits current naturally at 5 V
- Firmware: use **SwitecX25** library (handles homing/reset); AccelStepper does not implement homing
- KiCad symbol: `Driver_Motor:DRV8833PW` (built-in)

## Screws

| Screw | Use |
|---|---|
| M2 | PCB mounts, small standoffs |
| M3 | Placards, light rings, small brackets |
| M4 | Instrument bezels, gauge mounts — clearance Ø4.3–4.5 mm |
| M5 | Panel-to-subpanel, corner mounts — clearance Ø5.3–5.5 mm |

## Connectors

| Series | Pitch | Use | Tooling |
|---|---|---|---|
| Molex Mini-Fit Jr | 4.2 mm | Main bus (CAN + power between controller groups) **and** LED power connectors on all boards | JRready ST6490-ACT |
| JST-XH | 2.54 mm | Signal/logic harnesses — MCU ↔ breakout (I²C, interrupts, analog) and switch wiring | Engineer PA-09 |

**Minimum pitch: 2.54 mm.** Nothing smaller is used anywhere in the build.

**Wire gauge: 24 AWG throughout.**

### Molex Mini-Fit Jr (main bus + LED power)

- **PCB footprint:** Through-hole, single-row vertical for LED connectors (1×2); dual-row vertical for main bus
- Main bus: CAN bus signals and power distribution between controller groups
- LED power: 2-pin (1×2) per lighting zone on every MCU and breakout board — carries `+12V_BACKLIGHT` and `BACKLIGHT_SW_RETURN`
- Polarized housing — one insertion orientation only

**Main bus connector: 2×4 (8-pin), `Connector_Molex:Molex_Minifit_Jr_5557-08A2_2x04_P4.20mm_Vertical`**

Two identical connectors per MCU board (J_BUS_IN + J_BUS_OUT) — same nets, bus passes through.

| Pin | Signal | Notes |
|---|---|---|
| 1 | +12V | Always-on 12V from PSU |
| 2 | +12V | Parallel — reduces connector resistance for LED backlight current |
| 3 | +5V | From PSU — 3.3V generated locally via AMS1117 |
| 4 | GND | |
| 5 | CANH | Differential pair with pin 6 |
| 6 | CANL | Differential pair with pin 5 |
| 7 | GND | |
| 8 | GND | |

Pins 1/2 both connect to +12V net. Pins 4/7/8 all connect to GND plane. CANH/CANL on pins 5/6 (same row) for clean differential pair routing.

### JST-XH (intra-group harnesses + switch wiring)

- **PCB footprint:** Through-hole, single-row, vertical
- **Standard sizes:** 4-pin, 6-pin, 8-pin — choose by pin count, leave no pins empty
- Rated 3A per pin — sufficient for 12V LED lines and all signal/power within a controller group
- Polarized housing — one insertion orientation only
- Switches share a common GND within each connector group; one GND pin per connector

Standard intra-group signal harness (6-pin JST-XH):

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | 3.3 V (chip power) |
| 6 | spare |

Breakout boards with analog outputs or interrupts use an 8-pin variant with additional signals on pins 6–8.

LED power is carried on a **separate 2-pin Mini-Fit Jr connector** (not the signal harness):

| Pin | Signal |
|---|---|
| 1 | BACKLIGHT_SW_RETURN (MOSFET drain — near GND when LEDs on) |
| 2 | +12V_BACKLIGHT (always-on 12V supply to LED string tops) |

## Switches & Controls

- Toggle switches: 12 mm (standard), ~6 mm (ECM modules)

## CAN Transceiver

**Selected: SN65HVD230** — SOIC-8, 3.3V logic.

- One per MCU board; connects STM32F103 CAN controller (PA11/PA12) to the physical bus
- Runs directly from 3.3V rail — no level shifter required
- CANH/CANL route to Molex Mini-Fit Jr main bus connector
- **Bus termination:** 120Ω resistor across CANH/CANL on each end node; omit on intermediate nodes
- Compatible 3.3V clones (e.g. VP230) acceptable for JLCPCB assembly

## LED Backlighting

**Architecture: 5-LED series strings, MOSFET-switched per zone, resistor current limiting.**

Confirmed from bench testing. Estimated ~500 LEDs total across full cockpit (~100 strings of 5).

### String design

- 5 × 5050 SMD red LEDs in series per string
- Measured Vf: 1.95–2.1 V per LED → ~10 V total per string at 12 V supply → ~2 V headroom for resistor
- One current-limiting resistor per string (back face of PCB)

### Resistor values (bench-tested)

| Resistor | Measured current | Use |
|---|---|---|
| 47Ω | 34–42 mA | **Rejected** — resistor overheated |
| 100Ω | 19–23 mA | Bright panels / gauges |
| 120Ω | ~17–20 mA | **Balanced default** |
| 150Ω | 14–17 mA | Standard |
| 180Ω | ~11–13 mA | Low brightness / night-friendly |
| 200Ω | 10–12 mA | Dimmest |

**Default: 120Ω.** Choose at assembly time per zone; 100Ω for bright panels, 180Ω for dimmer zones.

**Resistor package:** 0805 minimum, 1206 preferred where board space allows. Dissipation ≈ 39 mW at 120Ω/18 mA — either package is thermally fine; 1206 is easier to hand-solder and rework.

### Zone dimming (MOSFET)

- One **IRLML2502** N-channel MOSFET per panel/lighting zone (SOT-23, 20V, 4A, Vgs(th) 0.3–0.7V) — **low-side switch**
- Gate driven directly by STM32 3.3V PWM — no gate driver required (Vgs = 3.3V, well within ±12V Vgs(max))
- Drain → BACKLIGHT_SW_RETURN (Mini-Fit Jr pin 1); source → GND
- LED strings: +12V_BACKLIGHT (Mini-Fit Jr pin 2) → resistor → 5× LEDs → BACKLIGHT_SW_RETURN
- Resistors set per-string current at full on; PWM duty cycle controls average brightness across the zone

### Other

- Placement: LEDs on PCB front face; resistors, MOSFETs, and all other components on back face
- PCB trace width: 0.3 mm minimum for LED string feeds
- 5-LED strings preferred over 3-LED: fewer parallel paths, lower wiring current (1.8A total at 120Ω vs ~3A for 3-LED strings across same ~500 LEDs)

## Gauges

- LOX gauge: 2-5/8″ (~67 mm total)
- Radar Altimeter gauge: 3-1/8″ (~100 mm with bezel)
- Cabin Pressure gauge: driven by X27.589 Switec stepper (shaft-through-PCB mount)

---

## Component Pin Assignment Rules

Hard design rules that apply at schematic capture time. These are not layout preferences —
violating them causes functional failures that cannot be fixed after fabrication.

| Chip | Affected pins | Rule | Reason |
|---|---|---|---|
| MCP23017 | GPA7, GPB7 | **Output only — never configure as inputs** | Silicon bug confirmed by Microchip (datasheet Rev D, June 2022): SDA signal is corrupted if pin voltage changes during an I2C bit transmission, causing bus malfunction. The IODIR register physically allows input mode but it is unsafe. |

**MCP23017 input capacity: 14 pins per chip** (GPA0–GPA6 + GPB0–GPB6).  
GPA7 and GPB7 may drive LEDs, enables, or any other output load without restriction.  
Source: Microchip support article *"GPA7 & GPB7 Cannot Be Used as Inputs In MCP23017"*

---

## Standard Circuit Blocks (Design Library)

These subcircuits repeat on every MCU board and are candidates for KiCad hierarchical sheet templates (`PCB/Libraries/sheets/`).

### LED Zone Switch (per lighting zone)

```
+12V_BACKLIGHT ── R (120 Ω) ──┬── LED1 A
                               └── LED2 A  (one resistor per string, each string 5 LEDs in series)
                                   ...
                               LED_n K → BACKLIGHT_SW_RETURN
                                              │
                                        IRLML2502 drain
                                        IRLML2502 source → GND
                                        IRLML2502 gate ← STM32 PWM (3.3V direct)
```

- Default off (gate at 0V, Vgs = 0V → MOSFET off)
- STM32 HIGH (3.3V) → Vgs = 3.3V → N-ch fully enhanced → BACKLIGHT_SW_RETURN pulled to GND → LED strings conduct
- STM32 LOW → Vgs = 0V → MOSFET off → no current path → LEDs off
- No gate pull-up or driver required: IRLML2502 Vgs(th) = 0.3–0.7V; 3.3V is well within Vgs(max) = ±12V
- LED power arrives on a **separate 2-pin Mini-Fit Jr connector** — not the signal harness

### MCP23017 Instance

> **Silicon bug — GPA7 and GPB7 must not be used as inputs.**  
> Confirmed by Microchip in datasheet Revision D (June 2022) and support article
> "GPA7 & GPB7 Cannot Be Used as Inputs In MCP23017": the SDA signal can be corrupted
> if the pin voltage changes during an I2C bit transmission, potentially causing bus host
> malfunction. The register bits allow direction to be changed to input, but doing so is
> unsafe. **Route switches and other inputs to GPA0–GPA6 and GPB0–GPB6 only (14 input
> pins per chip). GPA7 and GPB7 may be used as outputs (LEDs, enables, etc.).**

Per chip on each MCU board:

| Component | Value | Notes |
|---|---|---|
| C_decoupling | 100 nF, 0805 | VDD → GND, placed adjacent to chip |
| C_bulk | 10 µF, 0805 | VDD → GND, one per board |
| R_RESET | 10 kΩ, 1206 | RESET → +3V3 pull-up |
| R_A1 (if addr bit = HIGH) | 10 kΩ, 1206 | Address pin → +3V3 |
| R_SDA, R_SCL | 33 Ω, 0805 | Series on SDA/SCL between harness and chip — damps ringing on off-board wiring |
| R_INTA, R_INTB | 100 Ω, 0805 | Series on interrupt outputs before harness connector — ESD/transient protection |

I2C pull-ups (4.7 kΩ) placed **on MCU board only** — one set per bus, not on breakout boards.

Internal MCP23017 pull-ups (GPPU register, ~100 kΩ) are sufficient for switch inputs — no external pull-ups required on breakout boards unless wiring is very long or noisy.

### ADC Input Filter (per analog input)

Placed on MCU board, close to STM32 ADC pin:

```
pot wiper (via harness) → 1 kΩ → STM32 ADC pin → 100 nF → GND
```

- 1 kΩ: protects ADC pin during faults/transients, forms RC with 100 nF
- 100 nF: low-pass filter, reduces noise/jitter on ADC reading
- RC corner frequency: ~1.6 kHz — well above polling rate, no signal distortion
