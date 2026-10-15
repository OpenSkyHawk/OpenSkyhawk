// CANProtocol — node status service test (#340)
//
// Single board, silent loopback (own frames come back). NODE_ID=1. Exercises
// CANProtocol::serviceNodeStatus() end to end:
//   - start() auto-accepts HB_0 on a non-bridge node, and drain() still forwards it to onReceive
//   - HB_1 every 500 ms, the first one ~500 ms after boot completes — setup() waits 300 ms after
//     start() to stand in for PanelGroup steps 5-7 (FirmwarePlan 09 step 8)
//   - HEALTH_1 every 1000 ms, the first one ~1000 ms after the first loop()
//   - master-loss watchdog: no WARNING before a master is seen; no WARNING while HB_0 flows;
//     WARNING once HB_0 stops for > MASTER_TIMEOUT_MS; cleared by the next HB_0
//
// The test plays the master itself: it sends HB_0 frames, which loop back as if PanelBridge sent
// them. HB_0 is sent t=300..2000 ms, paused 2000..4000 ms, resumed from 4000 ms.
//
// Expected output (DiagSerial 115200), all PASS:
//   HB_0 auto-accepted + forwarded: PASS
//   first HB_1 at ~500 ms:          PASS
//   HB_1 period 500 ms:             PASS
//   first HEALTH_1 at ~1000 ms:     PASS
//   HEALTH_1 period 1000 ms:        PASS
//   no WARNING before master seen:  PASS
//   no WARNING while HB_0 flows:    PASS
//   WARNING after HB_0 stops:       PASS
//   WARNING cleared on HB_0 return: PASS
//
// Overheat build (env test_node_status_overheat, -DNODE_OVERHEAT_C=0 → every HEALTH sample is
// "overheat"): WARNING has two causes, and a returning master must not clear the overheat one
// (PR #342 review P1). Same probes, overheat expectations:
//   no WARNING before first HEALTH:  PASS   (t=250, nothing sampled yet)
//   WARNING held while HB_0 flows:   PASS   (overheat since the 1 s HEALTH)
//   WARNING after HB_0 stops:        PASS
//   WARNING held on HB_0 return:     PASS   (master back, overheat still true)

#include <STM32Board.h>
#include <CANProtocol.h>

static constexpr uint8_t  MAX_STAMPS = 16;
static constexpr uint32_t TOL_MS     = 25;   // loop + loopback jitter allowance

static uint32_t _startMs = 0;

static uint32_t _hbStamps[MAX_STAMPS];
static uint8_t  _hbCount = 0;
static uint32_t _healthStamps[MAX_STAMPS];
static uint8_t  _healthCount = 0;
static uint8_t  _hb0Seen = 0;

static void onRx(uint32_t canId, const uint8_t*, uint8_t) {
    const uint32_t t = millis() - _startMs;
    if (canId == canIdHb(0)) _hb0Seen++;
    if (canId == canIdHb(NODE_ID)     && _hbCount     < MAX_STAMPS) _hbStamps[_hbCount++]         = t;
    if (canId == canIdHealth(NODE_ID) && _healthCount < MAX_STAMPS) _healthStamps[_healthCount++] = t;
}

static bool near(uint32_t v, uint32_t target) {
    return v + TOL_MS >= target && v <= target + TOL_MS;
}

// Every gap between consecutive stamps within TOL_MS of `period`.
static bool periodOk(const uint32_t* s, uint8_t n, uint32_t period) {
    if (n < 3) return false;
    for (uint8_t i = 1; i < n; i++)
        if (!near(s[i] - s[i - 1], period)) return false;
    return true;
}

static bool warningOn() { return STM32Board::currentState() == LedState::WARNING; }

static void pass(const __FlashStringHelper* label, bool ok) {
    auto& d = STM32Board::diagSerial();
    d.print(label);
    d.println(ok ? F("PASS") : F("FAIL"));
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== node_status_service ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    // Own HB/HEALTH must pass the list filter to be timed; HB_0 is deliberately NOT added here —
    // start() must add it on its own for a non-bridge node.
    CANProtocol::filterAcceptId(canIdHb(NODE_ID));
    CANProtocol::filterAcceptId(canIdHealth(NODE_ID));
    CANProtocol::startLoopback();
    // Stand-in for PanelGroup setup steps 5-7 (EVT burst, flush, READY) between start() and boot
    // complete. The first HB/HEALTH must be timed from the END of setup, not from start() — with
    // timers armed in start() the first HB_1 would land ~200 ms after boot (PR #342 review P2).
    delay(300);
    _startMs = millis();  // boot complete
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();
    CANProtocol::serviceNodeStatus(0);

    const uint32_t t = millis() - _startMs;

    // Fake master: HB_0 every 500 ms in [300, 2000) and from 4000 on.
    static uint32_t lastHb0 = 0;
    const bool masterUp = (t >= 300 && t < 2000) || t >= 4000;
    if (masterUp && (lastHb0 == 0 || t - lastHb0 >= 500)) {
        lastHb0 = t;
        HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(0, 0);
        CANProtocol::send(canIdHb(0), reinterpret_cast<const uint8_t*>(&hb), sizeof(hb));
    }

    // Watchdog probes, each latched once at its checkpoint.
    static bool warnBeforeMaster = true, warnWhileFlowing = true, warnAfterStop = false,
                warnAfterReturn = true;
    static bool p1 = false, p2 = false, p3 = false, p4 = false;
    if (!p1 && t >= 250)  { p1 = true; warnBeforeMaster = warningOn(); }  // nothing seen yet
    if (!p2 && t >= 1900) { p2 = true; warnWhileFlowing = warningOn(); }  // HB_0 flowing
    // Last HB_0 at ~1800 ms → timeout at ~3300 ms; probe at 3700.
    if (!p3 && t >= 3700) { p3 = true; warnAfterStop    = warningOn(); }
    if (!p4 && t >= 4200) { p4 = true; warnAfterReturn  = warningOn(); }  // HB_0 back at 4000

    static bool reported = false;
    if (!reported && t >= 6000) {
        reported = true;
        auto& d = STM32Board::diagSerial();
        d.println(F("--- node_status_service results ---"));
        pass(F("HB_0 auto-accepted + forwarded: "), _hb0Seen > 0);
        pass(F("first HB_1 at ~500 ms:          "), _hbCount > 0 && near(_hbStamps[0], 500));
        pass(F("HB_1 period 500 ms:             "), periodOk(_hbStamps, _hbCount, 500));
        pass(F("first HEALTH_1 at ~1000 ms:     "), _healthCount > 0 && near(_healthStamps[0], 1000));
        pass(F("HEALTH_1 period 1000 ms:        "), periodOk(_healthStamps, _healthCount, 1000));
#ifdef NODE_OVERHEAT_C
        pass(F("no WARNING before first HEALTH: "), !warnBeforeMaster);
        pass(F("WARNING held while HB_0 flows:  "), warnWhileFlowing);
        pass(F("WARNING after HB_0 stops:       "), warnAfterStop);
        pass(F("WARNING held on HB_0 return:    "), warnAfterReturn);
#else
        pass(F("no WARNING before master seen:  "), !warnBeforeMaster);
        pass(F("no WARNING while HB_0 flows:    "), !warnWhileFlowing);
        pass(F("WARNING after HB_0 stops:       "), warnAfterStop);
        pass(F("WARNING cleared on HB_0 return: "), !warnAfterReturn);
#endif
    }
}
