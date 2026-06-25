# Hardware Standards

## Component Package Rules

**Constraint: all packages must be visually inspectable after reflow.** T962 reflow oven is available; the limitation is inspection, not soldering.

| Acceptable | Not acceptable |
|------------|---------------|
| SOIC, SSOP, TSSOP, HTSSOP | DFN, WSON (true no-lead) |
| LQFP | BGA, LGA |
| SOT-23, SOT-223 | Any *fully* bottom-terminated package |
| Through-hole, QFN *(case-by-case — see below)* | |

HTSSOP (exposed thermal pad on underside) is acceptable — side leads are the critical joints; the GND pad is verified by continuity check.

**QFN — case-by-case, not a blanket ban.** A QFN whose perimeter leads are side-accessible (a solder fillet climbs the castellated edge) with only a GND center pad — e.g. the **RP2040 QFN-56** — is judged like HTSSOP: the side fillets are the inspectable critical joints; the center pad is GND (continuity-checked). It reflows on the T962 with a stencil, a reduced center-pad aperture (~50–70 %), and a proper profile. Only *fully* bottom-terminated packages — BGA, LGA, true no-lead DFN/WSON — are out, because there is no side joint to inspect.

## MCU

**Selected: STM32F103C8T6** — LQFP48, 64 KB flash, 20 KB RAM. Default for every board (PanelGroup and PanelBridge alike; PanelBridge's DCS-BIOS input map still compiles to ~26 KB). The **STM32F103CBT6** (128 KB flash) is a drop-in fallback on the identical LQFP48 footprint for any future build that exceeds 64 KB — no board currently needs it.

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

**Selected: ADS1115IDGSR (TI)** — 16-bit ADC, VSSOP-10. **LCSC: C37593** _(note: LCSC description erroneously says QFN-10; IDGSR suffix = VSSOP-10 with gull-wing leads — inspectable)_

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
| 5 V rail | also feeds | — | DRV8833 VM (stepper driver motor supply) |
| 12 V → 5 V | AP63205WU | SOT-23-6 | **Only on high-5V-current boards** (future actuator/high-power boards). Not used on standard MCU or breakout boards. Typical BOM: C_in 10 µF, C_bypass 100 nF, L 4.7 µH, C_out 2×22 µF |

Local decoupling required on every board: 100 nF + 10 µF per rail, placed close to each IC. Never use a linear regulator for 12 V → 5 V conversion.

## Stepper Driver

**Selected: DRV8833PWPR (TI)** — dual H-bridge, TSSOP-16-EP. **LCSC: C50506**

DRV8835 was considered but is only available in WSON-12 (fully bottom-terminated, not inspectable after reflow). DRV8833PW is the same electrical capability in an inspectable HTSSOP-16 package.

- Drives one X27.589 Switec stepper (bipolar, **~945 partial steps/315°** = 1/3°/step, 1080 steps/full-rev; 180–300 Ω coils, ~15–30 mA at 5 V)
- VM supply: 2–10.8 V (use 5 V); VINT: 3.3 V output (bypass with 100 nF to GND — do not drive with external supply)
- VCP (charge pump): 100 nF cap to VM — required for internal charge pump
- **~SLEEP pin (active LOW) — sim-gated, not held HIGH.** The driver **sleeps** (~SLEEP LOW) until the sim connects; on connect the firmware drives ~SLEEP HIGH, runs the homing sequence, then **syncs the gauge to the current DCS value**; on disconnect it returns ~SLEEP LOW. This de-energises the coils when idle (no holding current / heat — important for the heavier servos) and gives free power-up anti-twitch (coils off until there is something to display). ~SLEEP must be a **controlled GPIO** — STM32 GPIO on host boards, an **MCP23017 output on sub-panels** — with a **10 kΩ pull-down to GND** so it defaults LOW (asleep) before the MCU/MCP configures it. **Never tie ~SLEEP to +3V3.**
- AISEN / BISEN: current sense inputs — tie to GND (no current regulation needed; X27.589 coil resistance limits current naturally at 5 V)
- ~FAULT: open-drain fault output — leave NC in this revision
- No current-regulation passives needed; X27.589 coil resistance limits current naturally at 5 V
- Firmware: **`StepperMotor`** (SwitecX25 accel ported in-tree; STALL & SENSOR homing) — see *Stepper drive* below
- KiCad symbol: `Driver_Motor:DRV8833PW` (built-in)

### Stepper drive — resolution, homing, and I²C-expander limits

**Resolution (X27.589 / VID-29 / BKA-30 air-core family).** `StepperMotor` drives the datasheet **partial step = 1/3°/step** (1080 steps/full-rev) via the 6-state air-core sequence. Full step = 1°; micro step = 1/12° (PWM current control) is **not used** — the DRV8833 has no indexer and the firmware drives plain digital coil states. 1/3° (~960 positions over a 320° gauge) is ample; microstepping is a deferred option (see #122).

**Homing (STALL).** `StepperConfig.rangeSteps` is the **mechanical stop-to-stop travel** (~945 for an X27.589 315°, ~960 for a 320° BKA-30) — distinct from `stepsPerRev` (the full 360° revolution, used for degree↔step and `wrap`). STALL home drives `rangeSteps` (+ a 1/8 margin), **not** a full revolution: over-driving a stop-limited gauge by the rev−range difference grinds the seated rotor and desyncs it, so the zero wanders between homes. The seek runs at `homeStepUs` (default 2000 µs ≈ 500 steps/s), which **must stay under the air-core start-stop speed** (~774 steps/s ≈ 1290 µs/step) — the former 800 µs (1250 steps/s) sat at that edge and caused the "home left, then home right" band-swap.

**Coil drive path & speed envelope** (bench-measured 2026-06-21, BKA-30 on DRV8833 @ 5 V):

| Path | Step rate | ≈ °/s | Notes |
|---|---|---|---|
| Native STM32 GPIO | ~1667 steps/s | ~600 | motor-limited (datasheet max with accel) |
| MCP23017 @ 400 kHz | ~490 steps/s | ~163 | batched port write; I²C-overhead-limited |
| MCP23017 @ 100 kHz | ~260 steps/s | ~87 | for long / loaded buses |

- **STM32F103 I²C ceiling = 400 kHz** (no fast-mode-plus on the F1 peripheral).
- **Batched port write:** `StepperMotor` writes all four coils in **one** `writePort()` per step (`PanelGroup::flushExpanderWrites()`), not four per-pin read-modify-writes — fewer I²C transactions, lower bus occupancy, and an **atomic coil transition** (no mixed-coil intermediate state). **Caveat: the batched write rewrites the whole 8-bit port from cache, so stepper coils must OWN their MCP23017 port** — do not place unrelated I/O on the other pins of that port.
- **Rule of thumb:** a gauge needing fast slews → coils on **native GPIO**; a slow / remote gauge → MCP expander (≈163 °/s, fine for gradual gauges like APN-153 DRIFT). The expander is a *reach* tool, not a speed tool.

**12-inch remote I²C bus.** At ~12" the bus capacitance (cable + device pins, ~150–250 pF) limits speed:

- **400 kHz** needs SCL rise < 300 ns → use **~1.5 kΩ pull-ups** (sink ~2.2 mA, within MCP23017 / STM32 limits). 4.7 kΩ at that capacitance rises in ~1 µs and **fails** 400 kHz.
- **100 kHz** tolerates **4.7 kΩ** at 12" (1 µs rise is within spec) — reliable but slower.
- Or an **active I²C buffer** (P82B715 / PCA9600 / LTC4311) to hold 400 kHz over 12–18".
- Verify on a scope (SCL rise time) with the **fully populated** bus (MCP + OLED + ADS) for realistic capacitance.

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

**Wire gauge:** 16–18 AWG silicone on the **main bus** (power + CAN daisy-chain); 24 AWG on
signal/logic harnesses (I²C, interrupts, switches). Match the Mini-Fit Jr crimp terminal to the
bus wire: 18 AWG → standard 5556 (18–24 AWG); 16 AWG → HCS / 16 AWG-rated terminal (5556 won't crimp 16 AWG).

### Molex Mini-Fit Jr (main bus + LED power)

- **PCB footprint:** Through-hole, single-row vertical for LED connectors (1×2); dual-row vertical for main bus
- Main bus: CAN bus signals and power distribution between controller groups
- LED power: 2-pin (1×2) per lighting zone on every MCU and breakout board — carries `+12V_BACKLIGHT` and `BACKLIGHT_SW_RETURN`
- Polarized housing — one insertion orientation only
- Rated up to **9 A/pin** (gauge/terminal-dependent; derate when all pins energized). Power split across pins (2×+12V, 3×GND) gives large margin over the ~2.3 A system load — current is never the limit, pour continuity at the pads is

**Main bus connector: 2×4 (8-pin), `Connector_Molex:Molex_Mini-Fit_Jr_5566-08A2_2x04_P4.20mm_Vertical`** (5566 = vertical PCB header; 5557 is the mating wire-side receptacle housing)

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

### Rotary switches (multi-position selectors)

- Three standard variants, distinguished by **detent angle**: **12-position (30°)**, **8-position (45°)**,
  **6-position (60°)** — positions in 360° = 360° ÷ angle. Pick the variant whose detent angle matches
  the real selector.
- Use **fewer positions** than the variant's max where needed, and limit travel with a **CAD turn-limiter
  in the bezel** (preferred — repeatable across prints) rather than the switch's adjustable stop washer.
  Example: a 5-position selector at 30° detent = the **12-position variant, CAD-limited to 5** (≈120° throw).
- Wire as `SwitchMultiPos`: **one GPIO per used position**, common → **GND** (MCP23017 or STM32 breakout).
  Prefer this over a resistor-ladder (`AnalogMultiPos`) when GPIO is plentiful — no ladder tolerance or
  ADC-threshold calibration; reserve the ladder for when GPIO/ADC-routing is tight.
- High-count or freely-spinning knobs (>12 positions, continuous) use a **`RotaryEncoder`** instead.

### Rotary encoders (EC11 / bare quadrature)

- A and B → two GPIO, each with a **10 kΩ pull-up to 3V3**. The `RotaryEncoder` class sets a plain
  `INPUT` (no internal pull-up), so an open contact floats without the external resistor. Common → GND.
- The integral push-switch on a 5-pin EC11 is a separate SPST — wire it as its own `Switch2Pos`, or
  leave the two switch pins unconnected if out of scope.
- Reverse the sensed direction by swapping the A/B pins — no firmware change.

### Potentiometers (analog)

- A pot is already a divider: wiper → an **ADC pin**, the two ends → `+3V3` and `GND`. No added
  resistors.
- **High end → 3V3, never 5 V.** The STM32F103 ADC pins (PA0–PA7, PB0, PB1) are **not 5 V-tolerant**;
  5 V damages them. Reverse the travel by swapping the end terminals.

### Freeing the JTAG pins for breakout GPIO

`PA15` (JTDI), `PB3` (JTDO), `PB4` (NJTRST) power up as the JTAG-DP. On the **PanelGroup base** they
are exposed as remap-gated breakout pins; to use them as ordinary GPIO, remap SWJ to SWD-only early
in `setup()`. This keeps SWD (PA13/PA14) live so ST-Link still flashes, and releases only the three
JTAG pins:

```cpp
__HAL_RCC_AFIO_CLK_ENABLE();
__HAL_AFIO_REMAP_SWJ_NOJTAG();   // JTAG off, SWD on → PA15/PB3/PB4 usable as GPIO
```

The Gateway/Bridge base leaves these NC — it has enough clean breakout pins not to bother (see
[base boards](base-boards.md)).

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

**Firmware consequence — GPINTEN must exclude bit 7 on both ports.**
`chip.interrupt(port, CHANGE)` (blemasle library) enables GPINTEN for all 8 bits. Do not use it directly. Instead, read IODIR after `configure()` and write it masked (`& 0x7F`) to GPINTEN:
```cpp
uint8_t gpintenA = chip.readRegister(MCP23017Register::IODIR_A) & 0x7Fu;
uint8_t gpintenB = chip.readRegister(MCP23017Register::IODIR_B) & 0x7Fu;
chip.writeRegister(MCP23017Register::GPINTEN_A, gpintenA);
chip.writeRegister(MCP23017Register::GPINTEN_B, gpintenB);
```
This also ensures interrupts are never enabled on output pins — redundant per datasheet (output pins cannot fire GPINTEN), but explicit.

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

**Selected: MCP23017-E/SS (Microchip)** — 16-bit I/O expander, SSOP-28. **LCSC: C506653**

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

Switch inputs are active-low by project convention: the net is held HIGH and the switch
closes to GND. Prefer a 10 kΩ external pull-up to +3V3 on the board that owns the switch
net. MCP23017 GPPU pull-ups are weak (~100 kΩ) and available only in the pull-up direction;
do not use them as the standard bias for OpenSkyhawk switch nets unless a specific schematic
intentionally documents that choice.

### ADC Input Filter (per analog input)

Placed on MCU board, close to STM32 ADC pin:

```
pot wiper (via harness) → 1 kΩ → STM32 ADC pin → 100 nF → GND
```

- 1 kΩ: protects ADC pin during faults/transients, forms RC with 100 nF
- 100 nF: low-pass filter, reduces noise/jitter on ADC reading
- RC corner frequency: ~1.6 kHz — well above polling rate, no signal distortion
