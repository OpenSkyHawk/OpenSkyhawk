# Power Architecture

How power is distributed across an OpenSkyhawk cockpit. The firmware/hardware contracts cover
the logic-side rails in detail; high-current actuator boards are not yet designed and are
marked TBD below.

## Distribution

A PC ATX power supply feeds the main bus with **12 V and 5 V**, distributed between
controller groups over **Molex Mini-Fit Jr** connectors. **Each board generates its own
3.3 V locally** — the 3.3 V rail is never distributed across the bus.

| Rail | Source | Used for |
|------|--------|----------|
| 12 V | ATX PSU, main bus | LED backlight strings; **servos, when fitted, via a local 5/6 V buck** (never off logic 5 V); input to local 5 V buck on high-current boards |
| 5 V | ATX PSU, main bus | Input to the local 3.3 V regulator; stepper driver motor supply (VM) |
| 3.3 V | Generated **on each board** | STM32 / RP2040 logic, MCP23017, ADS1115, SN65HVD230 |

## Local 3.3 V regulation

Every MCU and breakout board carries an **AMS1117-3.3** LDO (SOT-223) that drops the bus 5 V
to 3.3 V. The 1.7 V drop is acceptable at the boards' logic load (≤ ~175 mA). Local
decoupling is required on every board: **100 nF + 10 µF per rail**, placed close to each IC.

For the rare board that needs significant 5 V current, an **AP63205WU** switching buck
(SOT-23-6) converts 12 V → 5 V on-board. This is **not** used on standard MCU or breakout
boards — only on future high-5V-current boards.

!!! warning "Never use a linear regulator for 12 V → 5 V"
    The 12 V → 5 V step is done with a switching buck (AP63205WU) where needed, never an LDO —
    the drop would dissipate too much power. The AMS1117 LDO is only for the small 5 V → 3.3 V
    step.

## Board power budget — logic + LED boards

Standard **logic + LED** boards are designed to stay within **≤ 500 mA at 12 V input** (a
servo-driving board runs higher — see *Servos* below):

| Rail | Typical | Max expected |
|------|---------|--------------|
| 12 V → LEDs | 54–180 mA | ~360 mA (large panel) |
| 12 V → AP63205 input (if fitted) | ~100 mA | ~150 mA |
| **Total 12 V per board** | ~160–280 mA | ~510 mA (large-panel edge case) |
| 5 V → stepper driver VM | 15–30 mA | 50 mA |
| 3.3 V → STM32 + MCP23017 + CAN | ~125 mA | ~175 mA |

Across a full ~20-board cockpit that's roughly **3 A at 12 V** today (LED-only), plus ~2 A at 5 V — a
small fraction of what the ATX supply delivers. **No servos are fitted yet;** when they are, powering
them off 12 V (below) adds ~1–1.5 A per console → **~6–8 A** cockpit-wide, so leave headroom. The
build uses a fully-modular **GOLDEN FIELD NX650**, heavily over-provisioned; a 350–450 W single-rail
ATX would be ample.

### Servos — a 12 V load (planned guideline)

**No servos in the cockpit yet — this is how they should draw power when added.** Run them off the
**12 V line through a local 5/6 V buck**, **never logic 5 V**: five to six servos per console draw
~2–3 A running (up to ~6 A stall), which on the shared 5 V logic rail means brownouts and I²C
glitches. On 12 V the same load is **~1–1.5 A per console** (the buck roughly halves rail current)
and stays isolated from logic; add a local bulk cap + polyfuse. A servo-driving board therefore
**exceeds the ≤ 500 mA logic+LED figure above** — budget servos separately as a 12 V load, captured
by that console's 12 V current monitoring.

## Source protection — fused per-console injection

The ATX feed splits into **per-console segments**, and each segment is injected through its own
**fuse per rail**. A console's fuse is sized to that console's draw and **protects every board
downstream of it** on that rail — a fault in one console opens only that console's fuse, sparing
the rest of the cockpit and its harness.

- **One fuse per rail per console**, at the injection point (12 V and 5 V). **GND and CAN are
  never fused** — they stay common and continuous as the bus reference and return.
- **Sized to consumption; copper sized to the fuse.** The rating sits above the segment's peak draw
  (with margin), and the fused power traces are then sized to clear it — **5 mm of 1 oz copper carries
  ~7 A**, comfortably above the 5 A / 4 A blanket. (A 1 mm trace is ~2.5 A — that's a local sub-1 A
  tap only, never the fused trunk.) A console draws little (~1 A at 12 V), so slow-blow fuses in the
  low single amps are typical — slow-blow because LED strings and buck inputs inrush at power-on.
- **Granularity follows consumption.** A console is the default injection zone; a **power-hungry
  group can get its own dedicated PDU** — 12 V/5 V straight from the PSU, its own fuse sized to just
  that group — so it is isolated and does not load the shared console feed. The PSU sources ~54 A at
  12 V (ample headroom); the per-zone fuse exists to **protect the downstream harness and contain a
  fault**, not to ration the supply.
- **The PSU's own OCP protects the PSU, not your harness** — a single-rail ATX only trips at tens
  of amps, long after a thin lead would cook. The per-zone fuse is what actually protects the
  wiring.
- A planned **power-distribution CAN node** adds live per-console current telemetry and active
  eFuse breaking **on top of** — not instead of — these baseline fuses.

!!! note "Exact values live in the hardware source"
    Fuse ratings, trace widths, and injection copper are specified in the base-boards hardware
    source; this page is the overview.

## Cross-tier notes

- **CAN transceiver** (SN65HVD230) runs directly from the 3.3 V rail — no level shifter.
- **RP2040 SimGateway** is bus-powered from USB. If it's co-located with STM32 CAN hardware,
  **share GND only** — do not tie the RP2040 module's 3.3 V to the STM32 board's 3.3 V.
- **Stepper driver motor supply (VM)** runs from the **5 V** rail. The driver is a **DRV8833**
  dual H-bridge (four logic inputs per motor).

## TBD — not yet specified

!!! note "Marked TBD because it isn't in source material yet"
    - **Actuator boards** (solenoids, large steppers, servos) are **not yet designed**. Servos'
      power sourcing is already decided (12 V → local buck, above); the rest — flyback protection,
      dedicated supply rails, driver selection — is open. Each will need its own power-budget
      analysis before PCB work. The established boundary is that logic +
      LED boards stay ≤ 500 mA at 12 V; actuator boards are sized to their specific loads and
      require separate design review.
