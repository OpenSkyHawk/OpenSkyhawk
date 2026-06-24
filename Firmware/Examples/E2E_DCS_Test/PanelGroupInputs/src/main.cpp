// E2E_DCS_Test — PanelGroup INPUTS-ONLY node (STM32F103C8, NODE 1)
//
// Stripped variant of the full E2E node: ONLY the four DCS-routed inputs — no OLED drums, stepper,
// LED, or button. Use this when the bench has just the inputs wired.
//
// Why it exists: the full node (../PanelGroup) drives two OLED DrumDisplays + a stepper every loop.
// With that hardware ABSENT, the live DCS export keeps changing the drum sources, so DrumDisplay
// keeps trying to render over I2C to OLEDs that aren't there — every transaction times out, the
// loop stalls, and the node's heartbeat lands late → the bridge flaps NODE 1 online/offline and
// input commands arrive seconds late. This sketch keeps the loop tight so the node stays CONNECTED.
//
//   RotaryEncoder REL  DEST_LAT_KNB      PA8/PB5
//   RotaryEncoder DIR  ARC51_FREQ_10MHZ  PB3/PB4   (JTAG-DP pins — remapped in setup())
//   AnalogInput        ARC51_VOL         PA2   (hysteresis 1024 — calm dispatch rate)
//   AnalogMultiPos     ARC51_MODE        PA3

#include <OpenSkyhawk.h>

using namespace OpenSkyhawk;

RotaryEncoder  destLat (DCSIN_DEST_LAT_KNB,     PinRef(PA8), PinRef(PB5), EncoderStepsPerDetent::Four, EncoderMode::Rel, /*step=*/1600);
RotaryEncoder  freq10  (DCSIN_ARC51_FREQ_10MHZ, PinRef(PB3), PinRef(PB4), EncoderStepsPerDetent::Four, EncoderMode::Dir);
AnalogInput    arcVol  (DCSIN_ARC51_VOL,  PinRef(PA2), /*reverse=*/false, 0, 65535, /*hysteresis=*/1024);
AnalogMultiPos arcMode (DCSIN_ARC51_MODE, PinRef(PA3), 4);

void setup() {
    STM32Board::setDebug(true);
    // Free PB3/PB4 (JTAG-DP) for the DIR encoder. SWD stays live — ST-Link still flashes.
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
