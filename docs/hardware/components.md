# Component Library

The standard parts that recur across OpenSkyhawk boards, with their KiCad symbols. Most are
KiCad built-ins; a few are in the shared `OpenSkyhawk` library. For how the library is wired up,
see [KiCad Workflow](kicad-workflow.md).

## Active parts

| Part | Package | Role | KiCad symbol |
|------|---------|------|--------------|
| STM32F103C8 / CB | LQFP48 | CAN node MCU (C8 nodes, CB PanelBridge) | `MCU_ST_STM32F1:STM32F103CBTx` |
| SN65HVD230 | SOIC-8 | CAN transceiver, 3.3 V | `Interface_CAN_LIN:SN65HVD230` |
| MCP23017 | SOIC-28 | I²C digital I/O expander (16-bit) | `Interface_Expansion:MCP23017x-x-SO` |
| ADS1115 | — | I²C 16-bit ADC, 4 channels | `Analog_ADC:ADS1115` |
| AMS1117-3.3 | SOT-223 | 5 V → 3.3 V LDO (every board) | `Regulator_Linear:AMS1117-3.3_SOT223` |
| IRLML2502 | SOT-23 | LED zone MOSFET (low-side, logic-level) | `OpenSkyhawk:IRLML2502` |
| 8 MHz crystal | — | HSE for CAN timing | `Device:Crystal` |
| LED 5050 (red) | 6-pad 5050 | Panel backlight (5-in-series strings) | `OpenSkyhawk:LED_5050_Red` |

## Gauge steppers

| Part | Role | KiCad symbol |
|------|------|--------------|
| X27.589 | Gauge needle stepper (e.g. cabin pressure) | `OpenSkyhawk:X27.589_Stepper` |
| X27.168 | Gauge needle stepper (single-mount variant) | `OpenSkyhawk:X27.168_Stepper` |

!!! warning "Stepper driver IC — TBD"
    The motor driver for the gauge steppers is **not yet finalised** — it's under bench
    evaluation. Don't design a board around a specific driver part until that's resolved; the
    motor supply rail (5 V) is settled, the driver IC is not.

## High-current boards only

| Part | Package | Role | KiCad symbol |
|------|---------|------|--------------|
| AP63205WU | SOT-23-6 | 12 V → 5 V switching buck | `OpenSkyhawk:AP63205WU` *(pending — add when first needed)* |

Standard MCU and breakout boards do **not** carry this — they draw 5 V from the bus. It's only
for future high-5V-current boards. See [Power Architecture](../architecture/power.md).

## Notes

- The custom `OpenSkyhawk` symbols (LED 5050, IRLML2502, the steppers) live in
  `PCB/Libraries/OpenSkyhawk.kicad_sym` + `OpenSkyhawk.pretty/`. Everything else is a KiCad
  built-in — no custom entry needed.
- SN65HVD230 3.3 V clones (e.g. VP230) are acceptable for JLCPCB assembly.
- The **MCP23017 GPA7/GPB7 input restriction** (silicon bug — output only) is covered in
  [Hardware Standards](standards.md).
