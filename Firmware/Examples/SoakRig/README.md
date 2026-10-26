# SoakRig — firmware for the #291 soak test panel

Two PanelGroup node sketches, one per board, behind the permanent soak/smoke-test panel
described in [#291](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/291). Between them they
carry **one instance of every v1 control class on every PinRef backend**, with a single
deliberate exception: `AngleSensorInput` (#294) is compiled out, because #291 moved it off the
faceplate onto its own magnet-knob fixture. #290's gate is still met — the live check happens
there instead.

This is separate from `../E2E_DCS_Test/`, which keeps its own purpose (it is what #137 was
verified through) and is not changed by this.

| | Board A | Board B |
|---|---|---|
| `NODE_ID` | 1 | 2 |
| Base revision | 0.1.0 **or** 0.2.0 | **0.2.0** — it holds the ShiftBus stations |
| MCU | `genericSTM32F103C8` | `genericSTM32F103C8` — tight, see below |
| Flash | 74.0 % of 64 KB | **97.3 %** of 64 KB (63,756 B) |
| RAM | 18.6 % | 28.1 % |

Board A deliberately uses only `J11` pins that exist on **both** base revisions — positions 7
and 11 (`PB5`/`PA8` on 0.1.0, `PB2`/`PC14` on 0.2.0) are left alone — so it runs on the
already-assembled 0.1.0 board today.

## These are placeholders

Expect to move things around when the panel is actually wired. What is safe to change and what
is not:

| Provisional — move freely | Fixed |
|---|---|
| **Which board a station sits on** | A station on the ShiftBus needs a **0.2.0** board (`J_SR` does not exist on 0.1.0) |
| Which `J11` pin, MCP port/bit or `'165`/`'595` bit it sits on | No output on `J11` position 11 of a 0.2.0 board (`PC14`, ~3 mA) |
| The DCS control each station is bound to | One instance per **class × PinRef backend** — the coverage contract #291's gate checks |
| Stepper `home`/`park`/`minPos`/`maxPos` and `GaugeCal` | Only one deliberate duplicate: the two Dimmers, both on the same value, as a sync check |
| `AngleSensorInput` `centerDeg` / `travelDeg` | External pull-ups on every input |
| `DrumReadout` digit geometry (copied from APN-153) | |

The stepper and angle-sensor numbers are **invented starting values**, not bench results — both
need calibrating against real hardware.

### If you redistribute stations, watch the flash

Both nodes are C8s, and the flash budget follows the *grouping*, not the board letter. Board A
links at **74.0 % with no displays at all**, and U8g2 is roughly 15 KB — so keep all the OLEDs on
one node, and keep that node's other stations light. It cannot also carry the full MCP + ADS
input stack. Re-measure after any move; both projects build in a few seconds.

## Board B is a C8 — at ~97 % of flash

The assembled 0.2.0 base carries an STM32F103**C8**, so board B builds for it: **97.3 %** of flash
(63,756 of 65,536 bytes), and the `StepPattern::FULL_4STATE` alternate build the same, since its
config is optimised out until you swap it in.

**That figure is drifting upward, and the drift is not this sketch's.** It was 96.6 % when board B
moved to the C8, 96.9 % after merging the #295 freeze, 97.3 % after the next merge from `main` —
roughly 230 bytes of library growth per merge, with **1,780 bytes left**. Nothing here changed to
cause it. Re-measure after every merge rather than at the end, and if a merge overflows, take the
fallback order below instead of trying to trim the sketch. U8g2
plus three `StepperMotor` instances fills a C8, but the soak adds no code, so ~2 KB is enough to
run it.

If a fix or another alternate build overflows, in order:

1. **Move a non-display station to board A** (say the MCP23017 needle), which has ~17 KB free.
2. **Rework board B to an STM32F103CB** — pin-compatible on the same LQFP-48 footprint — and set
   `board = genericSTM32F103CB`.

## Using both I²C trunks

Both nodes use `J_I2C1` and `J_I2C2`. Every bus choice is made through our own API —
`PanelGroup::registerADC(adc, addr, bus)`, `I2cMux(addr, bus)`, `MCP23017(addr, bus)` — and
`PinRef` abstracts the backend entirely, so no control class needs to know which trunk it is on.
The STM32 core does not predefine a second `TwoWire` for these variants, so each sketch declares
one.

Board B's bare 0x3C panel sits on `J_I2C2`, the **non-default** trunk, and that is deliberate.
`DrumDisplay` gained a `(U8G2&, const DrumReadout&, TwoWire&, DrumFont, …)` overload in #318
(`c37f475b`), alongside the no-bus constructor that probes the first trunk. Putting the bare panel
on `Wire` would select the no-bus form and leave the new overload unexercised against hardware —
the same "frozen API that nothing runs" trap that `StepPattern::FULL_4STATE` and
`HomeMode::SENSOR` are on this rig to close before #295 freezes the API at 1.0.0. So the mux and
its two panels take `J_I2C1` and the bare panel takes `J_I2C2`.

`hardware-standards` only requires that a bare 0x3C device not share a trunk with 0x3C behind a
mux. Both arrangements satisfy that, so it does not pick a side — the overload coverage does.

## `StepperMotor`'s coil documentation is out of date

The constructor comment says coils are "GPIO or MCP23017", but `PinRef::writeDeferred()` handles
`Type::SR` (shift-register writes are already deferred, so a `'595` gauge gets an atomic
four-coil flip in one SPI frame). Board B drives a needle that way.

## Reflash variants, not extra hardware

Firmware-only differences are `#if 0` blocks in the sketches rather than duplicated stations:

| Variant | Where |
|---|---|
| `SwitchWithCover2Pos` (#293) | board A — reflash of the MCP 2-pos toggle |
| `Switch2Pos` | board A — reflash of the `J11` momentary (the GPIO backend) |
| `StepPattern::FULL_4STATE` | board B — reflash of the direct-GPIO needle |
| `AngleSensorInput` (#294) | board A — **not a rig station**; see below |

`ActionButton` and `Switch2Pos` are **siblings** off `InputBase`, not a hierarchy — edge-triggered
`EVT_ACTION` versus absolute 0/1 from the level — so they cannot share a pin and the second one
has to be a reflash. (`SwitchWithCover2Pos` is the class that derives from `Switch2Pos`.)

`FULL_4STATE` is a plain designated initializer over `StepperConfig`, whose fields now default to
the X27 family (#359) — everything it does not name keeps its default. It is spelled out rather
than built from `makeX27Config()` because that helper fixes the pattern and takes no pattern
argument: the older form had to overwrite the field after the call, and a version that merely
passed the helper's other parameters compiled, looked right, and ran the **6-state** table while
reporting the enum proven.

`AngleSensorInput` is the odd one out: it is not a reflash of anything, it is a station this rig
does not carry. It stays in the source, disabled, because board A's ADS is where it would land if
it ever came back. Leave it off while the rig soaks — nothing is wired to ADS channel 1, so a live
instance reads a floating input and streams `DCSIN_GUNSIGHT_KNB` noise onto the bus for hours.

`SwitchWithCover2Pos` takes a single pin and fakes the cover in firmware, so it is electrically
the same input as `Switch2Pos`; no guarded switch is fitted. The A-4E-C uses that class **0×** —
its only cover guards a 3-position switch the mod does not gate, and that cover has its own
`DCSIN`. It is kept for DCS-BIOS parity, and the check pairs the two armament station selects
(same broadcast word 0x8500, masks 0x0020/0x0040) so the 200 ms ordering is visible both on
screen and as two bit changes in one word.

## Wiring

Both sketches set plain `INPUT` on GPIO and leave `GPPU=0` on the MCP23017 — **every input needs
an external 10 kΩ pull-up**. Analog legs want the 1 kΩ + 100 nF RC at the connector end. The
per-pin map is in the header comment of each `src/main.cpp`; the panel-level drawing and the full
station list live on #291.

`ShiftBus1` needs no setup code: declaring an SR-backed `PinRef` is the whole opt-in, and
`PanelGroup::setup()` begins the bus.

**Both ShiftBus chains are two chips deep**, and both chips in each chain carry live bits. The
chain auto-sizes — `configureAsInput()` / `configureAsOutput()` call `noteInput(chip)` /
`noteOutput(chip)`, which grow the chain to cover the highest chip declared (`MAX_CHAIN` = 8 per
direction) — so a chip-1 `PinRef` is the entire opt-in.

| | chip 0 | chip 1 |
|---|---|---|
| `'165` | encoder, stepper home switch | ON-OFF-ON |
| `'595` | X27 coils, 2 caution LEDs | 2 caution LEDs |

The caution ladder is split deliberately. All four lamps come from one broadcast word, so a single
`MASTER_TEST` press has to fan out across the cascade hop inside one transfer; a tidy
chip-per-function split would not test that. The coils stay together on chip 0 so a step is still
one atomic four-bit flip. The ON-OFF-ON rather than the home microswitch goes on the far chip
because it is polled every loop — a dead `QH->SER` hop shows up at once, whereas the home switch
is only read during the boot seek and a fault there would hide until the next reset.

Note this is *not* the first exercise of the cascade: `Firmware/Tests/ShiftBus` already has
`bench_probe595` and `bench_probe165`, both two-chip walking-bit probes, and `frame_map` spans
chips 0 and 1. What is new here is the cascade carrying **live control classes under the soak**
rather than a standalone probe sketch.

Board A's momentary and its J11 LED are **one R16-503 illuminated pushbutton**
(`SW-PUSH-R16503-*`) — a single 16 mm hole, but still two independent stations, since the lamp is
driven from the sim rather than from the button contacts. Pressing it and watching its own lamp
light from the returning broadcast is the whole round trip in one part.
