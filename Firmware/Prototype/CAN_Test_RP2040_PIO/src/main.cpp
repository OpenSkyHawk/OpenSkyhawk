// DCS-BIOS bridge — Waveshare RP2040-Plus
//
// SimGateway owns: USB identity, Joystick HID, DCS-BIOS CDC serial,
//                  UART link to PanelBridge (GP0 TX / GP1 RX @ 250000).
//
// This sketch adds: NeoPixel status, altitude colour logic, ping-pong RTT
// test against the CAN sub-nodes, and DCS-BIOS output callbacks.
//
// USR button (GP24, active-low) sends BTN:USR:1 (press) / BTN:USR:0 (release).
//
// NeoPixel (GP23, R68 bridged) colour:
//   ARM_MASTER on  → solid red (overrides altitude)
//   ARM_MASTER off → altitude gradient: ground=red, 20k ft=green, 40k ft=blue
//
// A-4E-C DCS-BIOS controls used:
//   A_4E_C_RPM          0x8400   Engine RPM
//   A_4E_C_D_FLAPS_IND  0x840E   Flap position indicator
//   A_4E_C_D_RADAR_ALT  0x8428   Radar altimeter needle
//   A_4E_C_D_ALT_10K    0x8430   Altimeter 10,000s drum
//   A_4E_C_D_ALT_1K     0x8432   Altimeter 1,000s drum
//   A_4E_C_D_ALT_100S   0x8434   Altimeter 100s drum
//   A_4E_C_ARM_MASTER   0x8500   Master Arm switch

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>       // must come before SimGateway (define must be set first)
#include <SimGateway.h>    // pulls in Joystick.h transitively
#include <Adafruit_NeoPixel.h>

// ── NeoPixel ──────────────────────────────────────────────────────────────────
static constexpr uint8_t NEO_PIN   = 23;
static constexpr uint8_t NEO_COUNT = 1;
Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

static bool     armMasterOn = false;
static uint16_t alt10k      = 0;
static uint16_t alt1k       = 0;

static void updateAltColour() {
    if (armMasterOn) return;
    uint32_t alt_feet = ((uint32_t)alt10k * 100000UL + (uint32_t)alt1k * 10000UL) / 65535UL;
    float norm = (float)alt_feet / 40000.0f;
    if (norm > 1.0f) norm = 1.0f;
    uint8_t r, g, b;
    if (norm < 0.5f) {
        uint8_t t = (uint8_t)(norm * 2.0f * 255.0f);
        r = 255 - t; g = t; b = 0;
    } else {
        uint8_t t = (uint8_t)((norm - 0.5f) * 2.0f * 255.0f);
        r = 0; g = 255 - t; b = t;
    }
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

// ── Ping-pong RTT test ────────────────────────────────────────────────────────
static constexpr uint32_t PING_INTERVAL_MS = 1000;
static constexpr uint32_t PING_TIMEOUT_MS  = 200;

static uint16_t  pingSeq     = 0;
static uint32_t  pingSentMs  = 0;
static bool      pingPending = false;
static uint32_t  rttMin      = UINT32_MAX;
static uint32_t  rttMax      = 0;
static uint64_t  rttSum      = 0;
static uint32_t  rttCount    = 0;
static uint32_t  rttLost     = 0;

static void onDiagRtt(uint16_t seq, uint32_t /*sentMs*/) {
    if (seq != pingSeq || !pingPending) return;
    uint32_t rtt = millis() - pingSentMs;
    pingPending  = false;
    pingSeq++;
    if (rtt < rttMin) rttMin = rtt;
    if (rtt > rttMax) rttMax = rtt;
    rttSum += rtt; rttCount++;
    Serial.print(F("PONG seq=")); Serial.print(seq);
    Serial.print(F("  RTT="));   Serial.print(rtt);
    Serial.print(F(" ms  avg=")); Serial.print((uint32_t)(rttSum / rttCount));
    Serial.print(F(" min="));    Serial.print(rttMin);
    Serial.print(F(" max="));    Serial.print(rttMax);
    Serial.print(F("  lost="));  Serial.println(rttLost);
}

static void onDiagHb(uint8_t nodeId, uint16_t rxCount) {
    Serial.print(F("HB node=")); Serial.print(nodeId);
    Serial.print(F("  rx="));   Serial.println(rxCount);
}

static void onDiagErr(uint8_t tec, uint8_t rec, uint8_t flags) {
    Serial.print(F("CAN_ERR TEC=")); Serial.print(tec);
    Serial.print(F(" REC="));        Serial.print(rec);
    Serial.print(F(" flags=0x"));    Serial.println(flags, HEX);
}

static void pingLoop() {
    uint32_t now = millis();
    if (!pingPending && (now - pingSentMs >= PING_INTERVAL_MS)) {
        SimGateway::send(CTRL_TEST_SEQ, pingSeq);
        pingSentMs  = now;
        pingPending = true;
    }
    if (pingPending && (now - pingSentMs > PING_TIMEOUT_MS)) {
        rttLost++;
        Serial.print(F("PING TIMEOUT seq=")); Serial.print(pingSeq);
        Serial.print(F("  lost="));           Serial.println(rttLost);
        pingPending = false;
        pingSeq++;
        pingSentMs = now - PING_INTERVAL_MS;
    }
}

// ── USR button ────────────────────────────────────────────────────────────────
static constexpr uint8_t USR_PIN = 24;

// ── DCS-BIOS callbacks ────────────────────────────────────────────────────────
void onRpmChange(unsigned int v) {
    static unsigned int lastRpm = 0xFFFF;
    unsigned int pct = v / 655;
    if (pct / 10 != (lastRpm / 655) / 10) {
        Serial.print("RPM: "); Serial.print(pct); Serial.println("%");
        lastRpm = v;
    }
}
DcsBios::IntegerBuffer rpmBuf(A_4E_C_RPM, onRpmChange);

void onFlapsIndChange(unsigned int v) {
    const char* pos = v < 10000 ? "UP" : v < 55000 ? "HALF" : "FULL";
    Serial.print("FLAPS: "); Serial.println(pos);
}
DcsBios::IntegerBuffer flapsIndBuf(A_4E_C_D_FLAPS_IND, onFlapsIndChange);

void onRadarAltChange(unsigned int v) { (void)v; }
DcsBios::IntegerBuffer radarAltBuf(A_4E_C_D_RADAR_ALT, onRadarAltChange);

static uint32_t lastPrintedAlt = 0xFFFFFFFF;
static void printAltIfChanged() {
    uint32_t alt_feet = ((uint32_t)alt10k * 100000UL + (uint32_t)alt1k * 10000UL) / 65535UL;
    uint32_t diff = alt_feet > lastPrintedAlt ? alt_feet - lastPrintedAlt
                                              : lastPrintedAlt - alt_feet;
    if (diff >= 500 || lastPrintedAlt == 0xFFFFFFFF) {
        Serial.print("ALT: "); Serial.print(alt_feet); Serial.println(" ft");
        lastPrintedAlt = alt_feet;
    }
}

void onAlt10kChange(unsigned int v) { alt10k = v; updateAltColour(); printAltIfChanged(); }
DcsBios::IntegerBuffer alt10kBuf(A_4E_C_D_ALT_10K, onAlt10kChange);

void onAlt1kChange(unsigned int v)  { alt1k  = v; updateAltColour(); printAltIfChanged(); }
DcsBios::IntegerBuffer alt1kBuf(A_4E_C_D_ALT_1K, onAlt1kChange);

void onAlt100sChange(unsigned int v) { (void)v; }
DcsBios::IntegerBuffer alt100sBuf(A_4E_C_D_ALT_100S, onAlt100sChange);

void onArmMasterChange(unsigned int v) {
    Serial.print("ACK:ARM_MASTER:"); Serial.println(v);
    armMasterOn = (v != 0);
    if (armMasterOn) {
        strip.setPixelColor(0, strip.Color(255, 0, 0));
        strip.show();
    } else {
        updateAltColour();
    }
}
DcsBios::IntegerBuffer armMasterBuf(A_4E_C_ARM_MASTER, onArmMasterChange);

// ── setup / loop ──────────────────────────────────────────────────────────────
void setup() {
    pinMode(USR_PIN, INPUT_PULLUP);

    strip.begin();
    strip.setBrightness(60);
    strip.setPixelColor(0, strip.Color(0, 0, 80));  // dim blue = waiting for data
    strip.show();

    SimGateway::onDiagRtt(onDiagRtt);
    SimGateway::onDiagHb(onDiagHb);
    SimGateway::onDiagErr(onDiagErr);
    SimGateway::setup(Serial1);  // sets USB identity, Joystick, starts UART
    DcsBios::setup();            // CDC serial @ 250000 — composite device with joystick
}

void loop() {
    DcsBios::loop();
    SimGateway::loop();  // UART drain + DIAG dispatch
    pingLoop();

    static bool     lastStable = HIGH;
    static bool     lastRaw    = HIGH;
    static uint32_t debounceMs = 0;
    static constexpr uint32_t DEBOUNCE = 20;

    bool raw = digitalRead(USR_PIN);
    if (raw != lastRaw) { debounceMs = millis(); lastRaw = raw; }
    if ((millis() - debounceMs) >= DEBOUNCE && raw != lastStable) {
        lastStable = raw;
        bool pressed = (raw == LOW);
        Joystick.button(1, pressed);
        Joystick.send_now();  // flush — manual-send mode requires explicit flush
        Serial.println(pressed ? "BTN:USR:1" : "BTN:USR:0");
    }
}
