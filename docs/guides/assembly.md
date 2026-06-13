# Assembly & Soldering

How an OpenSkyhawk board goes from bare PCB to populated and inspected. The part choices are all
deliberately reflow-and-inspect friendly, so a hobbyist setup handles them.

!!! note "Photos coming"
    This guide is text-complete but the step-by-step photos are still being produced — image
    placeholders are marked **(photo TBD)**.

## What you need

- Soldering iron (fine tip) and/or a **T962 reflow oven** with solder paste and a stencil
- Multimeter (continuity + voltage)
- Flux, solder wick, tweezers, magnification for inspection

## Why everything is inspectable

Every package on these boards is chosen so the joints are **visible after reflow** — SOIC, SSOP,
TSSOP, HTSSOP, LQFP, SOT-23/223, through-hole. No QFN/DFN/BGA. That means you can verify every
critical joint by eye (and the HTSSOP thermal pad by continuity). See
[Hardware Standards](../hardware/standards.md).

## Placement

- **LEDs on the front face; everything else on the back** — ICs, passives, connectors, MOSFETs.
- Through-hole connectors are vertical and accessible from the panel side.
- *(photo TBD — populated front and back)*

## Order of work

1. **Paste + reflow the back-side SMD** (ICs, passives, MOSFETs, regulator). *(photo TBD)*
2. **Inspect** every joint — bridges, tombstones, the AMS1117 and any HTSSOP thermal pad
   (continuity-check the pad to GND). *(photo TBD)*
3. **Hand-solder through-hole** — connectors, headers, crystal.
4. **Front-side LEDs** last (5050 strings; mind the anode notch — the chamfered corner is the
   anode on the project's 5050 part).
5. **Power-on check before anything else** — verify the 3.3 V rail and look for shorts before
   the first flash. Continue at [Bring-Up & Testing](bring-up.md).

## Connectors & harnesses

Crimp harnesses per the [Connector & Harness Guide](../hardware/connectors.md) — Molex Mini-Fit
Jr for power/bus, JST-XH for signal. 24 AWG throughout.

!!! warning "Don't power a board you haven't inspected"
    A solder bridge on the 3.3 V rail can take out the regulator or the MCU. Inspect and
    continuity-check first, then power.
