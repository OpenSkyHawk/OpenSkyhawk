# PowerMonitor — Technical Specification

**Status:** Done — firmware + tests (hardware bench on the PDU itself pending #202 bring-up)
**FirmwarePlan ref:** `02-can-protocol.md` (Node Telemetry Payload — TELEM_n), `04-dcs-bios-integration.md`
(`_NODE_TELEM`), `08-hardware-firmware-contracts.md` (PDU analog front end & calibration page),
`00-decisions.md` D17 (node status service) + D18 (TELEM_n)
**Depends on:** CANProtocol, STM32Board, NodeStatus

---

## Responsibility

The firmware of the **PDU monitoring node** (#202 hardware, #216 firmware) — one per console,
NODE_IDs 3/4/5. It samples the 12 V and 5 V rails (voltage + current) and the board NTC, turns
them into engineering units with the board's own calibration, raises rail / thermal conditions,
publishes them as `TELEM_n` pages, and feeds its faults into `HEALTH_n` through the FaultSource
registry. Node plumbing (HB / HEALTH / master watchdog) comes from `CANProtocol::serviceNodeStatus()`,
called with `NodeKind::PDU` so every PDU announces its kind in `HEALTH_n` whatever its NODE_ID (#344);
there is **no PanelGroup dependency** (the PDU has no inputs or outputs).

Does **not** own: the TELEM wire contract (`TelemPayload` in CANProtocol, page vocabulary in
NodeStatus), host forwarding (PanelBridge), or the status LED (STM32Board / CANProtocol — rail
faults never touch it, D14).

Reads the analog pins directly (`analogRead`) rather than through `PinRef`: `PinRef` lives in the
PanelGroup library, which this node deliberately does not link. The pins are fixed by the PDU board
and overridable as `PDU_PIN_*`.

---

## File Layout

```
Firmware/Libraries/PowerMonitor/
├── library.json        ← deps: STM32Board, CANProtocol, NodeStatus
├── PduConfig.h         ← pin map, front-end constants, PDU_* threshold macros (defaults)
├── PduMath.h/.cpp      ← pure conversions: VDDA, rail mV, current mA, NTC 0.1 °C, CRC-32
├── RailMonitor.h/.cpp  ← one rail: flags (Latch persistence + hysteresis), window stats, page
├── BoardThermal.h/.cpp ← NTC warn / critical / fault
├── TelemSchedule.h/.cpp← 1 frame/s rotation + rate-limited edge sends
├── PduCalStore.h/.cpp  ← calibration records in the reserved flash page
└── PduNode.h/.cpp      ← the node: setup()/loop(), sampling, telemetry, DiagSerial CAL commands

Firmware/Panels/PDU_Left|PDU_Right|PDU_Center/   ← identical thin sketches, NODE_ID 3/4/5
Firmware/Tests/PowerMonitor/                      ← 5 test envs (below)
```

---

## Public API

```cpp
namespace PduNode {
    void setup();                 // STM32Board::begin, load cal, CAN start, READY_n
    void loop();                  // tick, drain, serviceNodeStatus, scan, telemetry, DiagSerial
    uint16_t scanRateHz();        // rail scans in the last second (target 1000)
    const OpenSkyhawk::RailMonitor&  rail12();
    const OpenSkyhawk::RailMonitor&  rail5();
    const OpenSkyhawk::BoardThermal& thermal();
}

class OpenSkyhawk::RailMonitor : public FaultSource {
    struct Config { TelemPage page; const char* name; uint16_t uvMv, ovMv, ocMa; bool fuseOpenCheck; };
    void sample(uint16_t vMv, int32_t iMa, bool vRailed, uint32_t nowMs, bool armed);
    uint8_t flags() const;                    // PduRailFlag bits
    TelemPayload buildPage(bool resetWindow); // PDU_RAIL_* page; periodic send restarts the window
    uint16_t peakMa() const;  void resetPeak();
    NodeFaultCode faultCode() const override; // FUSE_OPEN > SHORT_CIRCUIT > OVER_V > UNDER_V > SENSOR
};

class OpenSkyhawk::BoardThermal : public FaultSource {
    void sample(int16_t tempDC, bool ntcFault, bool armed);
    uint8_t flags() const;                    // TEMP_WARN | TEMP_CRIT | NTC_FAULT
    int16_t tempDC() const;                   // INT16_MIN = unavailable
    NodeFaultCode faultCode() const override; // OVER_TEMPERATURE > SENSOR_FAULT
};

class OpenSkyhawk::Pdu::TelemSchedule {       // PAGES = 3, TELEM_PERIOD_MS = 1000, EDGE_MIN_MS = 100
    void start(uint32_t nowMs);
    int8_t next(uint32_t nowMs, const uint8_t flags[PAGES], bool& periodic);  // slot or -1
    void sent(uint8_t slot, uint8_t flags, uint32_t nowMs);
};

class OpenSkyhawk::Pdu::CalStore {            // page = PDU_CAL_PAGE_ADDR (0x0800FC00)
    bool load(CalRecord& out) const;          // newest valid record; false → calDefaults()
    bool save(CalRecord& rec);                // append (seq+1); erase first when full
    bool erase();
    uint8_t usedSlots() const;
};
```

---

## Key Data Structures

**`CalRecord`** — 32 bytes, CRC-32 over bytes 0–27, up to 32 per page (append-only, newest valid
wins, erase only when full):

| Off | Size | Field |
|---|---|---|
| 0x00 | u32 | `magic` `'PCAL'` |
| 0x04 | u8 | `version` (1) |
| 0x05 | u8 | `calFlags` (`CAL_VDDA` / `CAL_ZERO` / `CAL_GAIN` / `CAL_NTC`) |
| 0x06 | u16 | `vrefintMv` |
| 0x08 / 0x0A | i16 ×2 | 12 V / 5 V current zero, 0.1 mA |
| 0x0C / 0x0E | u16 ×2 | current gain trims (10000 = ×1) |
| 0x10 / 0x12 | u16 ×2 | voltage gain trims |
| 0x14 | i16 | NTC offset, 0.1 °C |
| 0x16 | u16 | reserved |
| 0x18 | u32 | `seq` |
| 0x1C | u32 | CRC-32 |

**`Pdu::Latch`** — a condition that must hold `setMs` to raise, and its clear condition hold
`clearMs` to drop; hysteresis is expressed through the separate clear condition.

---

## Implementation Notes

- **Sampling.** Rails every `PDU_SAMPLE_US` (1000 µs; measured 1000 scans/s on an F103C8), four
  `analogRead`s per scan; NTC + Vrefint every 100 ms. `analogReadResolution(16)` comes from
  STM32Board, shared with `readDieTempC()` — same thread, no contention.
- **Conversions** (PduMath, integer except the NTC): pin mV = counts × VDDA / 65536; 12 V = pin × 43/10;
  5 V = pin × 23/13; mA = 2 × pin mV − zero, × gain; NTC ratiometric Beta (B 3450, 10 k / 10 k).
  VDDA = 3300 mV until `CAL VDDA` stores a Vrefint, then Vrefint-tracked.
- **Flags** (defaults in `PduConfig.h`, all `-D` overridable): UV 11.40 / 4.75 V, OV 13.2 / 5.5 V,
  OC 4.5 / 1.8 A (90 % of fuse), fuse-open < 1.0 V (12 V only); voltage set 50 ms, OC set 100 ms,
  clear 500 ms, 100 mV hysteresis; NTC warn 60 °C / crit 75 °C, 3 °C hysteresis; NTC fault outside
  2–98 % FS; voltage channel ≥ 99.5 % FS → SENSOR_FAULT. Everything held clear for
  `PDU_BOOT_BLANK_MS` (500 ms) after boot.
- **Telemetry.** `TelemSchedule` drives one `TELEM_n` frame per second, rotating the three pages;
  a flag change sends that page at once (≤ 1 / 100 ms / page) without restarting its window;
  `SYNC_REQ` sends all three. The status page's peaks reset on its periodic send.
- **DiagSerial commands** (115200, one per line, replies always printed): `CAL SHOW`,
  `CAL VDDA <mV>` (3000–3600; refuses a derived Vrefint outside 1100–1300 mV), `CAL ZERO` (256-sample
  average; refuses above 75 mA on 12 V / 120 mA on 5 V), `CAL CLEAR`, `RAW` (live values + scan rate).
- **Flash budget.** `board_upload.maximum_size = 64512` keeps the image out of the calibration page;
  the PDU build is ~31.5 KB.

---

## Test

`Firmware/Tests/PowerMonitor/` — results on DiagSerial, any STM32F103 board:

| Env | Covers |
|---|---|
| `test_pdu_math` | VDDA, rail / current conversions incl. trims and signed offset, NTC curve (25 / 50 / 0 °C), NTC open / short, railed, CRC-32 check value |
| `test_rail_monitor` | boot blanking, UV set 50 ms / clear 500 ms, hysteresis band, OC 100 ms + 90 % clear, fuse-open priority, sensor railed, 5 V has no fuse check, page bytes, window + peak reset |
| `test_board_thermal` | warn / crit thresholds + hysteresis, NTC fault keeps last state, fault codes |
| `test_telem_schedule` | 1 Hz rotation, edge send, 100 ms hold-off, rotation undisturbed, no burst after a stall, all-clear baseline |
| `test_cal_store` | real flash page: save / load, newest wins, corrupt record skipped, wrap-around erase, left erased |

Plus `Tests/PanelBridge` `test_node_telem` (`_NODE_TELEM` forwarding, needs the UART2 adapter) and
`Tests/CANProtocol` `protocol_layout` (`canIdTelem` range, `TelemPayload` size).

---

## Dependencies

| Dependency | Use |
|---|---|
| CANProtocol | `send`, `drain`, `start`, `serviceNodeStatus`, `canIdTelem`, `canIdReady`, `TelemPayload` |
| STM32Board | `begin`, `tick`, `diagSerial`, `onCanStatus`, ADC resolution |
| NodeStatus | `FaultSource`, `NodeFaultCode`, `TelemPage`, `PduRailFlag`, `PduStatusFlag` |
| STM32duino core | `analogRead` (incl. `AVREF`), `micros`, `millis`, HAL flash (`HAL_FLASHEx_Erase`, `HAL_FLASH_Program`) |
