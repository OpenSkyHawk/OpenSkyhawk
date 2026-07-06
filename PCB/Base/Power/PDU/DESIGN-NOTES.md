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

### B2. Shunt resistor ×2 — 10 mΩ 2512  [FINAL: CSRF2512FT10L0, C346481, 2-terminal]
- **Stackpole CSRF2512FT10L0** — **2512**, 2W, ±1%, **±100 ppm/°C**, **2-terminal**, LCSC **C346481**. Footprint = STOCK **`Resistor_SMD:R_2512_6332Metric`**. Symbol = 2-pin `Device:R` (value 10m).
- **Reverted from HoRCG27284 (2728, ±25ppm, C53115028)** during layout: the 2728 had NO stock KiCad footprint + no working EasyEDA model (easyeda2kicad API failed) → would need hand-draw. CSRF2512 = 2512 with a stock footprint, cheap, and ±100ppm is acceptable (±12mA/40°C at 3A, inside ballpark). Traded ±25ppm (nice-to-have) for a stock footprint.
- **2-terminal → Kelvin via LAYOUT:** route INA180 IN+/IN− to the **inner corners of the two pads**, off the fat current copper (poor-man's Kelvin — adequate for INA180 ±1% class). Current in/out on the outer pad ends.
- Wiring: 2-pin R in the rail (`+xV_FUSED → R → +xV`); INA IN+ on fused/high side, IN− on delivered/low side (same 2 nets/terminals).
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

### B5. Rail voltage sense  [PASSIVES — divider → STM32 ADC]  [LOAD-SIDE ONLY, 2 dividers]
- **2 dividers = delivered +12V, +5V only** (user 2026-07-03: "we only care about 12 and 5"). NO source-side / pre-fuse taps.
- **Fuse-open = inferred from rail-low**, NOT a pre/post comparison. 12V fuse blow → `12V_READ_ADC`≈0 (board alive on 5V → flags it). 5V fuse blow → board loses power → drops off CAN (silence = the alert). Explicit fuse-open dropped.
- Values: 12V→**33k/10k** (2.79V@12V, ~3.0V@13V); 5V→**10k/13k** (2.83V@5V, ~3.1V@5.5V). **0.1% 25ppm 0603** (~2c). Uncalibrated ±24mV on 12V, under 100mV target.
- Per divider: Ra top, Rb bottom, **Rs 1k** series to ADC (fault-limit + anti-alias w/ Cf), **Cf 100nF** AT the ADC pin. Resolution 3.5mV/LSB.
- Nets: `12V_READ_ADC`, `5V_READ_ADC` → MCU sheet ADC pins (hier/global labels). Real drift source = VDDA (3V3 rail) → corrected via internal Vrefint (B1b).
- BOM: 33k ×1, 10k ×2, 13k ×1 (0.1%) + 1k ×2 + 100nF ×2.
- ADC budget: 2× INA180 (I) + 2 dividers (V) + 1 NTC = **5 external ch**. F103C8 has 10. Ample.

### B6. PDU power-input connector  [LOCKED: Mini-Fit Jr 4-pin 2×2 (J_PSU_IN)]
- **Molex Mini-Fit Jr 2×2 (4-circuit), vertical PCB header** — KiCad footprint **`Connector_Molex:Molex_Mini-Fit_Jr_5566-04A2_2x02_P4.20mm_Vertical`** (in STOCK lib — no custom footprint). Same **5566 family** as J_BUS (5566-08A2) → shared crimp terminals/tooling.
- Pinout **as fabricated (J1 pads, authoritative):** **pin1 +12V · pin2 +5V · pin3 GND · pin4 GND**. ~9A/pin → 12V ~3A / 5V ~1A / GND split 2 pins ≪ 9A. Ample. ⚠ **Harness MUST crimp to this board pinout** — earlier "12V/5V non-adjacent, GND-between" intent was NOT realized on the routed board (12V/5V land adjacent p1/p2); boards fabbed → this is the truth, don't build to any older note.
- **House-connector = board-to-board feed** (NOT direct PSU). PDU is fed from a future **power-entry/bridging board** (24-pin ATX in → per-console Mini-Fit feed out, ~216W, 2L/2oz — deferred, task_ab680ebd). ATX/PSU-brand specifics confined to that one entry board; the PDU is now a pure house-connector board.
- **INTERIM (until entry board):** hand-built harness — PSU Molex/SATA device end → crimp → Mini-Fit Jr → PDU. Match pinout (1=12V/2=3=GND/4=5V), 18–20 AWG so it's reusable as the entry-board feed later. Board unchanged either way.
- **History:** Mini-Fit Jr 6-pin → Molex-4 peripheral (direct-PSU idea) → **back to Mini-Fit Jr 4-pin** once the entry-board architecture was adopted (PDU feeds from it, so house connector is right). PCIe 6-pin ruled out (12V-only).
- ⚠ PSU identity unconfirmed (docs say GOLDEN FIELD NX650; a Thermaltake Toughpower GF1 diagram surfaced) — reconcile at entry-board design; doesn't affect the PDU (house connector now).

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
- **5V input TVS → SMBJ6.0A** (SMB/DO-214AA) to GND. *(Changed from SMBJ5.0A 2026-07-05: 5.0A standoff sits at nominal 5.0V and leaks ≤800µA near the +5% rail corner; 6.0A standoff clears 5.25V → leakage ~0. Identical footprint, drop-in — only value/MPN/LCSC change. Clamp 9.2→10.3V, still < DRV8833 VM 11.8V.)* 12V stays SMBJ12A (leaks only 5µA; 13A would eat cap-clamp margin for no gain).
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
- [x] **B6 power-input connector** → **Mini-Fit Jr 4-pin 2×2** (5566-04A2, stock KiCad footprint), 12V/GND/GND/5V. Board-to-board feed from future power-entry board (task_ab680ebd). [LOCKED]
- [x] **Datasheets verified** → INA180A2 (SBOS741H) ✓ +zero-cal note · shunt CSRF2512 ✓ · NTC C52155460 ✓. Pending: 153-PC holder + Mini-Fit exact PN at BOM.
- [x] **B8 soft-power PS_ON#** → DROPPED (not on these cards; PSU always-on jumper). [LOCKED]
- [x] **STM32 variant** → STM32F103C8 (light telemetry FW fits 64KB). [LOCKED]
- Sourcing: **LCSC** (order source) — carry C-numbers on all parts.

## ALL SENSING + POWER-PATH DECISIONS LOCKED 2026-07-03 → ready to scaffold PCB/Base/Power.

---

## SCHEMATIC FRONT-END DRAWN 2026-07-03 (`PDU.kicad_sch`, Rev 1.0)

Blocks drawn (PDU-specific; standard STM32 block imported separately):
- **Input/fuse/TVS** (refs **per fabricated board**) — **J1** J_PSU_IN (Mini-Fit Jr 4-pin) → per rail: TVS shunt-to-GND (**D1 SMBJ12A on 12V, D2 SMBJ6.0A on 5V**, cathode→rail/anode→GND, pre-fuse) → fuse (**F1 5A on 12V, F2 2A on 5V**) → `+12V_FUSED`/`+5V_FUSED`. (Matches BOM table above: D1=C42368008 12V clamp, D2=C140903 5V clamp.) **D2 schematic symbol still shows SMBJ5.0A → swap to SMBJ6.0A on the next KiCad touch (GUI: same DO-214AA footprint, change value/MPN/LCSC only — no layout change).**
- **Voltage sense** (load-side only): `+12V`→33k/10k→1k→`12V_READ_ADC`+100nF; `+5V`→10k/13k→1k→`5V_READ_ADC`+100nF.
- **Current sense**: R1/R2 shunt (10mΩ) in rail `+xV_FUSED`→`+xV`; U1/U2 INA180A2 (IN+=fused/high, IN−=delivered/low), VS=+3V3+100nF, OUT→1k→`I_12V_ADC`/`I_5V_ADC`+100nF.
- **Temp**: +3V3→10k→[NTC TH1→GND]→1k→`NTC_ADC`+100nF.
- **STM32 (U3)**: VDDA filter = **FB1 ferrite bead** (`+3V3→VDDA`) + **C8 100nF ∥ C9 1µF** to VSSA; VSS/VSSA→GND. Symbol shows CBTx — set value to **C8** at BOM (same footprint).
- **Power indicators**: D3 (12V_ST) + R13 4.7k off `+12V`; D4 (5V_ST) + R14 1.5k off `+5V`; anode→rail, cathode→R→GND (~2mA).

**Final ADC map** (5 external ch, 5/10 used):
`PA0`=12V_READ · `PA1`=5V_READ · `PA2`=I_12V · `PA3`=I_5V · `PA6`=NTC. + internal Vrefint (IN17) + internal temp (IN16) in FW.

**CAN-node reuse (firmware, verified in code):**
- **STM32Board** class = status LED (PB14 red/PB15 green, state machine: BOOTING/NORMAL/CONNECTED/CAN_ERROR/BUS_OFF/WARNING) + DiagSerial (USART1 PA9/PA10) + CAN peripheral config (SN65HVD230 PA11/PA12 @500k). Does NOT start CAN.
- **CANProtocol** class = actual bus ops (start/filter/send/status). Full node = STM32Board + CANProtocol.
- **Base status LEDs (PB14/PB15) = node health** (imported). PDU **power indicators (D3/D4) = rail health, MCU-independent** (light even if MCU dead).
- **Pins RESERVED by STM32Board** (don't reuse for ADC): PB14/PB15 (LED), PA9/PA10 (UART), PA11/PA12 (CAN). Our ADC (PA0-3,PA6) clears them ✓.
- PDU FW to add: **NODE_ID 1-63** (0=PanelBridge), telemetry frames (rail V/I/temp/fault bitmap), setWarning() on fault, setLinkActive() on data.

**SCHEMATIC COMPLETE + ERC-CLEAN 2026-07-03.** *(Historical snapshot — some designators below are pre-final draft refs. **Authoritative refs = the fabricated board / BOM table:** J1=J_PSU_IN, J2=J_BUS_OUT, J3=J_BUS_IN(CAN), J4=J_SWD, J5=J_CAN_TERM, J6=J_DIAG.)* Standard STM32 block imported (VDD decoupling 5×100nF + 8MHz xtal Y1/22pF + BOOT0 10k-pulldown + NRST reset SW1/10k/100nF + SWD J7 + SN65HVD230 U5 [Rs pin8→GND high-speed, pin2→GND, Vref pin5 NC] + AMS1117 U4 [10µF in/22µF out] + status LEDs + diag serial J1 + mounting holes H1-4). CAN connectors: **J3 CAN-in (2×2 CANH/CANL/GND)** + **J9 J_BUS_OUT (8-pin power+CAN)** + J5 J_CAN_TERM (120Ω R13, populate if end-node) = split-source injection. PWR_FLAG on +12V/+5V/+12V_IN/+5V_IN/+3V3/GND; 10µF bulk C21/C22 per rail (injection; downstream panels self-decouple). Decoupling stack: 22µF reg-out + 100nF/VDD-pin + bead-VDDA + 10µF/rail.
**Remaining:** assign footprints (**verify shunt C53115028 = 2 vs 4 terminal** for R1/R2 symbol) · DRC after layout (B3).

---

## BOM — LCSC part numbers  [CLOSED 2026-07-04, all verified]

> **InvenTree = source of truth for LCSC on reused parts** (http://192.168.85.85). On conflict the InvenTree SupplierPart SKU wins. AMS1117 corrected C6186→**C347222** per InvenTree pk13. Every new-part LCSC below datasheet-verified this session.

**Active / new-sourced:**
| Ref | Function | MPN | Pkg | LCSC | Qty |
|---|---|---|---|---|---|
| U1,U2 | current-sense amp gain 50 | INA180A2IDBVR | SOT-23-5 | **C192764** | 2 |
| R1,R2 | shunt 10mΩ 2W ±100ppm 2-term | CSRF2512FT10L0 | 2512 | **C346481** | 2 |
| TH1 | NTC 10k B3450 ±1% | ANTC3216-103F3450FB | 1206 | **C52155460** | 1 |
| F1,F2 | MINI-blade fuse holder | XF-508P-B-B | DIP-4 THT | **C19727305** | 2 |
| D1 | TVS 12V rail clamp | SMBJ12A | SMB/DO-214AA | **C42368008** | 1 |
| D2 | TVS 5V rail clamp | SMBJ6.0A | SMB/DO-214AA | **C140903** | 1 |
| FB1 | VDDA ferrite bead 600Ω@100MHz | TDK MPZ2012S601AT000 | 0805 | **C21519** | 1 |
| R4 | 12V divider top 0.1% 25ppm | Viking ARG05BTC3302 | 0805 | **C2828767** | 1 |
| R7 | 5V divider bottom 0.1% 25ppm | YAGEO RT0805BRD0713KL | 0805 | **C865184** | 1 |
| R6,R20 | divider 10k 0.1% 25ppm | Viking ARG05BTC1002 | 0805 | **C406725** | 2 |
| R8-12,R15-17 | 1k series/anti-alias (0.1% 25ppm) | YAGEO RT0805BRD071KL | 0805 | **C110774** | 8 |
| R14 | 1.5k 5V-status LED limit 1% | CHANGLONG CL0805FN1K5PS | 0805 | **C49254235** | 1 |
| J1,J3 | Mini-Fit-Jr 5566 4-pin (chxunda clone OK) | XD-5566-2×2A | 2×2 4.2mm | **C20608114** | 2 |
| J2 | Mini-Fit-Jr 5566 8-pin (chxunda clone OK) | XD-5566-2×4A | 2×4 4.2mm | **C20608116** | 1 |
| D3,D4,D5,D7 | status/indicator LED green | (InvenTree pk27) | 0805 | **C19171393** | 4 |
| D6 | indicator LED red | (InvenTree pk28) | 0805 | **C19171391** | 1 |

**Reused — InvenTree SKUs (authoritative):**
| Ref | Part | LCSC |
|---|---|---|
| U5 | STM32F103C8T6 | C8734 |
| U4 | SN65HVD230DR | C12084 |
| U3 | AMS1117-3.3 | C347222 |
| Y1 | 8MHz crystal | C20617997 |
| SW1 | 6mm tactile RESET | C2939600 |
| J6 | JST-XH B3B 3-pin | C51940188 |
| R3,R5,R_BOOT1 | 10K 1% pulldown (NRST/BOOT0) | C844937 |
| R13 | 4.7K 1% | C844941 |
| R18 | 33 1% | C2090807 |
| R19 | 120 1% CAN term | C844816 |
| C1-7,C12-18,C20 | 100nF | C1711 |
| C8,C9 | 10µF 0805 | C669099 |
| C10 | 10µF elec 4×5.4 | C3343 |
| C11 | 22µF elec 4×5.4 | C72502 |
| C19 | 1µF | C28323 |
| C21,C22 | 22pF C0G | C1804 |

**Off-board consumable (not PCBA):** MINI blade fuses **5A** (12V) + **2A** (5V) — hand-inserted into F1/F2 holders. Order separately or use on-hand automotive MINI (5A tan / 2A grey).

**On hand (user-supplied, no order):** J4 1×05 + J5 1×02 pin headers.

**Divider grades matched:** 33K/13K/10K all thin-film 0.1% 25ppm (C2828767 / C865184 / C406725) — 5V + 12V divider ratios temp-honest; FW cal handles offset, tempco within <100mV target. (Earlier 13K C2991327 = 100ppm rejected; 1K C54921104 was mislabeled 13k, rejected.)

---

## FW CALIBRATION architecture (carry-forward for PDU firmware)

**Per-BOARD, not per-revision** — cal captures each unit's own AMS1117 VDDA, INA180 offsets, shunt/R tolerances. **Same firmware binary on every unit**; cal = **DATA** in a reserved flash page (F103 has no EEPROM → emulate EEPROM in a dedicated flash page). **Memory-map the cal page outside the code region so a FW update that doesn't erase it preserves cal.** One image ships to all units; each self-cals once + stores its constants.

**REQUIRED cals** (needed to meet spec; both FW-internal, no probes):
1. **VDDA cal** — ADC ref = VDDA (AMS1117 ±1–2%). Read internal **Vrefint** → compute true VDDA → correct all readings. (Better absolute: DMM-measure the 3.3V rail once, pass as param.) Uncal = **±1.5–2% scale on ALL readings** → 12V ±180–240mV (>100mV target), 3A ±45–60mA (>20mA target).
2. **INA180 zero-offset cal** — INA offset ±500µV@12V → **±15–50mA idle offset floor**. Read INA output at **no load**, store, subtract. This is the mission-critical one (Rev1 = measure idle current).

**OPTIONAL cals** (only for tighter than ballpark): current gain (known bench current + meter) · voltage (DMM per rail) · NTC one-point (thermometer). 0.1% dividers + the 2 required cals already hit ±10–20mA / 100mV / ±0.5°C.

**TRIGGER:** primary = **CAN command** (from SkyHawkClient / bring-up tool → node runs cal, ACKs result over CAN); fallback = **DiagSerial `CAL`** on J6 UART (bench); safety-net = **auto-if-cal-page-blank** on first boot.

**GUARD (critical):** INA zero-cal MUST verify **~zero load current first** (read raw current; if loaded, **refuse + report** — else it calibrates a real current out as "offset"). VDDA just reads Vrefint (no condition).

**FLOW:** trigger → verify no-load → sample → compute → write cal page + valid/magic flag → report success/fail (CAN + serial).

**EQUIPMENT:** required = board power + **ST-Link** (already needed to flash) + **no load connected**; optional DMM for better VDDA. Optional cals add DMM + bench electronic load. At ~3 boards = 3 quick runs; scales to end-of-line for production (automate over CAN).

## D. Not on this board (confirm exclusions)
- Servo buck (AP63205) → panel boards. 3V3 gen local per board (AMS1117). No central 3V3 monitor.
