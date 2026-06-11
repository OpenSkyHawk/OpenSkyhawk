// PanelBridge — input_dispatch_hid test
//
// Purpose: verify that EVT slots with controlId < 0x8000 produce the correct
// 6-byte HID frame (0xAA 0x55 controlId[2 LE] value[2 LE]) on Serial (UART2).
//
// Hardware: single STM32F103CB board. No second board or CAN bus required.
//   - Serial (UART2 PA2/PA3) → USB-UART adapter → PC terminal at 250000 baud (raw hex view)
//   - DiagSerial (USART1 PA9/PA10) at 115200 baud
//   - STLink for flash
//
// HOW TO USE:
//   1. Flash:   pio run -e test_input_dispatch_hid -t upload
//   2. Connect PA2 (TX) / PA3 (RX) to USB-UART adapter. Open terminal in raw hex view.
//   3. Open DiagSerial (USART1 PA9/PA10) at 115200 baud.
//   4. Observe on USB-UART every 1.5 s:
//        AA 55 10 00 00 80  — CTRL_ROLL, value=0x8000 (mid)
//        AA 55 20 00 01 00  — CTRL_HAT_0, value=0x0001 (N)
//        AA 55 30 00 01 00  — CTRL_TRIGGER, value=0x0001 (pressed)
//        AA 55 30 00 00 00  — CTRL_TRIGGER, value=0x0000 (released)
//        AA 55 17 00 FF FF  — reserved axis slot 7, full range
//   5. DCS-range controlId 0x8001 (DCSIN_ACCEL_RESET): nothing on USB-UART, DiagSerial
//      shows "[BRIDGE] drop ctrl=0x8001" — wait, 0x8001 >= CTRL_ID_DCS_MIN, so it routes
//      to DCS path not HID. Verify ASCII "ACCEL_RESET 1\n" on USB-UART adapter.
//
// Pass criteria (manual):
//   - Each controlId < 0x8000 produces exact 6-byte frame on USB-UART
//   - Frame byte layout: [AA][55][ID_LO][ID_HI][VAL_LO][VAL_HI]
//
// Uses PANELBRIDGE_TEST test hook. No CAN required.

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>
#include <HIDControls.h>

struct HidCase {
    uint16_t controlId;
    uint16_t value;
    const char* label;
};

static const HidCase CASES[] = {
    { CTRL_ROLL,    0x8000, "CTRL_ROLL mid"      },
    { CTRL_HAT_0,   0x0001, "CTRL_HAT_0 N"       },
    { CTRL_TRIGGER, 0x0001, "CTRL_TRIGGER press"  },
    { CTRL_TRIGGER, 0x0000, "CTRL_TRIGGER release" },
    { 0x0017,       0xFFFF, "reserved slot7 max"  },
};
static const uint8_t CASE_COUNT = sizeof(CASES) / sizeof(CASES[0]);

static uint8_t  _idx    = 0;
static uint32_t _lastMs = 0;

void setup() {
    STM32Board::setDebug(true);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] input_dispatch_hid ready"));
    STM32Board::diagSerial().println(F("[TEST] watch USB-UART (250000, raw hex) for 6-byte HID frames"));
    STM32Board::diagSerial().println(F("[TEST] expected format: AA 55 [id_lo] [id_hi] [val_lo] [val_hi]"));
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

    const HidCase& c = CASES[_idx++];
    auto& d = STM32Board::diagSerial();
    d.print(F("[TEST] dispatch "));         d.println(c.label);
    d.print(F("[TEST] expect frame: AA 55 "));
    d.print(c.controlId & 0xFF, HEX);       d.print(' ');
    d.print(c.controlId >> 8,   HEX);       d.print(' ');
    d.print(c.value      & 0xFF, HEX);      d.print(' ');
    d.println(c.value    >> 8,   HEX);

    PanelBridge::testDispatchEvt(c.controlId, c.value);
}
