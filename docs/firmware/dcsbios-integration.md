# DCS-BIOS Integration

Most cockpit controls talk to DCS through **DCS-BIOS**. This page covers how that path is wired
in firmware: the compact command IDs that cross the CAN bus, the generated headers, and the
output addresses that drive LEDs and gauges. PanelBridge is the only tier that runs the
DCS-BIOS library — see [Firmware Tiers](../architecture/firmware-tiers.md).

## The controlId range

A control routes by its `controlId`. DCS-BIOS controls live in **`0x8000`–`0x86FF`**;
PanelBridge looks the ID up in its input map and calls `sendDcsBiosMessage()`. (Values below
`0x8000` are HID — handled by SimGateway. See [HID Controls](hid-controls.md).)

## DCSIN_* — compact command IDs

A CAN frame carries only 8 data bytes — far too little for a DCS-BIOS ASCII command like
`"ARM_MASTER 1\n"`. So inputs send a compact **`DCSIN_*`** constant instead, generated
sequentially from `0x8001`. PanelBridge maps it back to the real command string.

```cpp
// PanelGroup sketch — fire a compact ID, not a string:
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PinRef(PB5));
```

`DCSIN_*` constants are **transport aliases**, not device IDs. Multiple controls may share one
`DCSIN_*` if they trigger the same DCS command — PanelBridge doesn't track which node sent it.

## Output addresses — A_4E_C_*

Outputs (LEDs, gauges) go the other way: DCS-BIOS streams a state word, PanelBridge broadcasts
it, and output objects pick out their value by **address + bitmask** from the generated
headers:

- `A_4E_C_<NAME>` — the 16-bit DCS-BIOS output address
- `A_4E_C_<NAME>_AM` — the bitmask for the relevant bits

```cpp
OpenSkyhawk::LED masterCaution(A_4E_C_MASTER_CAUTION, A_4E_C_MASTER_CAUTION_AM, PinRef(PB0));
```

!!! warning "No `_A` suffix"
    DCS-BIOS's own `Addresses.h` uses an `_A` suffix for these. The generated `A4EC` headers
    deliberately **drop it** and add the `_AM` mask companion. Always use `A_4E_C_*` /
    `A_4E_C_*_AM` — never the old `_A` form.

Input `DCSIN_*` IDs and output `A_4E_C_*` addresses are **separate namespaces** — an output
address is never used as an input `controlId`.

## Generated A4EC headers

The A4EC generator parses the A-4E-C DCS-BIOS JSON and emits committed headers under
`Firmware/Libraries/A4EC/` (currently **150** `DCSIN_*` constants):

| Header | Included by | Contents |
|--------|-------------|----------|
| `A4EC_CmdIds.h` | PanelGroup sketches | `DCSIN_*` command-ID `#define`s |
| `A4EC_InputMap.h` | PanelBridge only | Sorted `DcsBiosInputEntry[]` lookup table |
| `A4EC_OutputIds.h` | Output objects | `A_4E_C_*` addresses + `_AM` masks |

PanelBridge looks up each incoming `DCSIN_*` in `A4EC_InputMap.h` by binary search, then calls
`sendDcsBiosMessage()` with the matched command name and the right argument for the control
type (switch → `"0"`/`"1"`, encoder → `"DEC"`/`"INC"`, multipos → the integer, etc.).

!!! note "The generator is maintainer tooling"
    The generated headers are **committed** — contributors do not run the generator. Refreshing
    them when a new A-4E-C release ships is a maintainer task (`tools/gen_a4ec/`). There is no
    DCS-BIOS version pinning today; refresh is manual.
