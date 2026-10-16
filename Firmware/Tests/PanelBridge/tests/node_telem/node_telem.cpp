// PanelBridge — node_telem test (#216)
//
// Purpose: verify TELEM_n forwarding. PanelBridge passes every TELEM_n CAN frame (0x181–0x1BF)
// to the host unread, as `_NODE_TELEM <nodeId><8 frame bytes>`; it drops a page-0 or short
// frame, and a TELEM frame never marks a node alive (HB_n owns liveness). Frames are injected
// with the PANELBRIDGE_TEST hook — no CAN bus or second board required.
//
// Hardware: single STM32F103CB board.
//   - Serial (UART2 PA2/PA3) → USB-UART adapter → PC terminal @ 250000 baud
//   - DiagSerial (USART1 PA9/PA10) @ 115200 baud (phase log)
//
// Expected on USB-UART (250000), each cycle:
//   _NODE_TELEM 030100E02E182EBC02   ← node 3 PDU_RAIL_12V: 12000 mV avg, 11800 min, 700 mA
//   _NODE_TELEM 050303FA0010273412   ← node 5 PDU_STATUS: flags 03, 25.0 °C, peaks 10000 / 4660 mA
//   (nothing for the page-0 frame, the 4-byte frame, or canIdTelem(0))
//   _NODE_STATUS 0001…03              ← roster request: the bridge's own node-0 row (#344; kind
//                                       03, or 00 before its first HEALTH_0)
//   _NODE_STATUS_END 1                ← node 0 only: TELEM made no CAN node alive
//
// hex = nodeId(2) then the 8 frame bytes in wire order (multi-byte page fields little-endian).

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>
#include <CANProtocol.h>

static uint32_t _lastMs = 0;

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: node_telem (#216) ==="));
    STM32Board::setDebug(true);
    PanelBridge::setup();   // starts Serial @ 250000 + CAN
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] watch USB-UART (250000) for _NODE_TELEM lines"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();

    const uint32_t now = millis();
    if (now - _lastMs < 3000) return;
    _lastMs = now;
    auto& d = STM32Board::diagSerial();

    const uint8_t rail12[8] = {0x01, 0x00, 0xE0, 0x2E, 0x18, 0x2E, 0xBC, 0x02};
    const uint8_t status[8] = {0x03, 0x03, 0xFA, 0x00, 0x10, 0x27, 0x34, 0x12};
    const uint8_t page0[8]  = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    d.println(F("[TEST] feed node 3 rail page + node 5 status page -> expect 2x _NODE_TELEM"));
    PanelBridge::testFeedCanFrame(canIdTelem(3), rail12, 8);
    PanelBridge::testFeedCanFrame(canIdTelem(5), status, 8);
    d.println(F("[TEST] feed page-0, 4-byte and TELEM_0 frames -> expect nothing"));
    PanelBridge::testFeedCanFrame(canIdTelem(3), page0, 8);
    PanelBridge::testFeedCanFrame(canIdTelem(3), rail12, 4);
    PanelBridge::testFeedCanFrame(canIdTelem(0), rail12, 8);
    d.println(F("[TEST] roster request -> expect node-0 row + _NODE_STATUS_END 1 (TELEM is not liveness)"));
    PanelBridge::testRequestNodeStatus();
    d.println(F("[TEST] --- cycle ---"));
}
