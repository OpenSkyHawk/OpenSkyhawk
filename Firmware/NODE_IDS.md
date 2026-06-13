# NODE_ID Registry

NODE_IDs are assigned at compile time via `platformio.ini` `build_flags = -DNODE_ID=N`.

**Assignments are permanent.** Never reuse a NODE_ID, even if a panel is retired — the
old assignment stays in this file marked Retired. Before starting a new panel group,
claim the next available NODE_ID here.

<!-- --8<-- [start:registry] -->
| NODE_ID | Panel Group       | Console | Status |
|---------|-------------------|---------|--------|
| 0       | PanelBridge       | —       | Reserved — CAN master, never transmitted on bus |
| 1       | Center_Armament   | Center  | Active |
<!-- --8<-- [end:registry] -->

## Rules

- NODE_ID 0 is reserved for PanelBridge.
- Assign incrementally: next available is 2.
- One NODE_ID per physical STM32 board.
- Add your assignment here in the PR that creates the new panel sketch.
