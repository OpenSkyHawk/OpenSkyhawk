# PDU (#202) — parts research / generic framing

Board: `PCB/Base/Power/PDU` — per-console Power Distribution + Monitoring CAN node. Rev1 = instrumentation-first.
Path in chain: **PSU → [Fuse → Shunt → INA180 sense] per rail → console J_BUS_OUT**. GND + CAN continuous.

Rev1 goal = *measure the unknown load*. Bias every choice to accuracy + swappability, not optimization.

---

## A. Reused blocks (canonical — from PanelGroup_Base / hardware-standards, no new decision)

| Block | Part | Package | Source |
|---|---|---|---|
| MCU core | STM32F103C8 (CB only if flash needs it) | LQFP-48 | PanelGroup_Base / variant policy |
| Clock | 8 MHz crystal + 2×caps | — | PanelGroup_Base |
| CAN node | SN65HVD230 + 120 Ω term jumper | SOIC-8 | hw-standards:335 |
| 5V→3.3V | AMS1117-3.3 + in/out caps | SOT-223 | hw-standards:78 |
| Status LED | red/grn on PB14/PB15 | 0805 | Status_LED block |
| SWD header | 4-pin | — | MCU core |
| Bus connector | Molex Mini-Fit Jr 5566-08A2 2×04 (J_BUS_IN + J_BUS_OUT) | THT | hw-standards:239 |

Note: **NO buck on the PDU.** Servo 12V→5/6V buck (AP63205WU) lives on each *panel* board, not here. PDU only injects + monitors + fuses.

---

## B. New parts to select (the actual research)

### B1. Current sense — INA180A2 ×2 (12V rail, 5V rail)  [LOCKED 2026-07-03 — analog/cost path]
- Part: **INA180A2IDBVR** — gain **50 V/V**, SOT-23-5 (0.95mm), ~$0.25 ea → **$0.50/board**. (LCSC A1=C122228 / A3=C122882; confirm A2 C-number at order.)
- Current-only analog Vout → 1 STM32 ADC pin/rail. CM –0.2..+26V (12V high-side OK).
- Gain choice: 50 on 10mΩ → **FS 6.6A** (covers 5A/4A fuses), ~1.5V ADC swing at 3A console = good resolution. (A1/20 wastes range; A3/100 clips at 3.3A.)
- **Voltage is separate** — INA180 does NOT measure voltage (see B5 dividers).
- **Datasheet-verified (SBOS741H):** gain 50 ✓, CM −0.2..+26V ✓, supply 2.7–5.5V ✓, SOT-23-5 ✓, unidirectional ground-ref (high-side OK) ✓, gain err ±1% max, offset drift 1µV/°C.
- **⚠ Offset: ±500µV MAX @VCM=12V** (vs ±150µV @0V) → ±50mA raw floor on 12V rail (> ±10–20mA target at idle). **Mitigation: one-point zero-cal in FW** (read output at I=0, subtract) → removes fixed offset, residual = drift only (~0.1mA/°C). Do at bring-up alongside VDDA cal.
- Chosen over INA219 ($0.98/rail digital, integrated V+I+self-cal): relaxed spec (±10–20 mA, absolute-V not critical → brownout/droop trends enough) → cheaper analog wins. INA282 $4 / INA3221 VQFN-banned both out earlier.

### B1b. VDDA reference — use free internal Vrefint (LM4040 DROPPED)
- ADC ref = VDDA (3.3V rail, AMS1117 ±1–2% + drift) → all readings scale with it.
- Correction = **STM32F103 internal Vrefint** (ADC ch17, ~1.20V bandgap): FW reads it → computes true VDDA → corrects. **0 parts, $0.**
- F103 Vrefint not per-chip trimmed (±~3% absolute) but tracks load/temp drift — enough for ballpark.
- **LM4040 dropped** — ~$0.60 (not $0.15) precision ref; overkill vs Vrefint for ballpark goal. Revisit only if absolute-V accuracy becomes a requirement.

### B2. Shunt resistor ×2 — 10 mΩ  [LOCKED: HoRCG27284R010F1T, C53115028]
- **Milliohm HoRCG27284R010F1T** — 2728, **4W**, ±1%, **±25 ppm/°C**, LCSC **C53115028**, $0.22@5.
- Chosen over CSRF2512FT10L0 (C346481, 2512 2W ±100ppm): **±25ppm = 4× better tempco** (0.1% vs 0.4% current-gain drift /40°C — the part we can't zero-cal out), cheaper, bigger 2728 = better Kelvin-pad geometry + thermal. 4W overkill (max dissipation ~0.25W @5A) but free.
- Kelvin sense: thin dedicated traces off the pad corners to INA180 IN+/IN− — never share high-current copper. 2728 gives room for clean Kelvin pads.
- On INA180 gain 50: FS = 3.3V/(50×10mΩ) = 6.6A (covers 5A fuse); ~1.6mA/LSB @12-bit.

### B3. Temperature — 1× NTC (between pours) + STM32 internal  [LOCKED]
- Ticket said **TMP117** → WSON/DSBGA only → BANNED. Dropped.
- **1× NTC 10k 1206 — APV ANTC3216-103F3450FB, LCSC C52155460** (~$0.12@5, $0.06 reel). 10kΩ @25°C, **B=3450K**, **R-tol ±1%**, 150mW. + bias R → STM32 ADC.
  - Upgraded from ±5% J-grade (C52155463) to **±1% F-grade** for **±0.3–0.5°C** temp accuracy (user target ±0.5–1°C), no cal needed. Same APV ANTC3216 family.
  - FW Steinhart/LUT **must use B=3450K**.
  - Alt (equivalent): KUU KNTC1206/10KF3450, C42377641.
  - Placed **between the 12V and 5V pours** (central hot-zone). PDU is low-power (~2–3A/console) → one central sensor enough; die temp ≠ board temp so external still needed.
- **STM32 internal temp sensor** (ADC ch16) — FREE, no part/pin. Enable in FW for node die-temp reference.
  - Uncalibrated on F103 (±few °C abs, die not ambient, self-heat) → relative/overheat flag only.
  - → cross-board convention filed as separate task (all nodes report internal temp over CAN).
- ADC: +1 external (NTC); internal temp is on-chip channel (no external pin).

### B4. Fuses — MINI blade + PCB holder ×2  [LOCKED]
- **Holder: XFCN XF-508P-B-B, LCSC C19727305** (MINI/小号 variant, DIP-4 THT, 15A/500V, contact R ≤5mΩ, −40…125°C). Field-swappable. (Note: …304=ATO/大号, …305=MINI/小号 — key: field2 A=大 B=小.)
- **Fuse: MINI blade** — field-swappable.
- Available MINI sizes: **2 / 5 / 10 A**. Rev1 values (load < fuse < 10A/pin link):
  - **12V → 5A** (load ~2–3A: LEDs + servo-buck)
  - **5V → 2A** (load ~1A: logic + OLED; servos offloaded to 12V. NOT 4A — that was a whole-bus blanket; per-console 5V is ~1A → 2A is right + standard.)
  - 10A blade = spare / for a console that graduates to a dedicated high-draw PDU.
- Fuse IS the per-connector protection (single-rail PSU has no per-lead OCP). Fuse-open ⇒ V-sense both sides (B5).
- Data-driven final sizing (Rev1 instrumentation) → blade swaps freely.

### B5. Fuse-open + rail voltage sense  [PASSIVES — divider → STM32 ADC]
- 4 dividers = both sides × 2 fuses. 12V→**33k/10k** (2.79V@12V, headroom ~14V); 5V→**10k/16k** (~3.1V).
- **0.1% 25ppm Rs** (~2c ea — cheap enough to just use). Uncalibrated ±24mV on 12V, under 100mV target → voltage accuracy stands without the per-board cal (cal still helps VDDA/current). Best tempco of the stack.
- Values: 12V 33k/10k, 5V 10k/13k (0603). Resolution 3.5mV/LSB — far finer than needed.
- RC 100nF at tap; bottom-R ≤10k (clean ADC S/H); series top-R also limits pin fault current.
- (Grade history: tolerance is cal-removable so 1% would also work; 0.1% chosen since ~2c makes it moot + no cal dependence for V.)
- Real drift source = **STM32 ADC ref = VDDA (3V3 rail)**, not the rail itself → LM4040 (B1b) corrects it.
- ADC budget: 2× INA180 (I) + 4 dividers (V) + 1 NTC = **7 external ch**. F103C8 has 10 external. Vrefint + internal-temp are on-chip channels (no external pin). Fits.

### B6. PSU-side power input connector  [LOCKED: Mini-Fit Jr 6-pin 2×3]
- **Molex Mini-Fit Jr 2×3 (6-pin)** (house standard, ~9–10 A/pin @18AWG, keyed crimp).
- Pinout: **2×+12V / 1×+5V / 3×GND** → 12V 2.5A/pin, 5V 2A, GND (7A return) 2.3A/pin — comfortable margin. Matches old ticket spec.
- Rationale: GND carries both rails' return (~7A) → spread over 3 pins. (4-pin works but GND 3.5A/pin.)
- Y-harness: PSU → PDU input (this 6-pin); PDU → J_BUS_OUT (power) + CAN from prev node's J_BUS_OUT.

### B7. Bulk capacitance at injection
- Electrolytic/poly bulk on 12V + 5V post-fuse (inrush + local stiffening). Values TBD by load.

### B8. Soft-power / PS_ON#  [LOCKED: DROPPED — not on these cards]
- **No PS_ON# / +5VSB / AC-present on the PDU.** PSU turn-on handled elsewhere (jumper PS_ON#→GND, always-on).
- Removes: +5VSB rail, PS_ON# MOSFET, AC-present divider (+1 ADC). Simplifies Rev1. Add later only via respin if wanted.

---

### B9. Support passives — add at schematic (gap-check 2026-07-03)

**Functional (must):**
- **SN65HVD230 Rs (slope-control)** — pin 8 → GND (high-speed) or ~10k to GND (slew-limited). CAN misbehaves if unset.
- **AMS1117 Cout ≥22µF** (ceramic/tantalum) — stability.
- **STM32 decoupling** — 100nF per VDD pair (×4) + 4.7µF bulk; BOOT0 10k pulldown; NRST 100nF.

**Sensing quality (ADC/instrumentation board):**
- **VDDA filter — ferrite bead 600Ω@100MHz + 1µF + 100nF.** Bead 3V3→VDDA (series), caps VDDA→GND. VDDA = ADC ref (VREF+ tied on LQFP48) → sets noise floor on every reading. ST-recommended.
- **INA180 output RC ×2** — series ~100Ω + 1–10nF to GND. Averages PWM/servo switching before ADC (INA180 = 350kHz, follows ripple otherwise).

**Protection — input-rail TVS = FLEET STANDARD (task_de8956f7); CAN TVS DROPPED:**
- **12V input TVS → SMBJ12A** (SMB/DO-214AA) to GND.
- **5V input TVS → SMBJ5.0A** to GND.
- **CAN bus TVS → DROPPED.** SN65HVD230 has built-in **±16kV HBM** bus ESD = covers the real indoor threat (handling ESD). External PESD1CAN only adds industrial surge/EFT (long cables, switching) → not seen in an indoor cockpit. (DNP footprint optional if ever wanted.)
- Rationale: input TVS cheap (~$0.10 ea), guards rails against PSU power-on overshoot/hot-plug (transceiver ESD does nothing for rails). Fleet standard = **input-rail TVS only** (bake into #201 Power block + hardware-standards; roll into PanelGroup_Base pending rev + Gateway_Bridge next rev). PDU includes from start.

**Bring-up (optional, fits Rev1 instrumentation):**
- Test points: VDDA, each shunt (Kelvin), 12V/5V rails, GND — eases zero-cal + VDDA cal probing.

**Explicitly NOT needed:** I²C pull-ups — PDU has no I²C bus (INA180/NTC analog, temp internal).

## C. Decisions
- [x] **Current** → INA180A2 ×2 (SOT-23, gain 50). Analog → ADC. [LOCKED 2026-07-03]
- [x] **Voltage** → dividers → ADC (B5); VDDA corrected via free internal Vrefint (LM4040 dropped). [LOCKED — cost path over INA219]
- [x] **Temp** → 1× NTC 10k 1206 (between 12V/5V pours) + STM32 internal temp (free). [LOCKED]
- [x] **B4 fuse** → MINI blade + Littelfuse 153-PC PCB holder (swappable). [LOCKED]
- [x] **B6 PSU input connector** → Molex Mini-Fit Jr **6-pin 2×3** (2×12V/1×5V/3×GND). [LOCKED]
- [x] **Datasheets verified** → INA180A2 (SBOS741H) ✓ +zero-cal note · shunt CSRF2512 ✓ · NTC C52155460 ✓. Pending: 153-PC holder + Mini-Fit exact PN at BOM.
- [x] **B8 soft-power PS_ON#** → DROPPED (not on these cards; PSU always-on jumper). [LOCKED]
- [x] **STM32 variant** → STM32F103C8 (light telemetry FW fits 64KB). [LOCKED]
- Sourcing: **LCSC** (order source) — carry C-numbers on all parts.

## ALL SENSING + POWER-PATH DECISIONS LOCKED 2026-07-03 → ready to scaffold PCB/Base/Power.

## D. Not on this board (confirm exclusions)
- Servo buck (AP63205) → panel boards. 3V3 gen local per board (AMS1117). No central 3V3 monitor.
