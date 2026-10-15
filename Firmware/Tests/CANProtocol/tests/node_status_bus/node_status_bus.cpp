// CANProtocol — node status service on a real bus (#340)
//
// One board as node NODE_ID=1 on a physical CAN bus, plus something playing the master: a
// PanelBridge, or a USB-CAN adapter sending HB_0 (0x100, DLC 8) every 500 ms and ACKing frames.
// The board runs only CANProtocol::serviceNodeStatus(), so the bus shows exactly what every node
// now sends: HB_1 (0x101) every 500 ms and HEALTH_1 (0x141) every 1000 ms.
//
// DiagSerial (115200) logs every status-LED state change with a timestamp, so the master-loss
// watchdog can be checked from the log instead of by eye:
//   t=<ms> LED <state>
// plus a summary line every 10 s:
//   t=<ms> summary warnRaises=<n> hb0Seen=<n> tec=<n> rec=<n>
//
// Expected with an adapter that sends HB_0, then stops for a few seconds, then resumes:
//   - no WARNING before the first HB_0
//   - LED WARNING ~1500 ms after the last HB_0 of the gap
//   - LED back to NORMAL on the first HB_0 after the gap
//   - tec / rec stay 0 (frames are being ACKed)
// Soak: leave HB_0 running; warnRaises must stay 0.

#include <STM32Board.h>
#include <CANProtocol.h>

static uint32_t _hb0Seen    = 0;
static uint32_t _warnRaises = 0;

static void onRx(uint32_t canId, const uint8_t*, uint8_t) {
    if (canId == canIdHb(0)) _hb0Seen++;
}

static const __FlashStringHelper* stateName(LedState s) {
    switch (s) {
        case LedState::OFF:       return F("OFF");
        case LedState::BOOTING:   return F("BOOTING");
        case LedState::NORMAL:    return F("NORMAL");
        case LedState::CONNECTED: return F("CONNECTED");
        case LedState::CAN_ERROR: return F("CAN_ERROR");
        case LedState::BUS_OFF:   return F("BUS_OFF");
        case LedState::WARNING:   return F("WARNING");
    }
    return F("?");
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== node_status_bus ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::start();  // normal mode; HB_0 is added to the filter automatically
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();
    CANProtocol::serviceNodeStatus(0);

    auto& d = STM32Board::diagSerial();
    const uint32_t now = millis();

    static LedState last = LedState::OFF;
    const LedState cur = STM32Board::currentState();
    if (cur != last) {
        if (cur == LedState::WARNING) _warnRaises++;
        d.print(F("t=")); d.print(now); d.print(F(" LED ")); d.println(stateName(cur));
        last = cur;
    }

    static uint32_t lastSummary = 0;
    if (now - lastSummary >= 10000) {
        lastSummary = now;
        d.print(F("t=")); d.print(now);
        d.print(F(" summary warnRaises=")); d.print(_warnRaises);
        d.print(F(" hb0Seen=")); d.print(_hb0Seen);
        d.print(F(" tec=")); d.print(CANProtocol::tec());
        d.print(F(" rec=")); d.println(CANProtocol::rec());
    }
}
