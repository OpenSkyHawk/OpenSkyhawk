# E2E_DCS_Test — live-DCS integration rig

A three-board chain that drives one live instance of **every control class** with real DCS-BIOS
data, both directions:

```
DCS ⇄ SimGateway (RP2040, USB↔UART) ⇄ PanelBridge (STM32 NODE 0, CAN master) ⇄ PanelGroup (NODE 1)
```

- **`SimGateway/`** — RP2040, transparent DCS-BIOS-ASCII relay. **Never re-flashed** for input work;
  it demuxes by frame type, not command content.
- **`PanelBridge/`** — STM32 NODE 0, `main` firmware (DcsBios + the #147 dispatch + collapsed map).
- **`PanelGroup/`** — STM32 NODE 1, the ongoing integration node. Outputs (Master-Test LED,
  APN-153 DRIFT needle, two OLED drums) were bench-verified 2026-06-15; the **inputs below were
  added for the #147 live-DCS acceptance**.

## Bench sketches (PanelGroup side)

| Sketch | Use |
|---|---|
| `PanelGroup/` | the full "every control class" node — needs **all** its hardware present (LED, stepper, mux + 2 OLEDs) |
| `PanelGroupInputs/` | inputs-only node (4 inputs, no outputs) — use when only the inputs are wired |
| `WiringCheck/` | standalone raw GPIO/ADC diagnostic (no CAN/sim) — verify input wiring before the full chain |

> **Gotcha — don't flash the full `PanelGroup` node with its OLEDs absent.** It drives the OLED
> DrumDisplays every loop; with the OLEDs unplugged, the live DCS export keeps triggering renders to
> the missing devices, each I2C transaction blocks on the HAL timeout, the loop stalls, and the node
> flaps online/offline (heartbeat starved). On a partial-hardware bench use `PanelGroupInputs`. The
> proper fix — a non-blocking circuit breaker so a dead I2C device can't stall the loop — is #164.

## Inputs under test (PanelGroup node)

The four DCS-routed input **dispatch forms**, end-to-end — and the first **real-pot → DCS** run for
the analog ABS classes (previously only `debugSetRaw`-seam-verified):

| Control | Class | Pins | Form | Frame | DCS reaction |
|---|---|---|---|---|---|
| `DEST_LAT_KNB` | `RotaryEncoder` REL | PA8 / PB5 | `±step` → `%+d` | `canIdEvtRel` | Destination-latitude knob steps |
| `ARC51_FREQ_10MHZ` | `RotaryEncoder` DIR | PB3 / PB4 | `±1` → `INC`/`DEC` | `canIdEvtDir` | 10 MHz digit steps (and the ch1 drum tracks it) |
| `ARC51_VOL` | `AnalogInput` | PA2 (pot) | 16-bit → `%u` | `canIdEvt` | ARC-51 volume sweeps |
| `ARC51_MODE` | `AnalogMultiPos` | PA3 (pot) | index → `%u` | `canIdEvt` | ARC-51 mode selector steps (4 positions) |

> **PB3 / PB4 are JTAG-DP pins.** `setup()` calls `__HAL_AFIO_REMAP_SWJ_NOJTAG()` to release them
> while keeping SWD (PA13/PA14) — ST-Link still flashes. The node is already pin-dense (stepper coils
> on PA0/PA1/PA4/PA5, button PB0, mux PB8/PB9), so the DIR encoder lands on the remapped JTAG pair.

## Wiring (PanelGroup node side)

```
ENCODER REL  DEST_LAT_KNB (EC11)         ENCODER DIR  ARC51_FREQ_10MHZ (EC11)
  A  → PA8   + [10kΩ] → 3V3                A  → PB3   + [10kΩ] → 3V3   (JTAG pin, remapped)
  B  → PB5   + [10kΩ] → 3V3                B  → PB4   + [10kΩ] → 3V3   (JTAG pin, remapped)
  C  → GND                                 C  → GND
  switch pins ×2 → unconnected             switch pins ×2 → unconnected

POT 1  AnalogInput  ARC51_VOL (10kΩ)     POT 2  AnalogMultiPos  ARC51_MODE (10kΩ)
  end → 3V3 ,  wiper → PA2 ,  end → GND     end → 3V3 ,  wiper → PA3 ,  end → GND
  (no resistors)                           (no resistors)
```

**Pull-resistor rules:**
- **Encoders need 10 kΩ pull-UPs** on A and B → **3V3** (the class sets plain `INPUT`, no internal
  pull-up; an open contact would otherwise float). Common → GND. No pull-downs.
- **Pots need none** — a pot is already a divider. **High end → 3V3, not 5 V** (PA2/PA3 are ADC pins,
  *not* 5 V-tolerant; 5 V damages them).
- Direction only: if a knob turns the wrong way, swap that one's two signal pins (or pot end
  terminals); nothing else changes.

## Flash

| Board | Flash | Notes |
|---|---|---|
| PanelGroup STM32 (NODE 1) | `pio run -d Firmware/Examples/E2E_DCS_Test/PanelGroup -t upload` | this node, ~79 % of a C8 |
| PanelBridge STM32 (NODE 0) | `pio run -d Firmware/Examples/E2E_DCS_Test/PanelBridge -t upload` | DcsBios + #147 dispatch |
| SimGateway RP2040 | — | unchanged transparent relay |

CAN bus between the two STM32s, **120 Ω** terminator at each end, shared GND.

## Verify chain (per input)

1. **Node diag** (USART1 PA9/PA10, 115200): `[ENC] 0x… REL +3200` / `DIR +1`, `[ANA] …`, `[MUL] …`.
2. **Bridge diag** (its USART1): `[BRIDGE] DCS -> "DEST_LAT_KNB" "+3200"`.
3. **DCS cockpit**: the control in the table above moves. For DIR, the ch1 drum re-renders the new
   frequency — input and output close the loop on one node.

## Pass criteria

- **REL** — turning steps `DEST_LAT_KNB` smoothly both ways; `±3200` per detent (tune the ctor `step`
  for feel).
- **DIR** — steps the 10 MHz digit `INC`/`DEC`; **clamps at the band ends** (no wrap); a malformed
  value never moves it (bridge guards `±1`).
- **AnalogInput** — sweeping pot 1 moves volume continuously, settles without flicker.
- **AnalogMultiPos** — sweeping pot 2 steps cleanly through the 4 positions (deadband = no boundary
  flicker).
- **Preset-safety** — set a value in DCS, power-cycle the node: the encoders do **not** reset it on
  reconnect (relative, `forceReport()`-noop). The pots *do* re-assert on SYNC (absolute) — expected.

## Troubleshooting

- Wrong direction → swap that control's two signal pins (encoder A/B, or pot end terminals).
- One physical detent emits two EVTs → drop the encoder to `EncoderStepsPerDetent::Two`.
- DIR encoder dead while the rest work → the SWJ remap didn't run; confirm
  `__HAL_AFIO_REMAP_SWJ_NOJTAG()` is in `setup()` before `PanelGroup::setup()`.
- Node not `CONNECTED` on the bridge → check CAN wiring + the 120 Ω terminators + a shared GND.
