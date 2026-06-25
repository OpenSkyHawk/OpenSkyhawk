# Reference Documents

Authoritative third-party standards, specifications, and source documents the OpenSkyhawk build relies on. These are **external references** (not OpenSkyhawk-authored); the project standards *derived* from them live under `docs/_source/` and `docs/hardware/`.

---

## [MIL-F-25173A](MIL-F-25173A.pdf) — *Fastener, Control Panel, Aircraft Equipment*

USAF / Navy Bureau of Aeronautics (Aeronautical Standards Group), 16 Feb 1956 — supersedes MIL-F-25173(USAF), 1955.

**What it represents:** the dimensional + performance standard for the quarter-turn **stud + receptacle-strip ("Dzus rail") fastener** that mounts aircraft equipment control panels. It is the basis for the OpenSkyhawk **Dzus rail panel-mounting standard** (panel width, screw spacing, hole grid).

**Key dimensions** (Figures 1–2):

| Element | Spec |
|---|---|
| Receptacle-strip hole grid | **3⁄8″ (9.525 mm)** pitch (±.005″/ft cumulative) · holes drilled .242″ · strip ~.637″ wide |
| Receptacle strip (the rail) | Dzus P/N **`PR 3½-L`** (or equal), cut to length · .051″ music-wire spring (QQ-W-470) · aluminum (QQ-A-325, T6) · cadmium plated (QQ-P-416) |
| Stud head | Ø **.375″ (9.53 mm)** · .625″ dome radius · .050″ screwdriver slot |
| Stud body / shoulder | body Ø **.257″ (6.53 mm)** · bearing shoulder Ø .315″ (8.0 mm) |
| Panel | **1⁄16″ (1.59 mm)** thick · quarter-turn lock (85–135°), positive stop |
| Stud detail design | left to the fastener manufacturer within the Figure-2 envelope — **no single mandated stud part number** |

**Applied in:** the OpenSkyhawk Dzus rail panel-mounting standard — panel width 146.05 mm (5¾″), mounting-screw centers 136.5 mm (5⅜″), 3⁄8″ vertical pitch — see [`mechanical-standards.md`](../hardware/mechanical-standards.md).
