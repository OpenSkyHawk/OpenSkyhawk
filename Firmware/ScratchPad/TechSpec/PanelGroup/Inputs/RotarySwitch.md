# RotarySwitch — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#rotaryswitch-new`
**Depends on:** `RotaryEncoder.md`

---

## Responsibility

Rotary encoder used as N-position absolute switch. Tracks position 0–(N-1); hard stops at
both ends (no wrap). Emits position index as MULTIPOS value. Initialises to 0 at boot;
re-syncs when user turns to either end-stop (matches DCS-BIOS Arduino library behaviour).

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
