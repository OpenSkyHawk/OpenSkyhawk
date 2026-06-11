// PanelBridge — output_submission test
//
// Purpose: verify that handleDcsBiosExport() submits ControlPackets to
// CANProtocol::sendBatched(CAN_ID_CTRL_BCAST) for in-range DCS-BIOS addresses.
//
// NOTE: testHandleExport() calls handleDcsBiosExport() directly, bypassing
// BridgeExportListener. The listener's registration and address filter (0x8000-0x86FF)
// are NOT exercised by this test. Do not add out-of-range address cases here —
// they would be submitted to sendBatched regardless of address value.
//
// Hardware: single STM32F103CB board.
//   - DiagSerial (USART1 PA9/PA10) at 115200 baud
//   - STLink for flash
//   (No second board or CAN bus required — PanelBridge::setup() starts CAN; TX
//   frames may not be ACKed without a bus, but the submission path under test
//   does not depend on TX success.)
//
// HOW TO USE:
//   1. Flash:   pio run -e test_output_submission -t upload
//   2. Connect DiagSerial at 115200 baud.
//   3. Observe every 2 s: addresses submitted without crash or hard fault.
//
// Pass criteria (manual, DiagSerial):
//   - No crash or hard fault across all cases
//   - No [BRIDGE] error messages
//
// Uses PANELBRIDGE_TEST test hook (testHandleExport).

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>

struct ExportCase {
    uint16_t address;
    uint16_t value;
};

static const ExportCase CASES[] = {
    { 0x8001, 0x0001 },  // first valid cockpit output address
    { 0x8040, 0x4000 },  // arbitrary mid-range address
    { 0x86FF, 0xFFFF },  // last valid DCS output address — boundary check
};
static const uint8_t CASE_COUNT = sizeof(CASES) / sizeof(CASES[0]);

static uint8_t  _idx    = 0;
static uint32_t _lastMs = 0;

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: output_submission ==="));
    STM32Board::setDebug(true);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] output_submission ready"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();

    uint32_t now = millis();
    if (now - _lastMs < 2000) return;
    _lastMs = now;

    if (_idx >= CASE_COUNT) {
        _idx = 0;
        STM32Board::diagSerial().println(F("[TEST] --- cycle ---"));
    }

    const ExportCase& c = CASES[_idx++];
    auto& d = STM32Board::diagSerial();
    d.print(F("[TEST] export addr=0x")); d.print(c.address, HEX);
    d.print(F(" val=0x"));              d.println(c.value,  HEX);

    PanelBridge::testHandleExport(c.address, c.value);
}
