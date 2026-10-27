// PDU_Center — power distribution + monitoring node for the Center console (#202 hardware, #216 firmware).
// Identical on every PDU; NODE_ID comes from platformio.ini. Everything else is in the
// PowerMonitor library — see PduNode.h for what runs and the DiagSerial calibration commands.

#include <PduNode.h>

void setup() {
    // DiagSerial log off (#389): it blocks loop() at 115200 baud. The CAL commands still answer on
    // DiagSerial with it off. Bench only: STM32Board::setDebug(true);
    PduNode::setup();
}

void loop() {
    PduNode::loop();
}
