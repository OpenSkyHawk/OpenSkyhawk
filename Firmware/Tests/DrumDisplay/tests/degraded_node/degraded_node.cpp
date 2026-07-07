// PanelGroup — degraded-node bench (#163)
//
// A REAL PanelGroup node (NODE_ID=1) that declares one DrumDisplay (the APN-153 speed drum) but is
// flashed to a board with NO OLED and NO mux wired. At boot DrumDisplay::begin() probes the absent
// mux/OLED, the I2cHealth breaker trips, faultCode() -> I2C_PERIPHERAL, and the periodic HEALTH_1
// TX packs DEGRADED + faultId. It lets you bench the #163 degraded path over CAN without any I2C
// hardware — the node "thinks" it drives a drum, but there's nothing on the bus.
//
// Pair with a PanelBridge node on the same CAN bus (NODE_ID=0):
//   - THIS board's DiagSerial (USART1 PA9, 115200): "[NODE] degraded: OLED not responding (fault 1)"
//     once at boot (the edge). Hot-plug an OLED+mux later -> "[NODE] recovered" within ~2 s.
//   - The bridge's _NODE_STATUS (USB-UART, 250000): node 1 present, hFlags DEGRADED (02), faultId 01.
//   - HB_1 keeps the node alive throughout — the breaker stops the dead device starving the
//     heartbeat (#166), so DEGRADED (a live-but-faulted node) is distinct from an offline node.
//
// Not a self-checking unit test (no PASS/FAIL) — a hardware bench sketch. CI still compiles it.
// Flash: ~/.platformio/penv/bin/pio run -d Firmware/Tests/DrumDisplay -e degraded_node -t upload

#include <Wire.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <DrumDisplay.h>
#include <Helpers/I2cMux/I2cMux.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

static const DrumSource SPEED_SRC[] = {
    { A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_X00_AM, 1, 2 },
    { A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_0X0_AM, 1, 1 },
    { A_4E_C_APN153_SPEED_00X, A_4E_C_APN153_SPEED_00X_AM, 1, 0 },
};
static const DrumReadout APN153_SPEED = {
    .sources = SPEED_SRC, .nSources = 3, .nDigits = 3,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
};

I2cMux mux(0x70, Wire);
// Global scope -> self-registers as a PanelGroup output AND a FaultSource. With no OLED/mux on the
// bus the begin() probe NAKs, the breaker trips, and this node reports I2C_PERIPHERAL to the
// aggregator — which the HEALTH_1 TX turns into DEGRADED + faultId on the wire.
DrumDisplay disp(oled, APN153_SPEED, mux, /*channel*/ 0, DrumFont::LARGE);

void setup() {
    STM32Board::setDebug(true);   // enable the [NODE] DiagSerial edge log
    Wire.begin();
    PanelGroup::setup();          // STM32Board::begin() + CAN; probes the (absent) drum -> trips breaker
}

void loop() {
    PanelGroup::loop();           // HEALTH_1 every 1 s: aggregateFaults() -> DEGRADED + faultId 01
}
