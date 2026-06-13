# Design Decisions

Empirically-derived findings and architectural decisions made during development.
Each entry records the observation, the test context, and the resulting design rule.

---

## CAN Bus Timing — SJW Must Be 4TQ on Blue Pill Clones

**Date:** 2026-05-20 | **Source:** Prototype Experiment B Step 2 — two-node CAN bus bench test

### Finding

STM32F103CBT6 Blue Pill clones running at 72 MHz (8 MHz crystal + PLL) require
**SJW = 4TQ** for reliable CAN communication at 500 kbps. Using SJW = 1TQ produced
intermittent CRC errors due to crystal frequency tolerance mismatch between boards.

### Test context

Two-node bus: master + one sub-node, daisy-chained over ~30 cm of 24 AWG twisted pair
with 120 Ω termination at each physical endpoint.

**Pass result (SJW = 4TQ, 2026-05-20):**

| Metric | Value |
|--------|-------|
| Soak duration | 21 minutes |
| Packets injected | 1257 @ 1 pkt/sec |
| Average RTT | 1 ms |
| Min / Max RTT | 1 ms / 1 ms |
| Packets lost | 0 |
| TEC | 0 |
| REC | 0 |
| Bus-off events | 0 |

**Failure mode (SJW = 1TQ):** Intermittent CRC errors. TEC climbed on both master and
sub-node within minutes of starting injection. Errors cleared immediately when SJW was
raised to 4TQ — no other change made.

### Design rule

All production `platformio.ini` files must set the CAN bit timing with SJW = 4TQ.
The `CANProtocol` library initialises CAN with this setting. Do not override it.

The internal RC oscillator is not acceptable for CAN timing — all MCU boards require an
**external 8 MHz crystal**. The crystal tolerance is the source of inter-board skew that
SJW = 4TQ absorbs.

---

## External Crystal Required for CAN

**Related to:** SJW = 4TQ finding above

The STM32F103CBT6 internal RC oscillator (HSI, 8 MHz ±1%) is not accurate enough for
500 kbps CAN timing. Crystal tolerance mismatch between boards is the root cause of the
SJW sensitivity described above.

**Design rule:** Every MCU board schematic must include an external 8 MHz crystal.
`platformio.ini` must set `-DHSE_VALUE=8000000` in `build_flags` to activate the HSE clock
source at firmware init.
