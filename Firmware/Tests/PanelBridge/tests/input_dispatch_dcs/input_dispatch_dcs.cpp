// PanelBridge — input_dispatch_dcs test
//
// Purpose: verify that EVT slots with controlId 0x8000-0x86FF produce the correct
// DCS-BIOS ASCII command on Serial (UART2). Also verifies that 0x8700 and 0xFFFF
// are dropped (nothing sent on Serial).
//
// Hardware: single STM32F103CB board. No second board or CAN bus required.
//   - Serial (UART2 PA2/PA3) → USB-UART adapter → PC terminal at 250000 baud
//   - DiagSerial (USART1 PA9/PA10) at 115200 baud (progress/test log)
//   - STLink for flash
//
// HOW TO USE:
//   1. Flash:   pio run -e test_input_dispatch_dcs -t upload
//   2. Connect PA2 (TX) / PA3 (RX) to USB-UART adapter. Open terminal at 250000 baud.
//   3. Open DiagSerial (USART1 PA9/PA10) at 115200 baud.
//   4. Observe on USB-UART terminal every 1.5 s:
//        ARM_MASTER 1\n
//        ARM_MASTER 0\n
//        ARM_GUN 1\n
//        GEAR_HANDLE 0\n
//   5. For out-of-range IDs (0x8700, 0xFFFF): nothing on USB-UART; DiagSerial shows
//        [BRIDGE] drop ctrl=0x8700
//        [BRIDGE] drop ctrl=0xFFFF
//
// Pass criteria (manual):
//   - Expected ASCII commands appear on USB-UART at correct interval
//   - Out-of-range IDs produce drop log on DiagSerial and no output on USB-UART

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>
#include <A4EC_CmdIds.h>

struct Case {
    uint16_t    controlId;
    uint16_t    value;
    const char* label;
    bool        expectOutput;
};

static const Case CASES[] = {
    { DCSIN_ARM_MASTER,  1, "ARM_MASTER val=1",  true  },
    { DCSIN_ARM_MASTER,  0, "ARM_MASTER val=0",  true  },
    { DCSIN_ARM_GUN,     1, "ARM_GUN val=1",     true  },
    { DCSIN_GEAR_HANDLE, 0, "GEAR_HANDLE val=0", true  },
    { 0x8700,            0, "0x8700 (drop)",     false },
    { 0xFFFF,            0, "0xFFFF (drop)",     false },
};
static const uint8_t CASE_COUNT = sizeof(CASES) / sizeof(CASES[0]);

static uint8_t  _idx    = 0;
static uint32_t _lastMs = 0;

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: input_dispatch_dcs ==="));
    STM32Board::setDebug(true);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] input_dispatch_dcs ready"));
    STM32Board::diagSerial().println(F("[TEST] watch USB-UART (250000) for DCS-BIOS commands"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();

    uint32_t now = millis();
    if (now - _lastMs < 1500) return;
    _lastMs = now;

    if (_idx >= CASE_COUNT) {
        _idx = 0;
        STM32Board::diagSerial().println(F("[TEST] --- cycle ---"));
    }

    const Case& c = CASES[_idx++];
    auto& d = STM32Board::diagSerial();
    d.print(F("[TEST] dispatch ")); d.println(c.label);
    if (c.expectOutput) d.println(F("[TEST] expect ASCII on USB-UART"));
    else                d.println(F("[TEST] expect drop (no USB-UART output)"));

    PanelBridge::testDispatchEvt(c.controlId, c.value);
}
