# NODE_ID Registry

Each STM32 PanelGroup node is assigned a unique `NODE_ID` at compile time via
`platformio.ini`:

```ini
build_flags =
    -DNODE_ID=1
```

The NODE_ID is a compile-time constant visible to all library translation units
(CANProtocol, PanelGroup). It must **not** be set in `src/main.cpp` — a `#define`
there is invisible to library code.

## Current assignments

| NODE_ID | Panel Group       | Console | Status   |
|---------|-------------------|---------|----------|
| 0       | PanelBridge       | —       | Reserved |
| 1       | Center_Armament   | Center  | Active   |

## Rules

- **NODE_ID 0** is reserved for PanelBridge (CAN master). It is never transmitted
  on the bus.
- Assignments are **permanent**. Never reuse a NODE_ID even if a panel is retired.
  Mark it Retired in this table instead.
- Assign incrementally. Next available: **2**.
- One NODE_ID per physical STM32 board.
- Claim your NODE_ID in this file in the same PR that creates the new panel sketch
  under `Firmware/Panels/`.

## Valid range

1–63. The CAN frame ID formula is `0x100 + NODE_ID` (heartbeat) and `0x200 + NODE_ID`
(event), so NODE_IDs above 63 would overflow the 11-bit CAN ID field.

See also: `Firmware/NODE_IDS.md` (brief pointer to this page).
