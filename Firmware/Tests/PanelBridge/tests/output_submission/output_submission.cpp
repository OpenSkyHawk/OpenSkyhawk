// PanelBridge — output_submission test
//
// Purpose: verify that handleDcsBiosExport() submits ControlPackets to
// CANProtocol::sendBatched(CAN_ID_CTRL_BCAST) for DCS-BIOS export addresses. The bridge forwards
// the whole export stream (#295) — any aircraft, CommonData, metadata — except 0x0000, the
// ControlPacket empty-slot sentinel.
//
// NOTE: testHandleExport() calls handleDcsBiosExport() directly, bypassing
// ExportDispatcher, so the listener's registration (0x0002-0xFFFE) is NOT exercised here.
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
    { 0x0402, 0x0001 },  // CommonData — forwarded like any aircraft output
    { 0x8400, 0x4000 },  // A-4E-C (RPM)
    { 0x8000, 0x1234 },  // AH-64D's block — forwarded whatever aircraft is flying
    { 0xFFFE, 0xFFFF },  // MetadataEnd — the last address the stream carries
    { 0x0000, 0x0042 },  // the empty-slot sentinel — dropped, nothing submitted
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
