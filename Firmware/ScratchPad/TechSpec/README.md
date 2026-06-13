# TechSpec — Technical Specifications

## Purpose

This directory contains per-library and per-class technical specifications for the OpenSkyhawk
firmware. Each file defines the *how* — public API, class structure, method signatures, file
layout, and implementation notes.

**FirmwarePlan/** defines the *what* (contracts, data flows, design decisions).
**TechSpec/** defines the *how* (concrete code structure, ready for implementation).

---

## Relationship to FirmwarePlan

Every TechSpec file references one or more FirmwarePlan documents. The FirmwarePlan is the
authority on behaviour — if there is a conflict between a TechSpec and FirmwarePlan, the
FirmwarePlan wins and the TechSpec must be updated.

FirmwarePlan files live at:
`Firmware/ScratchPad/FirmwarePlan/`

---

## How to Write a TechSpec

Each file follows this structure:

```markdown
# <ClassName> — Technical Specification

**Status:** Not started | In progress | Ready for implementation | Done
**FirmwarePlan ref:** <path to relevant FirmwarePlan file(s) and section>
**Depends on:** <list of other TechSpec files that must be implemented first>

---

## Responsibility
One paragraph — what this class does and what it does NOT do.

## File Layout
Header and source file names, directory location in the repo.

## Public API
Full class declaration with all public methods, constructors, and constants.
Include parameter types, return types, and a one-line description per method.

## Key Data Structures
Any structs, enums, or typedefs owned by this class.

## Implementation Notes
Non-obvious details: timing, ISR constraints, register sequences, library quirks.

## Dependencies
External libraries, other OpenSkyhawk classes, platform requirements.
```

Testing is continuous throughout implementation — not a separate phase or document.
Each class should be exercisable in isolation before integration. Build test sketches
alongside the library as you go.

---

## Timing Standard

Each class that needs timing calls `millis()` directly and maintains its own `uint32_t`
timestamp. There is no shared clock or centralized `now` on STM32Board.

`millis()` on STM32 reads a 32-bit hardware counter — effectively free. Multiple calls
within one loop iteration return values microseconds apart, which is irrelevant at the
timing granularity used here (debounce ~20 ms, blink 250 ms+, heartbeat 500 ms).

This keeps each class fully self-contained. A Switch2Pos has no dependency on STM32Board
just to debounce a pin.

---

## Code Documentation — Docblock Standard

All header files use **Doxygen-style docblocks**. This applies to every public class, method,
enum, and non-obvious constant. Private members use plain `//` comments.

### Format

```cpp
/**
 * @brief One-line summary of what this does.
 *
 * Optional longer description for non-obvious behaviour, constraints, or side effects.
 * Omit if the brief is sufficient.
 *
 * @param name     Description of the parameter.
 * @param other    Description of another parameter.
 * @return         What the function returns, and what values mean.
 * @note           Non-obvious constraint or timing requirement.
 */
```

### Rules

- **Every public method gets a `@brief`** — even if it seems obvious.
- **`@param` for every parameter** — include units where relevant (ms, Hz, steps).
- **`@return` when the return value is non-trivial** — omit for `void`.
- **`@note` for ISR/timing/threading constraints** — e.g. "must not be called from ISR".
- **Enums:** document the enum with a `/** @brief */` block; document each value with `///< inline`.
- **Constants:** one-line `///` or `///<` inline comment is sufficient.

### Example

```cpp
/** @brief CAN bus health states reported to STM32Board via onCanStatus(). */
enum class CanStatus {
    STARTING,   ///< CAN peripheral configured but not yet started
    NORMAL,     ///< Bus active, no errors
    TX_ERROR,   ///< TEC > 0 — transmit errors accumulating
    BUS_OFF,    ///< CAN controller halted — bus-off condition
};

class CANProtocol {
public:
    /**
     * @brief Start the CAN peripheral and enable receive filters.
     *
     * Must be called after STM32Board::begin(). Sets status to NORMAL on success
     * and fires the registered onStatusChange callback.
     */
    static void start();

    /**
     * @brief Register a callback invoked whenever CAN bus status changes.
     * @param cb Function pointer called with the new CanStatus value.
     * @note Callback is called from the CAN RX interrupt — keep it short.
     */
    static void onStatusChange(void (*cb)(CanStatus));
};
```

---

## Status Tracking

Each file has a **Status** field in its header. Valid values:

| Status | Meaning |
|--------|---------|
| `Not started` | Placeholder only — spec not yet written |
| `In progress` | Spec being written |
| `Ready for implementation` | Spec complete — can be handed off for coding |
| `Implementing` | Code being written |
| `Done` | Implemented and tested |

---

## Implementation Order

Dependencies flow downward — implement in this order:

```
HIDControls         (no dependencies — platform-agnostic header; shared by STM32 and RP2040)
A4EC                (no dependencies — generated by running tools/gen_a4ec/gen_a4ec.py)
CANProtocol         (depends on HIDControls — includes HIDControls.h)
STM32Board          (depends on CANProtocol for the CanStatus event type)
  ↑ implement HIDControls first; then A4EC and CANProtocol (either order); then STM32Board
PanelBridge         (depends on CANProtocol, STM32Board, A4EC)
SimGateway          (depends on HIDControls — CTRL_* constants used in sketch declarations)
  ↑ build the bridge/gateway backbone before the large PanelGroup surface
  └── PanelGroup/
        ├── PinRef              (no dependencies)
        ├── PanelGroup          (depends on PinRef, CANProtocol, STM32Board;
        │                         owns MCP23017 integration via external library)
        ├── Inputs/
        │     ├── Switch2Pos              (depends on PinRef, PanelGroup)
        │     ├── Switch3Pos              (depends on PinRef, PanelGroup)
        │     ├── SwitchMultiPos          (depends on PinRef, PanelGroup)
        │     ├── AnalogMultiPos          (depends on PinRef, PanelGroup)
        │     ├── ActionButton            (depends on PinRef, PanelGroup)
        │     ├── RotaryEncoder           (depends on PinRef, PanelGroup)
        │     ├── RotaryAcceleratedEncoder (depends on RotaryEncoder)
        │     ├── RotarySwitch            (depends on RotaryEncoder)
        │     ├── AnalogInput             (depends on PinRef, PanelGroup)
        │     └── AngleSensorInput        (depends on PinRef, PanelGroup)
        └── Outputs/
              ├── LED                     (depends on PinRef, PanelGroup)
              ├── IntegerOutput           (depends on PanelGroup)
              ├── AnalogOutput            (depends on PinRef, PanelGroup)
              ├── SwitecX25Output         (depends on PanelGroup)
              ├── AccelStepperOutput      (depends on PanelGroup)
              └── ServoOutput             (depends on PinRef, PanelGroup)

Note: PanelGroup sketches (per-panel main.cpp files) depend on both the PanelGroup
library and A4EC (for DCSIN_* constants). The PanelGroup library itself does not.
```

---

## Library and Test Project Structure

### Library layout

Every STM32 library lives under `Firmware/Libraries/<LibraryName>/` and must include
a `library.json` so PlatformIO can resolve it as a local dependency:

```
Firmware/Libraries/<LibraryName>/
├── <LibraryName>.h
├── <LibraryName>.cpp
└── library.json
```

Minimal `library.json`:
```json
{
  "name": "<LibraryName>",
  "version": "0.1.0",
  "frameworks": "arduino",
  "platforms": "ststm32"
}
```

### Test project layout

Each library has a companion test project at `Firmware/Tests/<LibraryName>/`. The test
project is a standalone PlatformIO project that references the library as a local dependency.
Each test scenario is a separate `.cpp` file in its own subdirectory under `tests/`:

```
Firmware/Tests/<LibraryName>/
├── platformio.ini
└── tests/
    ├── <scenario_a>/
    │   └── <scenario_a>.cpp
    ├── <scenario_b>/
    │   └── <scenario_b>.cpp
    └── <scenario_c>/
        └── <scenario_c>.cpp
```

### `platformio.ini` template

`src_dir` is a project-level setting — it goes in `[platformio]`, not `[env:]`.
`build_src_filter` patterns are relative to `src_dir`. `[env_base]` is a custom section;
environments inherit from it via `extends = env_base`.

```ini
[platformio]
src_dir = tests

[env_base]
platform = ststm32
board = genericSTM32F103CB
framework = arduino
build_flags = -DNODE_ID=1
lib_extra_dirs = ${PROJECT_DIR}/../../Libraries
lib_deps = <LibraryName>

[env:test_<scenario_a>]
extends = env_base
build_src_filter = -<*> +<<scenario_a>/<scenario_a>.cpp>

[env:test_<scenario_b>]
extends = env_base
build_src_filter = -<*> +<<scenario_b>/<scenario_b>.cpp>
```

`-<*>` drops the default `src/` folder; `+<tests/foo.cpp>` adds only that file as the
entry point. Switch environments from the VSCode status bar or with
`pio run -e test_<scenario> -t upload` on the CLI.

---

## Referencing FirmwarePlan

When writing a TechSpec, always link to the specific FirmwarePlan section that defines the
behaviour. Use this format in the **FirmwarePlan ref** field:

```
FirmwarePlan/05-panelgroup-api.md#switch2pos
FirmwarePlan/02-can-protocol.md#controlpacket-wire-format
```

If the spec contradicts FirmwarePlan, raise it as a conflict — do not silently diverge.
