// PanelBridge — model_time_resync test
//
// Purpose: verify DCS session change detection via model-time string buffer.
// First valid model-time value seeds state (no SYNC_REQ). Later value that decreases
// triggers SYNC_REQ broadcast.
//
// Hardware: single STM32F103CB board.
//   - Serial (UART2 PA2/PA3) → USB-UART adapter @ 250000 baud (sends DCS-BIOS export bytes)
//   - DiagSerial (USART1 PA9/PA10) at 115200 baud (progress log)
//   - STLink for flash
//
// HOW TO USE:
//   1. Flash:   pio run -e test_model_time_resync -t upload
//   2. Connect PA3 (UART2 RX) to USB-UART adapter TX. Open adapter at 250000 baud.
//   3. Open DiagSerial (USART1 PA9/PA10) at 115200 baud.
//   4. Send DCS-BIOS binary export frames on the adapter for CommonData_MOD_TIME_A (0x0440):
//        First: model_time = "100  " (100 seconds) — seeds state; NO SYNC_REQ expected.
//        Then:  model_time = "200  " (200 seconds) — higher; NO SYNC_REQ expected.
//        Then:  model_time = "050  " (50 seconds)  — DECREASE; SYNC_REQ expected.
//   5. Watch DiagSerial for:
//        [BRIDGE] DCS session change — SYNC_REQ
//        [BRIDGE] SYNC_REQ broadcast
//
// DCS-BIOS binary export frame format (see DCS-BIOS protocol docs):
//   0x55 0x55 <count_hi> <count_lo> <address_lo> <address_hi> <len_lo> <len_hi> <data...> 0x55 0x55 <count> <count>
//   For model_time string at 0x0440 (5 bytes): address=0x0440, len=5, data="100  "
//
// Pass criteria (manual, DiagSerial):
//   - "[BRIDGE] DCS session change — SYNC_REQ" appears only on the third send (decrease)
//
// DEFERRED (2026-06-11): USB-UART adapters available during Phase 2 testing cannot open
// at 250000 baud (macOS stty rejects the rate). Injection requires a sender at 250000.
// Deferred to Phase 6 SimGateway integration — RP2040 UART handles 250000 natively and
// will feed live model-time data through PA3, exercising this path end-to-end.

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: model_time_resync ==="));
    STM32Board::setDebug(true);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] model_time_resync ready"));
    STM32Board::diagSerial().println(F("[TEST] send DCS-BIOS binary frames on PA3 (UART2 RX):"));
    STM32Board::diagSerial().println(F("[TEST]   1. model_time='100  ' → seeds state, no SYNC_REQ"));
    STM32Board::diagSerial().println(F("[TEST]   2. model_time='200  ' → higher, no SYNC_REQ"));
    STM32Board::diagSerial().println(F("[TEST]   3. model_time='050  ' → DECREASE → SYNC_REQ expected"));
    STM32Board::diagSerial().println(F("[TEST] watch for [BRIDGE] DCS session change"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}
