# RotaryAcceleratedEncoder — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#rotaryacceleratedencoder-new`, `FirmwarePlan/04-dcs-bios-integration.md#value-encoding-by-type`
**Depends on:** `RotaryEncoder.md`

---

## Responsibility

Accelerated variant of RotaryEncoder. Tracks inter-detent timing (175ms threshold) and encodes
direction + speed into value: 0=slow CCW, 1=slow CW, 2=fast CCW, 3=fast CW. Includes momentum
tracking to prevent false direction reversals. The 4-value scheme is a CAN transport layer —
PanelBridge maps values to DCS-BIOS argument strings via input map.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
