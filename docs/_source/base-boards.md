# Base Boards — Gateway/Bridge & PanelGroup

Canonical electrical reference for the two **reusable base boards** that anchor every
OpenSkyhawk controller. Both are designed breakout-style: they carry only the shared base
circuitry, expose the standard signals on connectors, and break out spare MCU pins as headers
so specialized panel variants build on top of an identical foundation.

- **Board 1 — Gateway/Bridge:** SimGateway (RP2040-Zero) + PanelBridge (STM32, CAN master,
  `NODE_ID=0`) on one custom PCB. The cockpit's single USB-HID + DCS-BIOS head node.
- **Board 2 — PanelGroup base:** generic STM32 CAN sub-node (`NODE_ID=1–63`). The board most
  panels start from.

Sources of truth: [hardware-standards.md](hardware-standards.md),
[architecture.md](architecture.md),
`Firmware/ScratchPad/FirmwarePlan/08-hardware-firmware-contracts.md`. Prototypes referenced:
`PCB/Center_Console/Center_Armament/Armament_MCU` and `…/Misc_Switch_Panel`.

> **Milestone 1 scope:** research, pin assignment, component list, connector pinouts, and a
> manual schematic diagram per board. No KiCad capture, no new footprints/symbols. The
> RP2040-Zero module symbol is user-supplied and is the only part not already in the library.

Manual schematic diagrams: [Gateway/Bridge](base-boards-gateway-bridge.svg) ·
[PanelGroup base](base-boards-panelgroup.svg).

---

## Shared conventions

| Item | Standard |
|---|---|
| Main bus connector | Molex Mini-Fit Jr 2×4 (8-pin), **two per board** (`J_BUS_IN` / `J_BUS_OUT`, nets pass through). Footprint `Connector_Molex:Molex_Mini-Fit_Jr_5566-08A2_2x04_P4.20mm_Vertical` |
| Bus-entry bulk + PWR_FLAG | **10 µF on `+5V` and 10 µF on `+12V` at `J_BUS_IN`**, each carrying that net's **PWR_FLAG** (bus pins are passive → ERC needs a driver marker; GND flagged too). Kept on **every** base board for a consistent entry block, even where a board has no local load on a rail (e.g. Gateway/Bridge draws no +12V — pass-through only). |
| Local 3.3 V | bus `+5V` → **AMS1117-3.3** (SOT-223) → `+3V3`. Bulk: **10 µF in / 22 µF out aluminum electrolytic** (≥ 22 µF out is required for AMS1117 loop stability — the regulator needs ESR a ceramic-only output can't guarantee; alu electrolytic is cheap, fails-open, fine ESR at room temp) + **100 nF ceramic** each rail. Crystal load caps **C0G/NP0** |
| CAN PHY | **SN65HVD230** (SOIC-8) on PA11/PA12; CANH/CANL to both bus connectors |
| CAN termination | **120 Ω across CANH/CANL gated by a 2-pin jumper header.** Always populated; the jumper switches it in. Fit at chain ends, pull mid-chain → every board can self-terminate, no terminator boards/harnesses needed |
| Crystal | 8 MHz on PD0/PD1 + 2×22 pF (mandatory for CAN bit-rate lock) |
| Status / power LEDs | **0805** discrete, **dim** (series R ≈ 2.2 k–3.3 k). Red = PB14, Green = PB15 (firmware-driven) + always-on power LED off `+3V3`. **All board-mounted, grouped in one front-face cluster, each silk-labeled** (`PWR`, `BRIDGE` R/G, `SIMGW` R/G, …); no module-onboard LEDs. **Low-Vf colors only on the 3.3 V rail — red (~1.9 V) + yellow-green (~2.1 V); avoid blue / white / pure-green (Vf ≈ 3 V, no headroom).** Power LED = yellow-green. All GPIO-source / rail-tie — no 5 V, no transistor. |
| SWD programming header | **1×5**: `+3V3 / SWDIO(PA13) / SWCLK(PA14) / NRST / GND`. NRST included for **connect-under-reset** (reliable attach on a busy MCU + clone chips; ST-Link supports it). Optional 33 Ω series on SWDIO. |
| Diag (`DiagSerial`) header | 1×3 JST-XH: `DIAG_TX(PA9) / DIAG_RX(PA10) / GND` @ 115200, USART1 |
| Boot | BOOT0 (pin 44) 10 k pull-down + jumper. PB2/BOOT1 → **NC** (don't-care when BOOT0=0; boards flash via SWD). 10 k pull-down only if reliable DFU via a BOOT0 jumper is wanted. |
| Unusable pins | PC13 / PC14 / PC15 → NC |
| MCU footprint | LQFP48 — identical for STM32F103C8 and CB; variant is a BOM swap only |
| Mounting holes | **4× M2.5 NPTH** (`MountingHole:MountingHole_2.5mm`), one per corner, **locked**, ⌀5 mm keepout. Per-board (outline-relative) — **not** part of the copy-paste base block; add after the board outline. Standard screw across the whole cockpit = M2.5. |

### Main bus pinout (`J_BUS_IN` / `J_BUS_OUT`)

| Pin | Net | Pin | Net |
|---|---|---|---|
| 1 | +12V | 5 | CANH |
| 2 | +12V | 6 | CANL |
| 3 | +5V | 7 | GND |
| 4 | GND | 8 | GND |

Pins 1/2 parallel +12V (LED backlight current); 4/7/8 all GND; CANH/CANL on pins 5/6 (same
row) for clean differential routing. **No PSU-input connector on either board** — 12V/5V/GND
are injected elsewhere on the bus and pass straight through.

### Power distribution & bus-connector layout

- **Topology:** power daisy-chains through both bus connectors (same path as CAN). `+12V` and
  `GND` are carried as **copper pours/zones**, not routed traces — pour area ≫ any 1 mm trace,
  so cumulative chain current is a non-issue on-board. `+5V` is low-current (logic + DRV8833
  stepper VM; backlight is on 12V) — route wide (~0.5 mm+) or pour it too.
- **Harness:** 18 AWG silicone on the bus (16 AWG also fine — match the Mini-Fit crimp terminal
  to the gauge). Mini-Fit Jr **8-circuit, all-loaded, 18 AWG = ~8 A/pin** (the 9 A figure is for
  ≤3 circuits) → per-pin actual ~2–2.5 A (12V 2 pins / 5V 1 pin / GND 3 pins) = **~3.5–4× margin.**
  **No per-*board* fusing, but fuse source-side** (see *Source fusing* below — the PSU's OCP
  protects the PSU, not the harness). GND stays common + continuous (CAN reference) — **never
  fused.** The THT bus pins double as **large plated-through vias** stitching top↔bottom GND at the
  connector.
- **Connector copper (Milestone-2 layout rules):**
  - **Place J_BUS_IN / J_BUS_OUT adjacent** — power pass-through becomes a short pin-to-pin
    bridge (minimal R / neck risk). Reserve **wire-exit room for both fat 18 AWG silicone
    bundles side-by-side** + housing/edge clearance — that's the real spacing constraint, not copper.
  - **Solid or wide-spoke (≥ 0.5 mm) pour-to-pad connection** on every 12V/5V/GND pin — default
    thermal relief necks the pour and defeats it. T962 reflow handles solid pads fine.
  - Flood the pour up to the pads (**zone clearance ~0.25 mm**, not 0.5).
  - Keep a **clear pour channel between J_BUS_IN ↔ J_BUS_OUT** — don't island it with crossing tracks/vias.
  - **Stitch top↔bottom GND with ≥ 4–6 power vias** (0.4 mm) at layer transitions (~1 A/via).
  - Leave **mechanical wire-exit clearance** for the fat 16–18 AWG silicone + crimp at each connector.
- **Net classes:** a `Power` class (1.0 mm track, via 0.8/0.4) is assigned to `+12V`/`+5V`, so
  any *routed* power segment defaults wide — but the **pours carry the bulk** 12V/GND current.
  Added to the project-template and the existing KiCad projects (Armament_MCU, Misc_Switch_Panel,
  AWRS_Panel). Template `zones.min_clearance` = 0.25 so pours flood to the pads.
- **Pour sizing:** current never sets the pour width — 1 mm of 1 oz copper already carries the
  ~2.3 A bus. The constraint is the **narrowest neck**: keep ≥ ~1 mm (aim ≥ 2 mm) past any via,
  hole, cutout, or other-net island; don't let a same-layer signal split the channel; stitch
  layer changes with ≥ 4–6 power vias. Sketches: [power-pour layout](base-boards-power-pour.svg).

### Source fusing & per-console injection

**The PSU's OCP protects the PSU, not the harness.** A single-rail ATX +12V trips at ~40 A — it
would dump tens of amps into a fault long before that, cooking a ~8 A connector pin / 1 mm trace.
So protect **at the supply, sized to the harness:**

- **Fuse each rail at every per-console injection node**, sized to *that console's* draw — the fuse
  **protects all boards downstream of it** on that rail (a console fault opens only that console's
  fuse, sparing the rest of the bus). At an injection node the 8-pin `J_BUS_IN` is **split-sourced**
  from two harnesses: **+12V/+5V + GND come from the PSU** (through this console's fuse), while
  **CANH/CANL + GND come from the previous node's `J_BUS_OUT`** — that node's power pins dead-end
  there, so **power never crosses the console boundary** while **CAN stays one continuous bus**. GND
  is the single common net tying both (reference + return). The board keeps one 8-pin `J_BUS`
  footprint — the split is in the injection harness, not the connector. Rule: **segment peak × margin
  ≤ fuse ≤ weakest downstream copper** (the ~2.5 A 1 mm 1 oz trace is the floor). Consoles draw ~1 A @12V, so **low-single-amp
  slow-blow** fuses fit under that trace (slow-blow — LED strings / buck inputs inrush at power-on).
  The blanket **+12V ~5 A · +5V ~3–4 A** is only the whole-bus upper bound; per console it comes down.
  **GND is never fused** (CAN reference + return).
- **The fuse sets the copper, not the connector** — size traces/pours for the *fuse* rating, not the
  connector's theoretical max. A 5 A fuse means the copper only ever sees 5 A, so 1 oz pours / wide
  bridges are ample (1 mm 1 oz ≈ 2.5 A).
- **Load reality:** whole cockpit ≈ 3 A @12V (backlight) + ~2 A @5V ≈ ~50 W → a **350–450 W
  single-rail ATX is ample** (500 W is overkill). Only **+12V** can be multi-rail; **+5V/+3.3V are
  always single-rail.** Use +12V (backlight) + +5V (logic + DRV8833 VM) + GND; boards regulate their
  own 3V3 (AMS1117), so +3.3V/+5VSB/−12V go unused. Parallel the ATX 24-pin's duplicate rail pins.

**Per-console injection (scaling lever — see #202).** Default = one daisy-chain (power passes through
every board; the first board's pass-through bridge carries the **cumulative** downstream current). To
scale or isolate:

- **Inject +12V/+5V per console** and **break that rail's pass-through** (leave its `J_BUS_OUT` pin
  NC) → each segment carries only *its* downstream load. Per group: **<1 A @12V** (a 1 mm trace
  suffices), modest @5V. The **pour is only needed on the pass-through bridge** that carries the
  cumulative — the board's own tap to local circuits is a light trace.
- **GND + CAN stay continuous** across all segments — never segment them.
- Make the 12V/5V pass-through **optional** (jumper / DNP) so a zone can be re-injected later with no
  redesign.
- **+12V gets the full pass-through treatment even on pass-through-only boards** (e.g. Gateway/Bridge
  draws no local 12V but still carries the cumulative downstream).
- Contingency: a **power-distribution + monitoring CAN node** (#202) — STM32 + per-zone INA226 +
  eFuse breakers, telemetry → PanelBridge — measures the (unknown) consumption *and* provides the
  injection points. Its eFuse breaking sits **on top of** the baseline per-console fuse (telemetry +
  active trip), not instead of it.

### PSU / source supply

**Source = a fully-modular ATX PSU** (DC-DC topology — no minimum-load resistor needed; safe at the
cockpit's ~50 W). Selected: **GOLDEN FIELD NX650** (650 W, 80+ Gold, fully modular, ~$60) —
over-provisioned ~13×, bought for clean modular cabling + expansion headroom (a tier-1 450 W is a
fine alternative).

- **Turn-on:** no motherboard → **jumper PS_ON# (green, 24-pin pin 16) → GND** (add a switch).
  Optional later: a small always-on MCU on **+5VSB** (always live, 2.5 A) toggles PS_ON# = soft power
  button + AC-present sense. Don't run panel logic off +5VSB — use the main +5V.
- **Single +12V rail (54 A) → source-fusing is MANDATORY.** Consumer ATX has **no per-pin /
  per-connector protection** — only rail-level OCP (~60 A) + dead-short SCP. A fault pulling 15 A
  through a 5 A connector burns it long before OCP notices. **The per-lead fuse *is* the
  per-connector protection** (12V ~5 A / 5V ~3 A, GND unfused).
- **+5V is only on the IDE/SATA cables** — PCIe / CPU / 12V-2x6 are **+12V-only**. The NX650 ships
  **2× IDE cables (3×SATA + 1×Molex each)** → only **2 independent feeds that carry 5 V**. For a 3rd
  console build a 3rd IDE/SATA cable or a custom **6-pin → Mini-Fit Jr** lead. Use the **Molex ends
  (~11 A)** for heavier feeds, SATA (~few A) for light; 12V-only zones (backlight) can use the PCIe
  cable.
- **All ports share the same internal rails** (single-rail DC-DC) — the limit is the
  **cable/connector**, not a separate rail. Sum of all leads ≪ 54 A / 18 A.
- **⚠️ Modular cables are brand-specific** — ATX 3.1 standardizes only the *device-side* connectors
  (SATA/Molex/PCIe/24-pin), **not** the PSU-side modular sockets. Never reuse another PSU's modular
  cables (wrong PSU-side pinout = +12V onto GND). Tap the standard device-side, or meter the PSU-side
  before building custom leads.
- Mechanical: needs an **ATX PSU mount** + **IEC C13 cord**; ECO fan likely zero-RPM at this load —
  ensure enclosure airflow.

---

## Board 1 — Gateway/Bridge

**Path:** `PCB/Base/Gateway_Bridge/` · **STM32:** STM32F103**C8**T6 (64 KB; compiled PanelBridge ≈ 26 KB / 40% — CB not needed) · **NODE_ID = 0**

**RP2040-Zero — module, not bare die (decision).** For prototyping we use the off-the-shelf
module. Bare-RP2040 parts cost is roughly a wash, but the module wins on *total* cost at low
volume: no stencil, factory-tested, far less bring-up/yield risk (it already integrates USB-C,
QSPI flash, crystal, LDO, BOOT/RESET, WS2812). QFN-56 is **not** the blocker — it reflows +
inspects fine on the T962 (stencil + reduced center-pad aperture + a proper profile), same logic
as HTSSOP. Go bare only at volume or for tight mechanical integration.

### Two power/signal domains

| Domain | Source | Generates 3.3 V | Notes |
|---|---|---|---|
| SimGateway (RP2040-Zero) | USB-C from PC (VBUS) | module's own onboard LDO | HID device; VID `0x2E8A` / PID `0x4134` |
| PanelBridge (STM32) | bus `+5V` → AMS1117 | AMS1117-3.3 | CAN master + DCS-BIOS |

**The two 3.3 V rails are NOT tied together.** The RP2040-Zero's onboard LDO is hardwired to
USB VBUS and cannot be disabled on the module; tying its output to the AMS1117 output puts two
regulators on one rail (neither can sink current → the lower one is back-driven, out of spec,
with a back-feed path toward USB VBUS). The two boards **share GND only** — that common
reference is all the UART link needs.

### RP2040-Zero ↔ STM32 UART link

This is **the UART / serial link between the two MCUs** — the only data path between SimGateway
(RP2040) and PanelBridge (STM32). DCS-BIOS + HID frames flow over it. **Net labels** are
direction-named to kill the classic TX–TX miswire (a net called `UART_TX` is ambiguous; these
state the data flow). Naming isn't self-evident at a glance, but it's unambiguous about
direction, which is what matters for the cross-over.

| Net label | source | → | dest | carries |
|---|---|---|---|---|
| `SIMGW_TO_BRIDGE` | RP2040 GP0 (UART0 TX) | → | STM32 PA3 (USART2 RX) | SimGateway → PanelBridge |
| `BRIDGE_TO_SIMGW` | STM32 PA2 (USART2 TX) | → | RP2040 GP1 (UART0 RX) | PanelBridge → SimGateway |
| `GND` | RP2040 GND | — | STM32 VSS | common reference (required) |

3.3 V logic both sides, no level shifter. Optional 33 Ω series on each line (label the
resistor-to-resistor trunk with the net name). 250000 baud. Cross-over: each MCU's TX drives the
*other's* RX — `SIMGW_TO_BRIDGE` is the SimGateway transmit net, `BRIDGE_TO_SIMGW` the Bridge transmit net.

### STM32 pin assignment (LQFP48)

| Pin | Port | Function | Pin | Port | Function |
|---|---|---|---|---|---|
| 1 | VBAT | +3V3 | 25 | PB12 | breakout → J3 |
| 2 | PC13 | NC | 26 | PB13 | breakout → J3 |
| 3 | PC14 | NC | 27 | PB14 | STATUS_LED_RED |
| 4 | PC15 | NC | 28 | PB15 | STATUS_LED_GRN |
| 5 | PD0 | OSC_IN (8 MHz) | 29 | PA8 | breakout → J3 |
| 6 | PD1 | OSC_OUT (8 MHz) | 30 | PA9 | USART1_TX (diag) |
| 7 | NRST | reset | 31 | PA10 | USART1_RX (diag) |
| 8 | VSSA | GND (filtered) | 32 | PA11 | CAN_RX → SN65HVD230 |
| 9 | VDDA | +3V3 (filtered) | 33 | PA12 | CAN_TX → SN65HVD230 |
| 10 | PA0 | breakout → J3 | 34 | PA13 | SWDIO |
| 11 | PA1 | breakout → J3 | 35 | VSS | GND |
| 12 | **PA2** | **USART2_TX → RP2040 GP1** | 36 | VDD | +3V3 |
| 13 | **PA3** | **USART2_RX ← RP2040 GP0** | 37 | PA14 | SWCLK |
| 14 | PA4 | breakout → J3 | 38 | PA15 | NC (JTDI — JTAG) |
| 15 | PA5 | breakout → J3 | 39 | PB3 | NC (JTDO — JTAG) |
| 16 | PA6 | breakout → J3 | 40 | PB4 | NC (NJTRST — JTAG) |
| 17 | PA7 | breakout → J3 | 41 | PB5 | breakout → J3 |
| 18 | PB0 | breakout → J3 | 42 | PB6 | I2C1_SCL |
| 19 | PB1 | breakout → J3 | 43 | PB7 | I2C1_SDA |
| 20 | PB2 | BOOT1 → NC | 44 | BOOT0 | 10 k pull-down + jumper |
| 21 | PB10 | I2C2_SCL | 45 | PB8 | breakout → J3 |
| 22 | PB11 | I2C2_SDA | 46 | PB9 | breakout → J3 |
| 23 | VSS | GND | 47 | VSS | GND |
| 24 | VDD | +3V3 | 48 | VDD | +3V3 |

**Breakout — one 1×20 header (J3, STM32), jumper-wire access.** PanelBridge is a head
node (no panel I/O), but spare STM32 pins are exposed on a single-row 0.1″ header for dev/third-party
use. At ~10×10 cm the board is too wide to plug into a breadboard, so this is a **jumper-access
header** — free placement, lean toward **female sockets** for dupont leads (not a breadboard straddle).

- **J3 — STM32 breakout (1×20):** 14 GPIO `PA0, PA1, PA4, PA5, PA6, PA7, PA8, PB0, PB1, PB5, PB8, PB9, PB12, PB13` + 2× board `+3V3` + 4× `GND`.
- **RP2040 breakout dropped (decision).** The original `J4` (RP2040 spare pins) was **removed** — the RP2040-Zero is already a breakout (its castellations are exposed on the module), so duplicating them was redundant and ate most of the routing ratsnest. RP2040 spares stay on the module for anyone who wants them.

STM32 spare-pin capability (silk-label on J3):
- **Analog (ADC) + PWM:** PA0, PA1, PA6, PA7, PB0, PB1 — *ADC pins PA0–PA7/PB0/PB1 are **not 5 V-tolerant**, 3.3 V only.*
- **Analog (ADC), no PWM:** PA4, PA5.
- **PWM (timer), digital:** PA8, PB5, PB8, PB9, PB13.
- **Digital only:** PB12.
- **EXTI:** every pin is interrupt-capable; same pin-number across ports shares one line (watch PA5↔PB5 = line 5, PA8↔PB8 = line 8 if both used as IRQs).

**NC (✕):** PA15 (JTDI), PB3 (JTDO), PB4 (NJTRST) — JTAG, not worth a remap given 14 clean pins; + PC13/14/15. **PB2 → NC** (BOOT1 don't-care). Keep the board **≤ 100×100 mm** for JLC's cheap tier.

### I²C buses (both exposed — display-oriented, no INT)

Both STM32 I²C buses are brought out for local I²C on the head board — typically a small status
display (I²C OLED/LCD) or a sensor. Unlike the PanelGroup base, **no interrupt lines** are
carried (no MCP23017 sub-panels on the bridge), so the headers are compact **4-pin**.

| Bus | SCL | SDA |
|---|---|---|
| I2C1 (`Wire`) | PB6 | PB7 |
| I2C2 (`Wire1`) | PB10 | PB11 |

On-board passives: **4.7 kΩ pull-ups per bus** (SCL+SDA) + **33 Ω series on SDA/SCL**. Exposed
via two **JST-XH 4-pin** headers (`J_I2C1`, `J_I2C2`): 1 SDA · 2 SCL · 3 +3V3 · 4 GND.
(PanelBridge firmware does not drive I²C itself; provided for local/optional use.) The
INT-capable GPIO that the PanelGroup base spends on MCP23017 interrupts (PB12/PB13, PB8/PB9)
stay free here and return to breakout header **J3**.

### Connectors & indicators

| Ref | Type | Purpose |
|---|---|---|
| J_BUS_IN, J_BUS_OUT | Molex Mini-Fit Jr 2×4 | CAN + power, pass-through |
| J_I2C1, J_I2C2 | JST-XH 4-pin | both I²C buses — display/sensor (pinout above) |
| (USB-C) | on RP2040-Zero module | PC HID link — no separate PCB USB |
| J_SWD | 1×5 header | STM32 programming/debug (incl. NRST for connect-under-reset) |
| J_DIAG | 1×3 JST-XH | STM32 `DiagSerial` console |
| J_TERM | 2-pin jumper | switches in 120 Ω CAN termination |
| status / power LEDs | **5× 0805**, board-mounted + grouped + silk-labeled, dim (high-value R) | Cluster order: **1 `PWR`** (off +3V3) · **2/3 `SIMGW`** red (GP3) / grn (GP2) · **4/5 `BRIDGE`** red (PB14) / grn (PB15). Module WS2812 **not used**. |
| J3 | 1×20 header | STM32 spare-pin breakout (jumper-access; see above) |
| H1–H4 | M2.5 mounting holes | 4× NPTH, board corners (trapezoid — parts own the top corners), locked |

### RP2040 programming

Via the module's USB bootloader (onboard BOOT + RESET buttons, drag-drop UF2). No SWD header
needed for SimGateway; the module exposes SWD pads if ever required.

### RP2040-Zero pins (SimGateway side — pass-through / breakout)

The board uses **only the 20 castellated edge pins + power** (GP0–GP15, GP26–GP29, 5V/3V3/GND).
The 10 underside pads (GP16–GP25, including the onboard WS2812 on GP16) are **not used**. All
edge GPIO are PWM-capable; GP26–GP29 are the 4× 12-bit ADC. The module's `3V3` pad is its
**own LDO output (≈300 mA)** — do **not** tie to the STM32 3V3 rail (GND-only sharing).

| RP2040 pin(s) | Use / capability |
|---|---|
| GP0 | UART TX → STM32 PA3 — **reserved (link)** |
| GP1 | UART RX ← STM32 PA2 — **reserved (link)** |
| GP2 | **SimGateway status — GREEN** (`SIMGW`, 0805, board-mounted, high-value R) — driven by the status-LED state machine (active-high) |
| GP3 | **SimGateway status — RED** (`SIMGW`, 0805) — driven by the status-LED state machine (active-high) |
| GP4–GP15 | free digital / PWM — break out on header |
| GP26–GP29 | **ADC0–3** (analog axes) + PWM/digital — break out on header |
| 5V / 3V3 / GND | power (3V3 = module LDO, separate domain; 5V NC) |

Pass-through priority: **GP26–GP29 (ADC)** for HID axes, plus a few GP4–GP15. Underside pads
(GP16–GP25) are not used — only the 20 edge pins are wired.

### SimGateway status LEDs (firmware-driven)

GP2 (green) / GP3 (red) are driven by a non-blocking state machine in the SimGateway library
(`Firmware/Libraries/SimGateway/`, ticked from `SimGateway::loop()`), active-high. States,
highest priority first:

| State | Trigger | LED |
|---|---|---|
| `FAULT` | uart0 RX hardware error (RSR overrun/framing/parity) | red fast (4 Hz) |
| `NO_HOST` | USB not enumerated, or unplugged after a mount | red solid |
| `STREAMING` | DCS-BIOS bytes from the PC within last ~500 ms | green solid |
| `USB_IDLE` | USB mounted, no recent DCS data | green slow (1 Hz) |
| `INIT` | booted, pre-USB-mount (brief) | red slow |

`FAULT` is read from the `uart0` PL011 `RSR` register (the arduino-pico `SerialUART` driver
surfaces no error flags), latched for ≥2 s, and clears only when clean UART data resumes after
the fault — a silent bus holds it. User-facing table: published
[architecture/sim-gateway.md → Status LEDs](../architecture/sim-gateway.md#status-leds).
Counterpart STM32 status LEDs
(PB14/PB15) are owned by `STM32Board` (issue #93); SimGateway shares only the animation
vocabulary, not the engine.

### BOM

| Ref | Part | Library symbol | Notes |
|---|---|---|---|
| A1 | RP2040-Zero (Waveshare) | `RP2040-ZERO:RP2040-ZERO` · fp `OpenSkyhawk:MODULE_RP2040-ZERO` | in lib (SnapEDA), 3D model linked. Footprint is **through-hole** (1.0 mm pads) — 20 edge pins + 3V3/GND/5V only (no underside GP16–24) |
| U1 | STM32F103C8T6 | `MCU_ST_STM32F1:STM32F103C8Tx` | LQFP48 (CB drop-in if a build ever exceeds 64 KB) |
| U2 | SN65HVD230 | `Interface_CAN_LIN:SN65HVD230` | SOIC-8 |
| U3 | AMS1117-3.3 | `Regulator_Linear:AMS1117-3.3_SOT223` | 5→3.3 V |
| Y1 | 8 MHz crystal | `Device:Crystal` | + 2×22 pF |
| R_pullup | 4.7 kΩ ×4 | `Device:R` | I²C pull-ups (2/bus) |
| R_i2c_ser | 33 Ω ×4 | `Device:R` | SDA/SCL series (2/bus) |
| R_term | 120 Ω | `Device:R` | + J_TERM jumper |
| R_boot | 10 kΩ | `Device:R` | BOOT0 pull-down |
| R_uart | 33 Ω ×2 (opt.) | `Device:R` | UART series |
| R_led | ≈2.2 k–10 k ×5 | `Device:R` | status/power LEDs, dim (PB R/G + PWR + SimGW R/G) |
| C_xtal | 22 pF ×2 (C0G/NP0) | `Device:C` | crystal load — stable dielectric |
| C_dec | 100 nF ×~5 | `Device:C` | per VDD/VDDA + IC decoupling |
| C_bulk | 10 µF in / 22 µF out, aluminum electrolytic | `Device:CP` | AMS1117 bulk — electrolytic for loop-stability ESR; mind polarity. +100 nF ceramic each rail |
| C_busentry | 10 µF ×2 | `Device:C` | bus-entry bulk on +5V & +12V at J_BUS_IN (C14/C15) |
| C_nrst | 100 nF | `Device:C` | NRST |
| C_vdda | 1 µF + 100 nF | `Device:C` | VDDA filter (+ ferrite/10 Ω) |
| D1–D5 | LEDs (**0805**, low-Vf) | `Device:LED` | power yellow-green (`PWR`) + SimGateway red (GP3) / yellow-grn (GP2) + PanelBridge red (PB14) / yellow-grn (PB15) — grouped + silk-labeled, dim. No blue/white/pure-green. |
| J_BUS_IN/OUT | Molex Mini-Fit Jr 2×4 | `Connector_Molex:Molex_Mini-Fit_Jr_5566-08A2_2x04_P4.20mm_Vertical` | ×2 |
| J_I2C1/2 | JST-XH 4-pin | `Connector_JST:JST_XH_B4B-XH-A_1x04_P2.50mm_Vertical` | ×2 |
| J_SWD | 1×5 header | `Connector_Generic:Conn_01x05` | +3V3/SWDIO/SWCLK/NRST/GND |
| J3 | 1×20 header | `Connector_Generic:Conn_01x20` | STM32 spare-pin breakout |
| H1–H4 | M2.5 mounting hole | `MountingHole:MountingHole_2.5mm` | 4× NPTH corners; hardware (screw/standoff) off-BOM |
| J_DIAG | 1×3 JST-XH | `Connector_JST:JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical` | |
| J_TERM | 1×2 header | `Connector_Generic:Conn_01x02` | jumper |

---

## Board 2 — PanelGroup base

**Path:** `PCB/Base/PanelGroup_Base/` · **STM32:** STM32F103**C8**T6 (64 KB default; drop-in CB for flash-heavy variants) · **NODE_ID** set per build via `platformio.ini`

### I²C buses (both pre-wired on this board)

| Bus | SCL | SDA | INT_A | INT_B |
|---|---|---|---|---|
| I2C1 (`Wire`) | PB6 | PB7 | PB12 | PB13 |
| I2C2 (`Wire1`) | PB10 | PB11 | PB8 | PB9 |

On-board passives (per hardware-standards.md): **4.7 kΩ pull-ups, one set per bus** (SCL+SDA);
**33 Ω series on SDA/SCL**; **100 Ω series on each INT line** before the connector. Addressing:
MCP23017 0x20–0x27, ADS1115 0x48–0x4B (independent per bus).

### Backlight (on the base — all panels are backlit)

Two PWM-dimmed low-side zones per the LED-zone standard: **IRLML2502** N-ch MOSFET ×2, gates
driven directly by STM32 PWM, drain → `BLn_RETURN`, source → GND. Per-string
current-limit resistors live on the LED strings (per zone), not on this board.

| Zone | PWM net | Pin | Timer | MOSFET | Return |
|---|---|---|---|---|---|
| BL1 | `PWM_BL1` | PA6 | TIM3_CH1 | Q2 | `BL1_RETURN` |
| BL2 | `PWM_BL2` | PA7 | TIM3_CH2 | Q3 | `BL2_RETURN` |

Each MOSFET: 100 Ω gate series + 100 kΩ gate pull-down (gate-side, holds off at boot). `+12V`
is the always-on bus +12V (no separate backlight net).

### STM32 pin assignment (LQFP48)

| Pin | Port | Function | Pin | Port | Function |
|---|---|---|---|---|---|
| 1 | VBAT | +3V3 | 25 | PB12 | I2C1 INT_A (100 Ω → J_I2C1.6) |
| 2 | PC13 | NC | 26 | PB13 | I2C1 INT_B (100 Ω → J_I2C1.7) |
| 3 | PC14 | NC | 27 | PB14 | STATUS_LED_RED |
| 4 | PC15 | NC | 28 | PB15 | STATUS_LED_GRN |
| 5 | PD0 | OSC_IN (8 MHz) | 29 | PA8 | breakout (GPIO) |
| 6 | PD1 | OSC_OUT (8 MHz) | 30 | PA9 | USART1_TX (diag) |
| 7 | NRST | reset | 31 | PA10 | USART1_RX (diag) |
| 8 | VSSA | GND (filtered) | 32 | PA11 | CAN_RX → SN65HVD230 |
| 9 | VDDA | +3V3 (filtered) | 33 | PA12 | CAN_TX → SN65HVD230 |
| 10 | PA0 | breakout (ADC0) | 34 | PA13 | SWDIO |
| 11 | PA1 | breakout (ADC1) | 35 | VSS | GND |
| 12 | PA2 | breakout (ADC2 / USART2) | 36 | VDD | +3V3 |
| 13 | PA3 | breakout (ADC3 / USART2) | 37 | PA14 | SWCLK |
| 14 | PA4 | breakout (ADC4) | 38 | PA15 | breakout (JTAG remap) |
| 15 | PA5 | breakout (ADC5) | 39 | PB3 | breakout (JTAG remap) |
| 16 | **PA6** | **PWM_BL1 → Q2** | 40 | PB4 | breakout (JTAG remap) |
| 17 | **PA7** | **PWM_BL2 → Q3** | 41 | PB5 | breakout (GPIO) |
| 18 | PB0 | breakout (ADC8) | 42 | PB6 | I2C1_SCL |
| 19 | PB1 | breakout (ADC9) | 43 | PB7 | I2C1_SDA |
| 20 | PB2 | BOOT1 → NC | 44 | BOOT0 | 10 k pull-down + jumper |
| 21 | PB10 | I2C2_SCL | 45 | PB8 | I2C2 INT_A (100 Ω → J_I2C2.6) |
| 22 | PB11 | I2C2_SDA | 46 | PB9 | I2C2 INT_B (100 Ω → J_I2C2.7) |
| 23 | VSS | GND | 47 | VSS | GND |
| 24 | VDD | +3V3 | 48 | VDD | +3V3 |

**Breakout summary (full breakout is a goal here).** Capability per pin — label these on the
silkscreen / breakout header:

- **Analog (ADC) + PWM:** PA0, PA1, PA2, PA3, PB0, PB1 — analog input *or* timer PWM out.
- **Analog (ADC), no PWM:** PA4, PA5 — ADC only (no timer channel).
- **PWM (timer), digital:** PA8 (TIM1) + PA15, PB3, PB4, PB5 (timer via remap).
- **Plain digital GPIO:** every breakout pin also works as digital I/O.

PA15/PB3/PB4 require JTAG remap (SWD still works on PA13/PA14). Per-channel ADC RC filter
(1 kΩ + 100 nF) is added **per variant**, not on the base.

### Connectors & indicators

| Ref | Type | Purpose |
|---|---|---|
| J_BUS_IN, J_BUS_OUT | Molex Mini-Fit Jr 2×4 | CAN + power, pass-through |
| J_I2C1, J_I2C2 | JST-XH 8-pin | I²C bus to sub-panels (pinout below) |
| J_BL1, J_BL2 | Molex Mini-Fit Jr 2×01 | switched-12V backlight feed to sub-panels (pin 1 +12V, pin 2 return) |
| J_SWD | 1×5 header | programming/debug (incl. NRST) |
| J_DIAG | 1×3 JST-XH | `DiagSerial` console |
| J_TERM | 2-pin jumper | switches in 120 Ω CAN termination |
| (breakout) | 0.1″ headers | full spare-pin field |
| status / power LEDs | 3× LED | PB14 / PB15 + power |

**I²C header (`J_I2C1` / `J_I2C2`, JST-XH 8-pin):**

| Pin | Net | Pin | Net |
|---|---|---|---|
| 1 | SDA | 5 | +3V3 |
| 2 | SCL | 6 | INT_A |
| 3 | GND | 7 | INT_B |
| 4 | GND | 8 | spare |

**Backlight power-out (`J_BL1` / `J_BL2`, Molex Mini-Fit Jr 2×01):** pin 1 `+12V` (always-on bus),
pin 2 `BLn_RETURN` (MOSFET drain). Sub-panel LED strings dim off this board's MOSFET. Keep this
pin order on **every** backlight connector for harness consistency.

### BOM

| Ref | Part | Library symbol | Notes |
|---|---|---|---|
| U1 | STM32F103C8T6 | `MCU_ST_STM32F1:STM32F103C8Tx` | LQFP48 (CB drop-in) |
| U2 | SN65HVD230 | `Interface_CAN_LIN:SN65HVD230` | SOIC-8 |
| U3 | AMS1117-3.3 | `Regulator_Linear:AMS1117-3.3_SOT223` | 5→3.3 V |
| Q1, Q2 | IRLML2502 | `OpenSkyhawk:IRLML2502` | SOT-23, backlight |
| Y1 | 8 MHz crystal | `Device:Crystal` | + 2×22 pF |
| R_pullup | 4.7 kΩ ×4 | `Device:R` | I²C pull-ups (2/bus) |
| R_i2c_ser | 33 Ω ×4 | `Device:R` | SDA/SCL series (2/bus) |
| R_int | 100 Ω ×4 | `Device:R` | INT_A/INT_B series (2/bus) |
| R_gate | 100 Ω ×2 | `Device:R` | MOSFET gate series |
| R_gpd | 100 kΩ ×2 | `Device:R` | MOSFET gate pull-down |
| R_term | 120 Ω | `Device:R` | + J_TERM jumper |
| R_boot | 10 kΩ | `Device:R` | BOOT0 pull-down |
| R_led | ≈1 k–3.3 k ×3 | `Device:R` | status/power LEDs |
| C_xtal | 22 pF ×2 (C0G/NP0) | `Device:C` | crystal load — stable dielectric |
| C_dec | 100 nF ×~5 | `Device:C` | decoupling |
| C_bulk | 10 µF in / 22 µF out, aluminum electrolytic | `Device:CP` | AMS1117 bulk — electrolytic for loop-stability ESR; mind polarity. +100 nF ceramic each rail |
| C_busentry | 10 µF ×2 | `Device:C` | bus-entry bulk on +5V & +12V at J_BUS_IN (C14/C15) |
| C_nrst | 100 nF | `Device:C` | NRST |
| C_vdda | 1 µF + 100 nF | `Device:C` | VDDA filter |
| D1/D2/D3 | LEDs | `Device:LED` | red / green / power |
| J_BUS_IN/OUT | Molex Mini-Fit Jr 2×4 | `Connector_Molex:Molex_Mini-Fit_Jr_5566-08A2_2x04_P4.20mm_Vertical` | ×2 |
| J_I2C1/2 | JST-XH 8-pin | `Connector_JST:JST_XH_B8B-XH-A_1x08_P2.50mm_Vertical` | ×2 |
| J_BL1, J_BL2 | Molex Mini-Fit Jr 2×01 | `Connector_Molex:Molex_Mini-Fit_Jr_5566-02A2_2x01_P4.20mm_Vertical` | ×2 |
| J_SWD | 1×5 header | `Connector_Generic:Conn_01x05` | +3V3/SWDIO/SWCLK/NRST/GND |
| J_DIAG | 1×3 JST-XH | `Connector_JST:JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical` | |
| J_TERM | 1×2 header | `Connector_Generic:Conn_01x02` | jumper |

---

## Open items

- Repo paths: base boards live at `PCB/Base/<Board>/` (`Gateway_Bridge`, `PanelGroup_Base`);
  panel-specific PCBs stay under `PCB/<Console>/<Group>/`. **Depth note:** `PCB/Base/<Board>/`
  is 2 levels under `PCB/`, but lib-table + shared-footprint 3D-model paths are hardcoded
  `../../../Libraries` (3-level). At scaffold, set the base-board lib tables to `../../Libraries`;
  the shared footprints' 3D-model refs stay 3-level (RP2040 3D preview won't resolve on a
  2-level board — cosmetic). Clean fix: switch lib + 3D refs to a `${OS_LIB}` KiCad path var.
- ~~RP2040-Zero module symbol/footprint~~ **done** — in lib (`RP2040-ZERO:RP2040-ZERO` +
  `OpenSkyhawk:MODULE_RP2040-ZERO` + 3D STEP), registered in the project-template `sym-lib-table`.
- Milestone 2: scaffold both KiCad projects (`/new-kicad-project`), capture schematics from the
  diagrams here, ERC clean; then layout, DRC, gerbers/BOM.
