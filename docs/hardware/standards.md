# Hardware Standards

The rules every OpenSkyhawk board follows. These aren't preferences — they're the constraints
that keep boards buildable, inspectable, and interchangeable across the cockpit. This page is
the hub; the detail lives on the pages it links.

## Component packages — must be inspectable

**Every package must be visually inspectable after reflow.** A T962 reflow oven handles the
soldering; the limit is *inspection*, not soldering. So bottom-terminated parts are out.

| Acceptable | Not acceptable |
|------------|----------------|
| SOIC, SSOP, TSSOP, HTSSOP | QFN, DFN, WSON |
| LQFP | BGA, LGA |
| SOT-23, SOT-223 | any fully bottom-terminated package |
| Through-hole | |

HTSSOP is fine — its side leads are the critical joints; the exposed thermal pad is verified by
continuity check.

## MCU

- **CAN nodes: STM32F103** (LQFP48). All boards default to the **`STM32F103C8`** (64 KB) —
  both PanelGroup nodes and **PanelBridge** (which carries the DCS-BIOS input map but still
  compiles to ~26 KB flash). The **`STM32F103CB`** (128 KB) is a drop-in fallback on the same
  footprint, needed by no board currently.
- Requires an **external 8 MHz crystal** — the internal RC oscillator isn't accurate enough for
  500 kbps CAN.
- **PA11/PA12** are shared between USB and CAN; pick one at init (CAN, in production).
- **HID controls: RP2040** off-the-shelf modules (flight stick, throttle, pedals, button boxes).
  No custom PCB.

## Power

A PC ATX supply distributes **12 V and 5 V** on the main bus; **each board makes its own 3.3 V**
locally with an AMS1117-3.3. Local decoupling (100 nF + 10 µF per rail) is required on every
board. Full detail — rails, budgets, the switching buck for high-current boards — is on the
[Power Architecture](../architecture/power.md) page.

## CAN transceiver

**SN65HVD230** (SOIC-8, 3.3 V), one per STM32 board, on PA11 (RX) / PA12 (TX). 120 Ω
termination across CANH/CANL at the **two end nodes only**. See
[CAN Bus Protocol](../architecture/can-bus.md) for the protocol side.

## LED backlighting

Confirmed by bench testing: **5-LED series strings**, MOSFET-switched per zone, one
current-limiting resistor per string.

- 5 × 5050 red LEDs in series per string (~10 V at 12 V supply, ~2 V resistor headroom)
- One resistor per string — **120 Ω default** (≈18 mA); 100 Ω for bright panels, 180 Ω for
  dim/night zones. (47 Ω was rejected — the resistor overheated.)
- One **IRLML2502** N-channel MOSFET per zone, low-side, gate driven directly by STM32 3.3 V PWM
  (no gate driver). PWM duty sets average brightness.
- **LEDs on the front face; resistors, MOSFETs, and everything else on the back.**

## Shift-register I/O — 74HC165 / 74HC595 (ShiftBus)

The SPI shift-register backend for sub-panel I/O, bench-decided against the MCP23017
(see [D9](../architecture/design-decisions.md)). Panels are **I²C-class or SPI-class by
their connector**, chosen by physics: rotary encoders or fast gauges (≳150 °/s) → SPI-class;
everything else is fine on I²C-class. Key rules:

- **74HC165 inputs:** bussed 10 k resistor array to 3V3 on **all 8 inputs of every chip**,
  used or not; switch/encoder commons to GND (active-low).
- **74HC595 outputs:** indicator LEDs ≤4 mA drive directly with a series resistor; anything
  above ~6 mA, any 5 V/12 V rail load, or a chip total nearing 70 mA gets a 2N7002 MOSFET.
- **DRV8833 motor supply is 5 V — never the 12 V rail** (10.8 V absolute maximum).
  Servos take 12 V through a panel-local buck, never a 5 V rail.
- **The SPI bus is dedicated** — a '165's data output never tristates, so the bus is the
  shift chains' alone. Chains daisy without limit; capacity never needs a second bus.
- **Standard pins:** SCK=PB3 · MISO=PB4 · MOSI=PB5 · LOAD=PB8 · LATCH=PB9 — one contiguous
  header run with I²C1. On mixed nodes the MCP23017 INT lines move to PB12/PB13.

Full rules, the measured numbers, and chip-placement guidance:
`docs/_source/hardware-standards.md` (Shift-Register I/O section). Harness pinouts:
the [Connector & Harness Guide](connectors.md).

## The other reference pages

- **[Mechanical Standards](mechanical-standards.md)** — screws, gauges, switch sizes, panel dims
- **[PCB Design Rules](pcb-design-rules.md)** — JLCPCB constraints, net classes, stackup
- **[Connector & Harness Guide](connectors.md)** — Molex Mini-Fit Jr, JST-XH, wire gauge
- **[Component Library](components.md)** — the selected parts and their KiCad symbols
- **[KiCad Workflow](kicad-workflow.md)** — shared libraries, design rules, the CLI

!!! warning "MCP23017 GPA7 / GPB7 — output only"
    A confirmed silicon bug means **GPA7 and GPB7 must never be configured as inputs** — the SDA
    signal can be corrupted if the pin voltage changes mid-transmission. That leaves **14 input
    pins per chip** (GPA0–GPA6, GPB0–GPB6). GPA7/GPB7 may drive LEDs or other outputs freely.
