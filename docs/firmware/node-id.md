# NODE_ID & CAN Addressing

Every STM32 board on the CAN bus has a **NODE_ID** — a small integer that defines which CAN
frames it owns. It's how PanelBridge tells one PanelGroup node from another, and it's baked in
at compile time. Get it right once and it never changes.

## The scheme

- **Set at compile time** via `build_flags = -DNODE_ID=N` in `platformio.ini` — **not** a
  `#define` in `main.cpp`.
- **Range 0–63.**
  - **0** — PanelBridge (the CAN master). Reserved; its heartbeat `HB_0` is never transmitted.
  - **1–63** — PanelGroup nodes, assigned incrementally, **permanent once assigned**.
- **SimGateway has no NODE_ID** — it's not on the CAN bus.

A node's frame IDs are computed from its NODE_ID with the helper functions — heartbeat
`canIdHb(n)`, events `canIdEvt(n)`, echo `canIdEcho(n)`, ready `canIdReady(n)`. Never hard-code
these. The full frame map is in [CAN Bus Protocol](../architecture/can-bus.md).

!!! note "Why build_flags and not `#define`"
    A `#define NODE_ID` in `main.cpp` is visible only in that translation unit. The CAN
    protocol library and other units need `NODE_ID` to compute frame IDs. `build_flags`
    injects it as a compiler flag visible to **every** translation unit in the project.

## The registry

The authoritative record lives in `Firmware/NODE_IDS.md` at the repo root. Current state:

--8<-- "Firmware/NODE_IDS.md:registry"

## How to claim a new NODE_ID

IDs are permanent — **never reuse one**, even if a panel is retired (mark it Retired instead).
When you start a new panel group:

1. Open `Firmware/NODE_IDS.md` and take the **next available** number (currently **2**).
2. Add a row with your panel group, console, and status.
3. Set `build_flags = -DNODE_ID=N` in your board's `platformio.ini`.
4. Commit the registry change **in the same PR** that creates the panel sketch.

One NODE_ID per physical STM32 board. See [Adding a New Panel Group](../guides/new-panel-group.md)
for the full workflow — claiming the ID is step one.

!!! note "CI enforces this"
    A CI check (`tools/check_node_ids.py`) validates every production panel under
    `Firmware/Panels/` against this registry on each firmware PR: it fails the build if a board's
    NODE_ID is unregistered, duplicated, or out of range. The table above is transcluded directly
    from `Firmware/NODE_IDS.md`, so the docs can't drift from the registry.
