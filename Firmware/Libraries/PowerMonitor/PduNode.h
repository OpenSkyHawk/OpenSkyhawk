/**
 * @file PduNode.h
 * @brief PDU monitoring node — the whole node in two calls (#216).
 *
 * A PDU sketch is just:
 * @code
 * #include <PduNode.h>
 * void setup() { STM32Board::setDebug(true); PduNode::setup(); }
 * void loop()  { PduNode::loop(); }
 * @endcode
 * with `-DNODE_ID=N` in platformio.ini. What runs:
 *   - **Node status** — HB_n / HEALTH_n / master-loss watchdog via CANProtocol::serviceNodeStatus().
 *     Rail and thermal faults reach HEALTH_n.faultId through the FaultSource registry.
 *   - **Sampling** — the four rail channels every PDU_SAMPLE_US (1 kHz target); the NTC and
 *     Vrefint every 100 ms. Faults are held clear for PDU_BOOT_BLANK_MS after boot.
 *   - **Telemetry** — one TELEM_n frame per second, rotating PDU_RAIL_12V → PDU_RAIL_5V →
 *     PDU_STATUS; a page whose flags change goes out at once (≤ 1 per 100 ms per page);
 *     SYNC_REQ sends all three pages immediately.
 *   - **Calibration** — DiagSerial (115200) commands, one per line:
 *       `CAL SHOW` · `CAL VDDA <mV>` (DMM reading of the 3V3 rail) · `CAL ZERO` (no load
 *       connected) · `CAL CLEAR` · `RAW` (live readings + scan rate).
 *     Stored per board in the reserved flash page (PduCalStore).
 *
 * The status LED follows every other node: CAN / link state plus the master-loss WARNING; rail
 * faults show on the client and the board's D3/D4 rail LEDs, not the MCU LED (FirmwarePlan D14).
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>
#include <STM32Board.h>
#include <CANProtocol.h>
#include "RailMonitor.h"
#include "BoardThermal.h"
#include "PduCalStore.h"

namespace PduNode {

/** @brief Start the board, CAN and calibration, then send READY_n. Call from setup(). */
void setup();

/** @brief Run one iteration: CAN, node status, sampling, telemetry, DiagSerial. Call every loop(). */
void loop();

/** @brief Rail scans completed in the last full second — bring-up check of the 1 kHz target. */
uint16_t scanRateHz();

/** @brief The 12 V rail monitor (bring-up / tests). */
const OpenSkyhawk::RailMonitor& rail12();
/** @brief The 5 V rail monitor. */
const OpenSkyhawk::RailMonitor& rail5();
/** @brief The board thermal monitor. */
const OpenSkyhawk::BoardThermal& thermal();

} // namespace PduNode
