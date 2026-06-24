// DrumDisplay — I2C circuit breaker (#164).
//
// No OLED/mux hardware needed: the probe result is forced via a test seam, so this validates the
// breaker LOGIC (trip / back-off / recover) + the render gating without ever running a real I2C op
// (which is the whole point — a dead device must not stall the loop). Runs on a bare STM32F103C8.
//
// Asserts:
//   - the data path (onControlPacket) decodes the value even while the device is "down"
//   - a dead probe trips the breaker, classifies the fault, and SKIPS the render (no sendBuffer)
//   - while tripped, a retry within RETRY_MS backs off — it does NOT re-probe
//   - after the back-off expires and the device returns, it re-probes, heals, fault clears
//
// Pass: every line is [PASS]; the sketch prints "=== ALL PASS ===".

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <STM32Board.h>
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
    SPEED_SRC, 3, 3, 4.5f, 8.0f, 1.0f, 0.0f, 0, nullptr, 0,
    { false, 0, 0, nullptr, 0, 0.0f }, DrumScroll::SNAP_SETTLE, 3.0f,
};

I2cMux mux(0x70, Wire);
DrumDisplay disp(oled, APN153_SPEED, mux, /*channel*/ 0, DrumFont::LARGE);

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}
static uint16_t digitWord(int digit) {
    return static_cast<uint16_t>(lroundf(digit / 9.0f * 65535.0f));
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay breaker (#164) ==="));

    // Data path keeps decoding even with the panel "down" (no I2C in onControlPacket).
    disp.onControlPacket(A_4E_C_APN153_SPEED_X00, digitWord(2));
    disp.onControlPacket(A_4E_C_APN153_SPEED_0X0, digitWord(5));
    disp.onControlPacket(A_4E_C_APN153_SPEED_00X, digitWord(0));
    check("data path decodes while down (target == 250)", disp.debugTarget() == 250);

    // 1) Dead device → trip + classify + skip the render (no sendBuffer).
    disp.debugForceProbe(0);                       // simulate a NAK
    const uint32_t r0 = disp.debugRenderCount();
    disp.update();                                 // dirty + first frame → reaches the gate → skips
    check("dead: breaker tripped (unhealthy)",     !disp.debugHealthy());
    check("dead: fault classified (Device)",       disp.debugFault() == static_cast<uint8_t>(DrumDisplay::Fault::Device));
    check("dead: render skipped (no sendBuffer)",  disp.debugRenderCount() == r0);

    // 2) Back-off — a retry within RETRY_MS must NOT re-probe.
    const uint32_t p1 = disp.debugProbeCount();
    const bool retry  = disp.debugReachable();
    check("backoff: still unreachable",            !retry);
    check("backoff: did NOT re-probe",             disp.debugProbeCount() == p1);

    // 3) Recover — expire the back-off + device returns → re-probes, heals, fault clears.
    disp.debugForceProbe(1);
    disp.debugExpireBackoff();
    const bool healed = disp.debugReachable();
    check("recover: probed again",                 disp.debugProbeCount() == p1 + 1);
    check("recover: healthy",                      healed && disp.debugHealthy());
    check("recover: fault cleared (None)",         disp.debugFault() == static_cast<uint8_t>(DrumDisplay::Fault::None));

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {}
