# Power Distribution Unit (PDU)

Each console gets one **PDU**: the board where that console's power comes in. It takes 12 V and
5 V from the supply, passes each rail through its own fuse, and feeds the console's bus — and it
measures everything it feeds. Voltage and current on both rails, plus the board's temperature, go
out over CAN to the host app about once a second, and at once when something goes wrong.

| | |
|---|---|
| **Role** | Per-console power injection, fusing and monitoring — a CAN node like any other |
| **Node IDs** | 3 (Left), 4 (Right), 5 (Center) |
| **Microcontroller** | STM32F103 — same core, CAN transceiver and status LED as the [base boards](base-boards.md) |
| **Measures** | 12 V and 5 V rail voltage and current, board temperature |
| **Fuses** | 12 V **5 A**, 5 V **2 A** — standard MINI blade |
| **Revision** | 0.2.0 |

## What it does

- **Fuses each rail at the injection point** — a fault in one console opens only that console's
  fuse. Fuses are swappable automotive MINI blades, so a rating can change without touching the
  board.
- **Measures what it feeds** — rail voltage (average and lowest dip), current (average and peak),
  and board temperature, sampled a thousand times a second. Logging a console through idle,
  startup, full lighting and its peaks shows its real load, so its fuses can be sized from data.
- **Raises the alarm** — a rail sagging below or rising above its limits, a current near the fuse
  rating, a blown 12 V fuse, or the board running hot. A fault turns the node amber in the host
  app's node list and is reported straight away, not on the next poll.
- **Shows its rails without the microcontroller** — three green power LEDs, one each for 12 V, 5 V
  and 3.3 V, are wired straight to their rails and light whenever that rail is present, even if
  the firmware isn't running.

!!! note "What the LEDs do and don't show"
    Rail faults are reported to the host app only. The red/green **status LEDs** work exactly as on
    every other board — CAN link and bus health (see [Debugging](../firmware/debugging.md)) — and
    don't change for a rail fault. The green **power LEDs** only show that a rail is present: one
    goes out on a dead rail or a blown fuse, but all stay lit through an over-voltage,
    over-current or overheat.

## Where it sits

```
PSU ──► PDU (fuse + measure) ──► console bus ──► that console's boards
```

Ground and CAN run continuously through every PDU; only 12 V and 5 V are split per console.

!!! warning "Power never chains past a PDU"
    A console's downstream harness must not feed another PDU's console. A blown fuse then leaves
    that console's rail dead — which is exactly how the PDU detects it. See the wiring
    assumptions on [Power](../architecture/power.md).

!!! warning "Power-in pinout — crimp the harness to the board"
    The 4-pin Mini-Fit Jr power input is **pin 1 = +12 V · pin 2 = +5 V · pin 3 = GND ·
    pin 4 = GND**. +12 V and +5 V sit side by side — check the harness against this before
    first power-up.

A blown **5 V** fuse powers the PDU itself down, so the host app shows that node going offline
rather than a fuse alert. The Gateway/Bridge head node also takes its power from one of the PDUs —
if the whole cockpit drops off at once, check the PDU fuses first.

## Bring-up and calibration

Each PDU is calibrated once, at bring-up. The values are stored on the board and survive firmware
updates.

1. **Flash** the PDU for its console, e.g. `pio run -d Firmware/Panels/PDU_Left -t bootstrap` on a
   new board — it puts the [bootloader](../firmware/bootloader.md) on too — or `-t upload` on one that
   already has it (`PDU_Right` / `PDU_Center` for the others — the code is identical, only the node ID
   differs).
2. **Open the debug serial console** at 115200 baud. The PDU prints its node ID and calibration
   state at boot; `CAL SHOW` prints it again.
3. **Supply reference:** measure the board's 3.3 V rail with a multimeter and enter it, in
   millivolts — `CAL VDDA 3297`. Until this is done the PDU assumes exactly 3.3 V.
4. **Current zero:** disconnect the console load, then run `CAL ZERO`. The PDU refuses if it still
   sees current flowing. The 5 V reading includes the PDU's own small draw, so afterwards it
   shows the console's load only.

`RAW` prints the live readings at any time, and `CAL CLEAR` returns the board to uncalibrated.

!!! note "Thresholds are starting points"
    The limits — under / over-voltage, over-current (90 % of each fuse), temperature — are
    defaults to tune against your console's measured load. Each can be changed per build without a
    code change; the list is on [Build Flags](../firmware/build-flags.md).

## Where it lives

- **KiCad project:** `PCB/Base/Power/PDU/` — schematic, layout, BOM, and `DESIGN-NOTES.md` with the
  full parts rationale
- **Firmware:** the `PowerMonitor` library and the `Firmware/Panels/PDU_*` sketches
- **How it fits the power system:** [Power](../architecture/power.md); the telemetry frame on
  [CAN Bus](../architecture/can-bus.md)
