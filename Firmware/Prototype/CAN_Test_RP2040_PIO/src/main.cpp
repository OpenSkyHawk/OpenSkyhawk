// DCS-BIOS bridge test — Waveshare RP2040-Plus (no STM32 connected)
//
// Receives fake DCS-BIOS binary frames from a laptop Python script over USB CDC,
// parses them using the DCS-BIOS Arduino library, and echoes parsed values back
// as human-readable ACK lines so the laptop can confirm the parser is working.
//
// USR button (GP24, active-low) sends BTN:USR:1 (press) / BTN:USR:0 (release).
//
// NeoPixel (GP23, R68 bridged) colour:
//   ARM_MASTER on  → solid red (overrides altitude)
//   ARM_MASTER off → altitude gradient:
//     ground (0 ft)    → red
//     mid    (~20k ft) → green
//     high   (40k+ ft) → blue
//
// Altitude is reconstructed from drum counters:
//   alt_feet = (D_ALT_10K / 65535) * 100000 + (D_ALT_1K / 65535) * 10000
// A-4E-C service ceiling ~49,000 ft; gradient is normalised over 40,000 ft.
//
// Serial port: USB CDC @ 250000 baud (set by DcsBios::setup()).
//
// A-4E-C DCS-BIOS controls (constants from Addresses.h, included via DcsBios.h):
//   A_4E_C_RPM          0x8400  0xFFFF  0   Engine RPM
//   A_4E_C_D_FLAPS_IND  0x840E  0xFFFF  0   Flap position indicator
//   A_4E_C_D_RADAR_ALT  0x8428  0xFFFF  0   Radar altimeter needle
//   A_4E_C_D_ALT_10K    0x8430  0xFFFF  0   Altimeter 10,000s drum
//   A_4E_C_D_ALT_1K     0x8432  0xFFFF  0   Altimeter 1,000s drum
//   A_4E_C_D_ALT_100S   0x8434  0xFFFF  0   Altimeter 100s drum
//   A_4E_C_ARM_MASTER   0x8500  0x4000  14  Master Arm switch

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Joystick.h>
#include <USB.h>
#include <CANProtocol.h>

// ── NeoPixel ──────────────────────────────────────────────────────────────────
static constexpr uint8_t NEO_PIN   = 23;
static constexpr uint8_t NEO_COUNT = 1;
Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

static bool     armMasterOn = false;
static uint16_t alt10k      = 0;   // D_ALT_10K value (0-65535)
static uint16_t alt1k       = 0;   // D_ALT_1K  value (0-65535)

// Reconstruct altitude in feet from the two drum counters, then map to colour.
// alt_feet = (alt10k / 65535) * 100000 + (alt1k / 65535) * 10000
// Normalised over 40,000 ft: 0.0 = ground (red), 1.0 = 40k ft (blue).
static void updateAltColour() {
    if (armMasterOn) return;

    uint32_t alt_feet = ((uint32_t)alt10k * 100000UL + (uint32_t)alt1k * 10000UL) / 65535UL;
    float norm = (float)alt_feet / 40000.0f;
    if (norm > 1.0f) norm = 1.0f;

    uint8_t r, g, b;
    if (norm < 0.5f) {
        // red → green
        uint8_t t = (uint8_t)(norm * 2.0f * 255.0f);
        r = 255 - t;
        g = t;
        b = 0;
    } else {
        // green → blue
        uint8_t t = (uint8_t)((norm - 0.5f) * 2.0f * 255.0f);
        r = 0;
        g = 255 - t;
        b = t;
    }
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

// ── CAN ping-pong latency test ────────────────────────────────────────────────
// Serial1 (GP0 TX / GP1 RX) @ 250000 → STM32 master
// Sends ControlPacket{0xFFFF, seq} every second.
// Master → CAN TEST_SEQ (0x011) → SubNode echoes → Master returns DIAG_RTT.
// RP2040 measures full round-trip time and prints stats.

static constexpr uint32_t PING_INTERVAL_MS = 1000;
static constexpr uint32_t PING_TIMEOUT_MS  = 200;

static uint16_t  pingSeq     = 0;
static uint32_t  pingSentMs  = 0;
static bool      pingPending = false;

// RTT stats
static uint32_t rttMin   = UINT32_MAX;
static uint32_t rttMax   = 0;
static uint64_t rttSum   = 0;
static uint32_t rttCount = 0;
static uint32_t rttLost  = 0;

// DIAG frame receive buffer
static uint8_t  diagBuf[8];
static uint8_t  diagPos = 0;

static void pingLoop() {
    uint32_t now = millis();

    // Send a ping every PING_INTERVAL_MS
    if (!pingPending && (now - pingSentMs >= PING_INTERVAL_MS)) {
        ControlPacket pkt = {CTRL_TEST_SEQ, pingSeq};
        Serial1.write(reinterpret_cast<uint8_t*>(&pkt), 4);
        pingSentMs  = now;
        pingPending = true;
    }

    // Timeout — no ACK within PING_TIMEOUT_MS
    if (pingPending && (now - pingSentMs > PING_TIMEOUT_MS)) {
        rttLost++;
        Serial.print(F("PING TIMEOUT seq="));
        Serial.print(pingSeq);
        Serial.print(F("  lost="));
        Serial.println(rttLost);
        pingPending = false;
        pingSeq++;
        pingSentMs = now - PING_INTERVAL_MS;  // trigger next ping immediately
    }

    // Drain DIAG frames from Serial1 (8-byte framing)
    while (Serial1.available()) {
        uint8_t b = Serial1.read();

        // Resync: if first byte isn't DIAG_MAGIC, shift and try again
        if (diagPos == 0 && b != DIAG_MAGIC) continue;

        diagBuf[diagPos++] = b;
        if (diagPos < 8) continue;
        diagPos = 0;

        switch (diagBuf[1]) {
            case DIAG_RTT: {
                uint16_t seq;
                memcpy(&seq, diagBuf + 2, 2);
                if (seq == pingSeq && pingPending) {
                    uint32_t rtt = millis() - pingSentMs;
                    pingPending = false;
                    pingSeq++;
                    if (rtt < rttMin) rttMin = rtt;
                    if (rtt > rttMax) rttMax = rtt;
                    rttSum += rtt;
                    rttCount++;
                    Serial.print(F("PONG seq="));
                    Serial.print(seq);
                    Serial.print(F("  RTT="));
                    Serial.print(rtt);
                    Serial.print(F(" ms  avg="));
                    Serial.print((uint32_t)(rttSum / rttCount));
                    Serial.print(F(" min="));
                    Serial.print(rttMin);
                    Serial.print(F(" max="));
                    Serial.print(rttMax);
                    Serial.print(F("  lost="));
                    Serial.println(rttLost);
                }
                break;
            }
            case DIAG_HB: {
                Serial.print(F("HB node="));
                Serial.print(diagBuf[2]);
                uint16_t rxc;
                memcpy(&rxc, diagBuf + 4, 2);
                Serial.print(F("  rx="));
                Serial.println(rxc);
                break;
            }
            case DIAG_ERR: {
                Serial.print(F("CAN_ERR TEC="));
                Serial.print(diagBuf[2]);
                Serial.print(F(" REC="));
                Serial.print(diagBuf[3]);
                Serial.print(F(" flags=0x"));
                Serial.println(diagBuf[4], HEX);
                break;
            }
        }
    }
}

// ── USR button ────────────────────────────────────────────────────────────────
static constexpr uint8_t USR_PIN = 24;

// ── DCS-BIOS callbacks ────────────────────────────────────────────────────────

void onRpmChange(unsigned int v) {
    // Only report RPM at key thresholds
    static unsigned int lastRpm = 0xFFFF;
    unsigned int pct = v / 655;  // 0-100
    unsigned int lastPct = lastRpm / 655;
    if (pct / 10 != lastPct / 10) {  // print every 10%
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

void onRadarAltChange(unsigned int v) {
    (void)v;  // silent — tracked by baro alt
}
DcsBios::IntegerBuffer radarAltBuf(A_4E_C_D_RADAR_ALT, onRadarAltChange);

static uint32_t lastPrintedAlt = 0xFFFFFFFF;  // force first print

static void printAltIfChanged() {
    uint32_t alt_feet = ((uint32_t)alt10k * 100000UL + (uint32_t)alt1k * 10000UL) / 65535UL;
    // Only print when altitude changes by more than 500 ft
    uint32_t diff = alt_feet > lastPrintedAlt ? alt_feet - lastPrintedAlt
                                              : lastPrintedAlt - alt_feet;
    if (diff >= 500 || lastPrintedAlt == 0xFFFFFFFF) {
        Serial.print("ALT: "); Serial.print(alt_feet); Serial.println(" ft");
        lastPrintedAlt = alt_feet;
    }
}

void onAlt10kChange(unsigned int v) {
    alt10k = (uint16_t)v;
    updateAltColour();
    printAltIfChanged();
}
DcsBios::IntegerBuffer alt10kBuf(A_4E_C_D_ALT_10K, onAlt10kChange);

void onAlt1kChange(unsigned int v) {
    alt1k = (uint16_t)v;
    updateAltColour();
    printAltIfChanged();
}
DcsBios::IntegerBuffer alt1kBuf(A_4E_C_D_ALT_1K, onAlt1kChange);

void onAlt100sChange(unsigned int v) {
    // 100s drum — used for colour only, not logged
    (void)v;
}
DcsBios::IntegerBuffer alt100sBuf(A_4E_C_D_ALT_100S, onAlt100sChange);

void onArmMasterChange(unsigned int v) {
    Serial.print("ACK:ARM_MASTER:"); Serial.println(v);
    armMasterOn = (v != 0);
    if (armMasterOn) {
        strip.setPixelColor(0, strip.Color(255, 0, 0));  // solid red = armed
        strip.show();
    } else {
        updateAltColour();  // restore altitude colour
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

    // Set USB identity before the stack starts — must precede Joystick.begin() / DcsBios::setup()
    USB.setManufacturer("OpenSkyhawk");
    USB.setProduct("A-4E Skyhawk");
    USB.setVIDPID(0x2E8A, 0x4134);

    Serial1.begin(250000);     // GP0 TX / GP1 RX → STM32 master UART2 (PA3/PA2)

    Joystick.use16bit();       // full ±32767 range on all axes
    Joystick.useManualSend(true);  // batch axis + button updates into one report
    Joystick.begin();
    DcsBios::setup();      // CDC serial @ 250000 — composite device with joystick
}

void loop() {
    DcsBios::loop();
    pingLoop();

    // USR button — debounced, drives both HID joystick and serial event
    static bool     lastStable   = HIGH;
    static bool     lastRaw      = HIGH;
    static uint32_t debounceMs   = 0;
    static constexpr uint32_t DEBOUNCE = 20;  // ms

    bool raw = digitalRead(USR_PIN);
    if (raw != lastRaw) {
        debounceMs = millis();   // restart debounce timer on any change
        lastRaw = raw;
    }
    if ((millis() - debounceMs) >= DEBOUNCE && raw != lastStable) {
        lastStable = raw;
        bool pressed = (raw == LOW);
        Joystick.button(1, pressed);          // HID button 1
        Serial.println(pressed ? "BTN:USR:1" : "BTN:USR:0");  // serial event
    }
}
