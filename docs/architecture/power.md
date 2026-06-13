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
| 12 V | ATX PSU, main bus | LED backlight strings; input to local 5 V buck on high-current boards |
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

Standard MCU and breakout boards are designed to stay within **≤ 500 mA at 12 V input**:

| Rail | Typical | Max expected |
|------|---------|--------------|
| 12 V → LEDs | 54–180 mA | ~360 mA (large panel) |
| 12 V → AP63205 input (if fitted) | ~100 mA | ~150 mA |
| **Total 12 V per board** | ~160–280 mA | ~510 mA (large-panel edge case) |
| 5 V → stepper driver VM | 15–30 mA | 50 mA |
| 3.3 V → STM32 + MCP23017 + CAN | ~125 mA | ~175 mA |

Across a full ~20-board cockpit that's roughly **2.3 A at 12 V** — well under 10% of what a
500–600 W ATX supply delivers on its 12 V rail.

## Cross-tier notes

- **CAN transceiver** (SN65HVD230) runs directly from the 3.3 V rail — no level shifter.
- **RP2040 SimGateway** is bus-powered from USB. If it's co-located with STM32 CAN hardware,
  **share GND only** — do not tie the RP2040 module's 3.3 V to the STM32 board's 3.3 V.
- **Stepper driver motor supply (VM)** runs from the **5 V** rail. The specific driver part is
  still being validated on the bench — see TBD below.

## TBD — not yet specified

!!! note "Marked TBD because it isn't in source material yet"
    - **Stepper driver selection.** The motor supply rail (5 V) is settled, but the driver IC
      itself is still under bench evaluation and is intentionally left unspecified here.
    - **Actuator boards** (solenoids, servos, large steppers) are **not yet designed**. Each
      will need its own power-budget analysis before PCB work — flyback protection, dedicated
      supply rails, and driver selection are all open. The established boundary is that logic +
      LED boards stay ≤ 500 mA at 12 V; actuator boards are sized to their specific loads and
      require separate design review.
