# Build Flags

Every OpenSkyhawk firmware project is configured by `-D` flags in its `platformio.ini`. There are
**32 of them**, and they come from three different places. That mixed provenance is the first
thing to understand, because it decides who you have to convince before you change one:

| Group | Count | Changing one is… |
|---|---|---|
| [**Ours — behavioural**](#ours-behavioural-13) | 13 | a local decision — we own the code that reads it |
| [**Third-party, set by us**](#third-party-set-by-us-7) | 7 | **not** a local decision — STM32duino, arduino-pico, and DCS-BIOS own these |
| [**Test seams**](#test-seams-12) | 12 | bench-only — never present in a shipped build |

This page is an inventory: what each flag defaults to, where it's set, and where it's read. Deep
per-flag semantics live in the library READMEs under `Firmware/Libraries/` and in
`Firmware/ScratchPad/TechSpec/`. When a table here and a header disagree, **the header wins** —
and the [grep recipe](#re-verifying-this-page) at the bottom will tell you which.

For a working `platformio.ini` with the common flags in context, see
[PlatformIO Setup](platformio-setup.md). If you are building a panel right now, start with the
next section instead — the tables below are reference, not a checklist.

## Starting a new panel

You copied `Firmware/Templates/PanelGroup/` and you're looking at its `build_flags`. Here is what
to do with each line. The [walkthrough](../guides/new-panel-group.md) covers the surrounding steps;
this is just the flags.

### Required — set these or the build fails

| Flag | What to set |
|---|---|
| `NODE_ID` | Your board's unique CAN address, 1–63, claimed from the registry in `Firmware/NODE_IDS.md` — see [NODE_ID & CAN Addressing](node-id.md). **No default.** Omitting it is a compile error, not a silent fallback. |
| `HAL_CAN_MODULE_ENABLED` | Nothing — keep it as-is. STM32duino does not enable the bxCAN HAL driver by default, so without this line there is no CAN driver to link against. |

### Already correct in the template — keep, don't tune

| Flag | Why to leave it alone |
|---|---|
| `HSE_VALUE=8000000` | Already matches both your board's crystal and the framework's own F1 default. Change it only if your board genuinely carries a different crystal — and if you do, get it right first time: a wrong value is **undetectable at runtime** (see the warning below). |
| `USB_NONE` | Inert. It documents that CAN owns PA11/PA12; it does not enforce it. Harmless to keep, harmless to drop. |

### Add only if your panel needs them

| Flag | Add when |
|---|---|
| `I2C_TIMEOUT_TICK=10` | Your panel has I²C devices (MCP23017, ADS1115, OLEDs) — shortens the stall when one is absent or miswired. |
| `SHIFTBUS_SCK` / `MISO` / `MOSI` / `LOAD` / `LATCH` | You are relocating the shift-register bus off its default pins. Otherwise leave all five out. |
| `SHIFTBUS_ISR_HZ` | You want timer-ISR sampling for encoder feel instead of polled sampling. |
| `SERIAL_RX_BUFFER_SIZE=256` | **PanelBridge only.** A PanelGroup has no use for it. |
| `NODE_HEALTH_TELEM=0` | You specifically want health telemetry **off**. It is on by default — omitting the line does not disable it. |

### Never in a panel

`FORCE_CLOCK_FALLBACK`, `PINREF_DEBUG`, `NODE_OVERHEAT_C`, and any `*_TEST` seam. These are bench
and debug instruments. If one appears in a panel's `platformio.ini`, it is a mistake — see
[Test seams](#test-seams-12) and [Deliberate non-defaults](#deliberate-non-defaults) for what each
actually does.

## Ours — behavioural (13)

| Flag | Default | Set where | Read at |
|---|---|---|---|
| `NODE_ID` | none — omitting it fails to compile | every STM32 env that links our libraries; `WiringCheck` omits it deliberately | `CANProtocol.cpp:197`, `PanelGroup.cpp:391`, `STM32Board.h:29` |
| `PANELBRIDGE_NODE_STATUS` | off | on in `Templates/PanelBridge` | `PanelBridge.cpp:42` + ~10 more |
| `NODE_HEALTH_TELEM` | **on** | never set | `PanelBridge.cpp:468`, `PanelGroup.cpp:399` |
| `NODE_OVERHEAT_C` | **unset — by design** | bench envs only | `CANProtocol.cpp:396`, `PanelGroup.cpp:409` |
| `FORCE_CLOCK_FALLBACK` | off | never set | `STM32Board.cpp:149` |
| `PINREF_DEBUG` | off | never set | `PinRef.cpp:8` + ~9 more |
| `SHIFTBUS_SCK` | `PB3` | never set | `ShiftBus.cpp:13` |
| `SHIFTBUS_MISO` | `PB4` | never set | `ShiftBus.cpp:16` |
| `SHIFTBUS_MOSI` | `PB5` | never set | `ShiftBus.cpp:19` |
| `SHIFTBUS_LOAD` | `PB8` | never set | `ShiftBus.cpp:22` |
| `SHIFTBUS_LATCH` | `PB9` | never set | `ShiftBus.cpp:25` |
| `SHIFTBUS_ISR_HZ` | unset → polled sampling | `Tests/ShiftBus`, `=1000` | `PanelGroup.cpp:125,240` |
| `SHIFTBUS_ISR_TIM` | `TIM2` | never set | `PanelGroup.cpp:241` |

**What they do, briefly.** `NODE_ID` is the board's CAN address — see
[NODE_ID & CAN Addressing](node-id.md). `PANELBRIDGE_NODE_STATUS` makes PanelBridge report which
PanelGroup nodes it can see, up to the host over DCS-BIOS. `NODE_HEALTH_TELEM` is the periodic
health frame (die temperature, fault bitmap) at half the heartbeat rate.
`FORCE_CLOCK_FALLBACK` exercises the dead-crystal fault path without a dead crystal.
`PINREF_DEBUG` adds assertions to the pin abstraction. The `SHIFTBUS_*` pin flags relocate the
shift-register bus off its default SPI1-remap pins; `SHIFTBUS_ISR_HZ` switches that bus from
polled to timer-ISR sampling (encoder feel) on the timer named by `SHIFTBUS_ISR_TIM`.

!!! warning "Set `NODE_ID` in `platformio.ini`, never in `main.cpp`"
    A `#define NODE_ID` in a sketch is invisible to library translation units — `CANProtocol` and
    `PanelGroup` are compiled separately. That mistake is caught at build time rather than
    shipped: those TUs include `STM32Board.h`, whose file-scope `static_assert` fails with
    `error: 'NODE_ID' was not declared in this scope`. There is no default and no silent
    fallback — the undefined-evaluates-to-zero rule applies inside `#if`, not in a
    `static_assert` expression.

    Sketches that link none of those libraries are the only exception: the standalone
    `Examples/E2E_DCS_Test/WiringCheck` env omits `-DNODE_ID` deliberately, because it is a raw
    GPIO/ADC diagnostic with no OpenSkyhawk libraries at all.

    Note that `0` is a legal value — it is PanelBridge's reserved address. It is just never
    correct for a PanelGroup node.

### Deliberate non-defaults

Three of the above look like oversights and are not. They are choices, and this is the record of
them:

!!! note "`NODE_OVERHEAT_C` is unset on purpose — OVERHEAT never raises in a shipped build"
    The STM32's internal die-temperature sensor is **uncalibrated**. Rather than ship a threshold
    nobody has field data for, the overheat bit is computed only when a build defines
    `NODE_OVERHEAT_C` — and **no shipped environment defines it**: not the templates, not
    `Panels/Center_Armament`, not the E2E examples. The `OVERHEAT` flag in the node-health frame is
    therefore never set on any real cockpit node.

    Two bench environments force it deliberately, and only to exercise the path:
    `Tests/CANProtocol` sets `=0` (any reading trips it, to test overheat and degraded coexisting)
    and `Tests/PanelGroup` sets `=20` (a room-temperature node reports overheat, to test the
    client's rendering). Enabling it for real is a one-line `-DNODE_OVERHEAT_C=N` once someone has
    calibration data. See `FirmwarePlan/00-decisions.md`.

!!! note "`NODE_HEALTH_TELEM` is ON by default — disable with `=0`, not by omission"
    It's the only flag in this group that's on when absent (`#if !defined(X) || (X)`). Leaving it
    out gets you health telemetry; you need an explicit `-DNODE_HEALTH_TELEM=0` to turn it off.

!!! note "`PANELBRIDGE_NODE_STATUS` is off in the library but on in every shipped PanelBridge"
    The library defaults it off, so the table above says "off" — but the PanelBridge template
    enables it, and so does every PanelBridge environment in the tree. In practice it ships on.
    The client half is already released, which is why the default flipped in the template rather
    than in the library.

## Third-party, set by us (7)

These belong to **STM32duino**, **arduino-pico**, and the **DCS-BIOS** library. We only set them —
we don't own the code that reads them, and a framework update can move a default out from under
us. Two of these have already cost real debugging time.

| Flag | Framework default | Set where | Read at |
|---|---|---|---|
| `HAL_CAN_MODULE_ENABLED` | **not** enabled | every STM32 env | `stm32f1xx_hal_conf_default.h:321` |
| `HSE_VALUE` | already `8000000U` on F1 | every STM32 env, `=8000000` | `stm32f1xx_hal_conf_default.h:82` |
| `SERIAL_RX_BUFFER_SIZE` | `64` | PanelBridge envs, `=256` | `HardwareSerial.h:41` |
| `I2C_TIMEOUT_TICK` | `100` | one E2E PanelGroup env, `=10` | `Wire/src/utility/twi.c:48` |
| `USB_NONE` | *no such macro* | 10 `platformio.ini` files | **nowhere** — see below |
| `USE_TINYUSB` | off | every RP2040 env | arduino-pico `platformio-build.py:242,325` |
| `DCSBIOS_DEFAULT_SERIAL` | off | `#define` in the sketch, not `-D` | `PanelBridge.cpp:11` |

`HAL_CAN_MODULE_ENABLED` is genuinely required: CAN is absent from STM32duino's default-enabled
HAL module list, so without our `-D` there is no bxCAN driver to link against.
`SERIAL_RX_BUFFER_SIZE=256` gives PanelBridge room for DCS-BIOS export bursts on UART2.
`I2C_TIMEOUT_TICK=10` shortens the Wire library's stall when an I²C device is absent.
`DCSBIOS_DEFAULT_SERIAL` is the odd one out — it's `#define`d at the top of the sketch's
`main.cpp` rather than passed as `-D`, because `PanelBridge.cpp` has to include `DcsBios.h`
*without* it and then restore it, to avoid an ODR violation.

!!! warning "`HSE_VALUE` does not select HSE — and it already matches the framework default"
    `-DHSE_VALUE=8000000` tells HAL what the crystal is. It does **not** switch the clock tree to
    it, and `8000000U` is already what STM32duino defaults the F1 to, so the flag changes nothing
    on its own. What actually selects HSE is `STM32Board`'s strong override of
    `SystemClock_Config` (`STM32Board.cpp:137`), which every OpenSkyhawk STM32 node links.
    Without it, the variant runs HSI-PLL at 64 MHz → APB1 32 MHz → CAN at 444 kbps against a
    500 kbps bus.

    The corollary: a **wrong-value** crystal is undetectable at runtime. `HAL_RCC_Get*Freq()`
    derives from the compile-time `HSE_VALUE`, not from a measurement, so a 12 MHz part would
    compute as 72 MHz while really running 108. Correct crystal value is a BOM guarantee.

!!! warning "`USE_TINYUSB` is required for HID — omitting it silently builds the wrong USB stack"
    On RP2040 this selects TinyUSB in place of the default Pico stack. `Adafruit_TinyUSB.h` and
    SimGateway's whole HID backend depend on it. The arduino-pico builder branches on it in two
    places; without it the project still compiles, which is what makes it expensive to diagnose.

!!! note "`USB_NONE` is inert — nothing reads it"
    It appears on 25 lines across 10 `platformio.ini` files and has **no effect**. STM32duino has
    no such macro: its "USB support: None" board option sets nothing at all, and only the CDC and
    HID options add defines. Nothing in the framework, the PlatformIO builder, or our code reads
    `USB_NONE`.

    The intent behind it is real, though. On the F103, USB D+/D− are PA11/PA12 — exactly the pins
    the CAN transceiver uses, so CAN genuinely does own them and USB must stay off. **What keeps
    it off is that `USBCON` is never defined, and no build config or source under `Firmware/`
    defines it.** The flag documents the decision; it does not enforce it.

## Test seams (12)

Each of these widens a library's visibility so its on-target tests can assert against internal
state. **A seam never changes shipped behaviour, and no shipped environment defines one** — each
is set only by its own `Firmware/Tests/<Library>/platformio.ini`.

| Flag | Library it opens | First read at |
|---|---|---|
| `ANALOGINPUT_TEST` | AnalogInput | `Inputs/AnalogInput/AnalogInput.h:72` |
| `ANALOGMULTIPOS_TEST` | AnalogMultiPos | `Inputs/AnalogMultiPos/AnalogMultiPos.h:90` |
| `DRUMDISPLAY_TEST` | DrumDisplay | `DrumDisplay/DrumDisplay.h:246` |
| `LED_TEST` | LED | `Outputs/LED/LED.h:68` |
| `MULTIPOS_TEST` | MultiPosInput | `Inputs/MultiPosInput/MultiPosInput.h:60` |
| `NEEDLEGAUGE_TEST` | NeedleGauge | `Outputs/NeedleGauge/NeedleGauge.h:78` |
| `PANELBRIDGE_TEST` | PanelBridge | `PanelBridge/PanelBridge.h:84` |
| `ROTARYENCODER_TEST` | RotaryEncoder | `Inputs/RotaryEncoder/RotaryEncoder.h:96` |
| `SHIFTBUS_TEST` | ShiftBus | `Helpers/ShiftBus/ShiftBus.h:141` |
| `SIMGATEWAY_TEST` | SimGateway | `SimGateway/SimGateway.cpp:11` (no-op HID stubs) |
| `STEPPERMOTOR_TEST` | StepperMotor | `Drivers/StepperMotor/StepperMotor.h:158` |
| `STM32BOARD_TEST` | STM32Board | `STM32Board/STM32Board.h:32` |

Paths are relative to `Firmware/Libraries/`, and the PanelGroup ones to
`Firmware/Libraries/PanelGroup/`. Most seams are read in several places; the column gives the
first, which is where the guarded block starts.

Two test projects set a second library's seam because they drive it: `Tests/NeedleGauge` also sets
`STEPPERMOTOR_TEST`, and `Tests/AnalogMultiPos` also sets `MULTIPOS_TEST`. Five test projects have
no seam of their own at all — `Tests/PinRef`, `Tests/CANProtocol`, `Tests/NodeStatus`,
`Tests/Switch2Pos` and `Tests/PanelGroup` test through the public API.

`SIMGATEWAY_TEST` is the one that does more than widen visibility: it swaps the real HID backend
for no-op stubs so SimGateway logic can be tested without a USB host.

## Re-verifying this page

Rather than trust the tables, re-derive them. Run both blocks from the repo root — **their union
must be 32**. If it isn't, this page is stale.

Be clear about what that proves. Both lists come from greps of the same shape, so the union
check catches **drift** — a flag added, removed, or renamed after this page was written. It does
not prove the inventory is **complete**: a flag consumed in some way neither grep looks for would
be missed by the check exactly as it was missed by the page. Treat 32 as the current count, not a
guarantee.

```bash
grep -rhE '^[^;#]*-D[A-Za-z_]' --include=platformio.ini Firmware | sed -E 's/[;#].*//' | grep -oE '[[:space:]]-D[A-Za-z_][A-Za-z0-9_]*' | sed 's/.*-D//' | sort -u
```

That's every flag **set** by some environment — 22 of them.

```bash
grep -rhoE '#[[:space:]]*(ifdef|ifndef|if|elif)[^A-Za-z_]*defined[^A-Za-z_]*[A-Za-z_][A-Za-z0-9_]*|#[[:space:]]*(ifdef|ifndef)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*' --include='*.h' --include='*.cpp' Firmware/Libraries | grep -oE '[A-Za-z_][A-Za-z0-9_]*$' | sort -u | grep -vE '^(ARDUINO_ARCH_|STM32F1xx$|ATEMP$|guard$|_)'
```

That's every flag **read** by a library conditional — 25 of them. The trailing filter drops
architecture macros, the `ATEMP` pin macro, and internal include guards; none are build flags.

The two lists deliberately don't match, and the differences are the interesting part:

- **Set but not read** — `NODE_ID` (used as a *value*, `canIdEvt(NODE_ID)`, never in a
  conditional) plus the six framework flags, which are read inside STM32duino and arduino-pico
  rather than in our tree.
- **Read but never set** — the nine flags whose defaults always apply in every committed
  environment (the five ShiftBus pins, `SHIFTBUS_ISR_TIM`, `NODE_HEALTH_TELEM`,
  `FORCE_CLOCK_FALLBACK`, `PINREF_DEBUG`), plus `DCSBIOS_DEFAULT_SERIAL`, which is `#define`d in
  sketch source instead of passed as `-D`.

To check the `USB_NONE` claim specifically, both of these must return nothing:

```bash
grep -rn 'USBCON\|USBD_USE_' Firmware
```

```bash
grep -rn 'USB_NONE' ~/.platformio/platforms ~/.platformio/packages/framework-arduinoststm32
```
