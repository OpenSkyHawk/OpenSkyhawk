# AnalogInput — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#analoginput-new`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Continuous analog input normalised to 16-bit (0–65535). Supports STM32 ADC (×16) and
ADS1115 (×2). Configurable [minRaw, maxRaw] clamp range. Dead-band 32 counts. Polls every
8ms. Routes via HID (CTRL_*) or DCS-BIOS (DCSIN_*) depending on controlId.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
