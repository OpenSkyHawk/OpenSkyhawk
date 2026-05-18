// CAN_Test_Arduino — Arduino Mega 2560
//
// Serial  (USB, 250000) : DCS-BIOS input only
// Serial1 (pins 18/19, 250000) : UART to STM32 master
// Serial2 (pins 16/17, 115200) : command input + analytics / diagnostic output
//   - Wire Serial2 to a USB-TTL adapter for the debug console.
//
// Commands (send a single character over Serial2):
//   D  — DCS-BIOS capture mode: relay Serial→Serial1 and reset capture stats
//   I  — Idle (stop all injection)
//   S  — Slow sweep: one ControlPacket per 100 ms (10 pkts/sec)
//   F  — Fast burst: one ControlPacket per 1.6 ms (~625 pkts/sec)
//   X  — Extreme burst: back-to-back, no delay (saturate path deliberately)
//   T  — Throughput test: 1000 TEST_SEQ packets, print RTT histogram
//
// Wiring (Phase 1):
//   Serial1 TX (pin 18) → 1kΩ → STM32 PA3 → 2kΩ → GND  (5V→3.3V divider)
//   Serial1 RX (pin 19) ← STM32 PA2  (3.3V into Mega RX is fine)
//   GND ─ GND
//
// In Phase 2 disconnect this board; RP2040 Bridge takes over Serial1 path.

#include <Arduino.h>
#include <CANProtocol.h>

// ── modes ─────────────────────────────────────────────────────────────────────
enum class Mode : uint8_t { IDLE, SLOW, FAST, EXTREME, THROUGHPUT, DCS_CAPTURE };
static Mode mode = Mode::IDLE;

static constexpr bool START_IN_DCS_CAPTURE = true;
static constexpr uint32_t DCS_BIOS_BAUD = 250000;
static constexpr uint32_t DEBUG_BAUD = 115200;

// ── ControlPacket sweep table (Slow/Fast/Extreme) ────────────────────────────
static const ControlPacket sweepTable[] = {
    {0x0001, 1}, {0x0001, 0},   // MASTER_ARM toggle
    {0x0002, 1}, {0x0002, 2}, {0x0002, 0},   // BLEED_AIR 3-pos
    {0x0003, 0x0000}, {0x0003, 0x8000}, {0x0003, 0xFFFF}, {0x0003, 0x8000}, // HUD_BRT ramp
    {0x0004, 1}, {0x0004, 0},   // APU toggle
};
static constexpr uint8_t SWEEP_LEN = sizeof(sweepTable) / sizeof(sweepTable[0]);
static uint8_t sweepIdx = 0;

// ── throughput test ───────────────────────────────────────────────────────────
static constexpr uint16_t T_COUNT = 1000;
static uint16_t tSent   = 0;
static uint16_t tEchoed = 0;
static uint32_t tStart  = 0;

// RTT histogram buckets (ms): <0.5, 0.5-1, 1-2, 2-5, 5-10, 10-20, 20-50, >50
static uint32_t rttBucket[8] = {};
static uint32_t rttSendTime[T_COUNT] = {};  // send timestamp per seq

static void rttRecord(uint16_t seq, uint32_t rxTime) {
    if (seq >= T_COUNT) return;
    uint32_t rtt = rxTime - rttSendTime[seq];
    uint8_t b;
    if      (rtt < 1)   b = 0;
    else if (rtt < 2)   b = 1;
    else if (rtt < 3)   b = 2;  // 0.5-1 ms at 1-ms resolution
    else if (rtt < 6)   b = 3;
    else if (rtt < 11)  b = 4;
    else if (rtt < 21)  b = 5;
    else if (rtt < 51)  b = 6;
    else                b = 7;
    rttBucket[b]++;
}

static void printRttHistogram() {
    Serial2.println(F("=== RTT Histogram ==="));
    const char* labels[] = {"<1ms","1-2ms","2-3ms","3-6ms","6-11ms","11-21ms","21-51ms",">51ms"};
    for (uint8_t i = 0; i < 8; i++) {
        Serial2.print(labels[i]); Serial2.print(F(": ")); Serial2.println(rttBucket[i]);
    }
    Serial2.print(F("Sent: ")); Serial2.print(tSent);
    Serial2.print(F("  Echoed: ")); Serial2.println(tEchoed);
}

// ── DCS-BIOS capture state ────────────────────────────────────────────────────
static uint32_t dcsStartMs       = 0;
static uint32_t dcsTotalBytes    = 0;
static uint32_t dcsTotalFrames   = 0;
static uint32_t dcsLastByteMs    = 0;
static uint32_t dcsSecBytes      = 0;
static uint32_t dcsSecFrames     = 0;
static uint32_t dcsSecMaxFrame   = 0;
static uint32_t dcsSecMinFrame   = 0xFFFFFFFF;
static uint32_t dcsSecMaxGap     = 0;
static uint32_t dcsSecStart      = 0;
static uint32_t dcsCurrentFrame  = 0;
static bool     dcsInFrame       = false;
// summary accumulators
static uint32_t sumMinRate = 0xFFFFFFFF, sumMaxRate = 0, sumRateCount = 0, sumRateTotal = 0;
static uint32_t sumMinFrame = 0xFFFFFFFF, sumMaxFrame = 0, sumFrameCount = 0, sumFrameTotal = 0;
static uint32_t sumMinGap   = 0xFFFFFFFF, sumMaxGap   = 0, sumGapCount  = 0, sumGapTotal  = 0;
static uint32_t dcsPeak100ms = 0, dcsCur100Bytes = 0, dcsCur100Start = 0;

static constexpr uint32_t DCS_FRAME_GAP_MS = 5;   // gap > 5ms = frame boundary
static constexpr uint32_t DCS_CAPTURE_MS   = 5UL * 60UL * 1000UL;  // 5 min

static void dcsResetStats(uint32_t now) {
    dcsStartMs = dcsSecStart = dcsCur100Start = now;
    dcsTotalBytes = dcsTotalFrames = 0;
    dcsSecBytes = dcsSecFrames = 0;
    dcsSecMaxFrame = 0;
    dcsSecMinFrame = 0xFFFFFFFF;
    dcsSecMaxGap = 0;
    dcsLastByteMs = now;
    dcsCurrentFrame = 0;
    dcsInFrame = false;
    sumMinRate = sumMinFrame = sumMinGap = 0xFFFFFFFF;
    sumMaxRate = sumMaxFrame = sumMaxGap = 0;
    sumRateCount = sumRateTotal = 0;
    sumFrameCount = sumFrameTotal = 0;
    sumGapCount = sumGapTotal = 0;
    dcsPeak100ms = dcsCur100Bytes = 0;
}

static void dcsProcessByte(uint8_t b, uint32_t now) {
    // relay to STM32
    Serial1.write(b);

    dcsSecBytes++;
    dcsTotalBytes++;

    // 100ms peak window
    if (now - dcsCur100Start >= 100) {
        if (dcsCur100Bytes > dcsPeak100ms) dcsPeak100ms = dcsCur100Bytes;
        dcsCur100Bytes = 0;
        dcsCur100Start = now;
    }
    dcsCur100Bytes++;

    // frame detection
    uint32_t gap = now - dcsLastByteMs;
    if (dcsInFrame && gap > DCS_FRAME_GAP_MS) {
        // frame ended
        if (dcsCurrentFrame > 0) {
            if (dcsCurrentFrame > dcsSecMaxFrame) dcsSecMaxFrame = dcsCurrentFrame;
            if (dcsCurrentFrame < dcsSecMinFrame) dcsSecMinFrame = dcsCurrentFrame;
            sumFrameCount++; sumFrameTotal += dcsCurrentFrame;
            if (dcsCurrentFrame > sumMaxFrame) sumMaxFrame = dcsCurrentFrame;
            if (dcsCurrentFrame < sumMinFrame) sumMinFrame = dcsCurrentFrame;
        }
        if (gap > dcsSecMaxGap) dcsSecMaxGap = gap;
        sumGapCount++; sumGapTotal += gap;
        if (gap > sumMaxGap) sumMaxGap = gap;
        if (gap < sumMinGap) sumMinGap = gap;
        dcsTotalFrames++;
        dcsSecFrames++;
        dcsCurrentFrame = 0;
        dcsInFrame = false;
    }
    dcsCurrentFrame++;
    dcsInFrame = true;
    dcsLastByteMs = now;
}

static void dcsSecReport(uint32_t now) {
    uint32_t elapsed = now - dcsSecStart;
    uint32_t rate = (elapsed > 0) ? (dcsSecBytes * 1000UL / elapsed) : 0;
    sumRateCount++; sumRateTotal += rate;
    if (rate > sumMaxRate) sumMaxRate = rate;
    if (rate < sumMinRate) sumMinRate = rate;

    Serial2.print(F("[DCS] "));
    Serial2.print(now / 1000); Serial2.print(F("s  "));
    Serial2.print(rate); Serial2.print(F("B/s  frames:"));
    Serial2.print(dcsSecFrames);
    if (dcsSecMinFrame < 0xFFFFFFFF) {
        Serial2.print(F("  fmin:")); Serial2.print(dcsSecMinFrame);
        Serial2.print(F(" fmax:")); Serial2.print(dcsSecMaxFrame);
    }
    Serial2.print(F("  maxgap:")); Serial2.print(dcsSecMaxGap); Serial2.println(F("ms"));

    dcsSecBytes = 0; dcsSecFrames = 0; dcsSecMaxFrame = 0;
    dcsSecMinFrame = 0xFFFFFFFF; dcsSecMaxGap = 0;
    dcsSecStart = now;
}

static void dcsSummary() {
    Serial2.println(F(""));
    Serial2.println(F("===== DCS-BIOS CAPTURE SUMMARY ====="));
    Serial2.print(F("Total bytes : ")); Serial2.println(dcsTotalBytes);
    Serial2.print(F("Total frames: ")); Serial2.println(dcsTotalFrames);
    Serial2.print(F("Peak 100ms  : ")); Serial2.print(dcsPeak100ms); Serial2.println(F(" bytes"));
    if (sumRateCount > 0) {
        Serial2.print(F("Byte rate   : min=")); Serial2.print(sumMinRate);
        Serial2.print(F(" avg=")); Serial2.print(sumRateTotal / sumRateCount);
        Serial2.print(F(" max=")); Serial2.print(sumMaxRate); Serial2.println(F(" B/s"));
    }
    if (sumFrameCount > 0) {
        Serial2.print(F("Frame size  : min=")); Serial2.print(sumMinFrame);
        Serial2.print(F(" avg=")); Serial2.print(sumFrameTotal / sumFrameCount);
        Serial2.print(F(" max=")); Serial2.print(sumMaxFrame); Serial2.println(F(" bytes"));
    }
    if (sumGapCount > 0) {
        Serial2.print(F("Frame gap   : min=")); Serial2.print(sumMinGap);
        Serial2.print(F(" avg=")); Serial2.print(sumGapTotal / sumGapCount);
        Serial2.print(F(" max=")); Serial2.print(sumMaxGap); Serial2.println(F(" ms"));
    }
    Serial2.println(F("====================================="));
}

// ── diagnostic receive state machine ─────────────────────────────────────────
static uint8_t diagBuf[8];
static uint8_t diagPos = 0;

static void processDiag(const uint8_t* p) {
    switch (p[1]) {
        case DIAG_RTT: {
            uint16_t seq; memcpy(&seq, p + 2, 2);
            uint32_t rxTs; memcpy(&rxTs, p + 4, 4);
            rttRecord(seq, millis());  // use local time; master rx_ts for cross-check
            tEchoed++;
            break;
        }
        case DIAG_HB: {
            uint8_t nid = p[2], flags = p[3];
            uint16_t rxc; memcpy(&rxc, p + 4, 2);
            uint16_t esr; memcpy(&esr, p + 6, 2);
            Serial2.print(F("[HB] node=")); Serial2.print(nid);
            Serial2.print(F(" flags=0x")); Serial2.print(flags, HEX);
            Serial2.print(F(" rx=")); Serial2.print(rxc);
            Serial2.print(F(" ESR=0x")); Serial2.println(esr, HEX);
            break;
        }
        case DIAG_ERR: {
            uint8_t tec = p[2], rec = p[3], flags = p[4];
            Serial2.print(F("[ERR] TEC=")); Serial2.print(tec);
            Serial2.print(F(" REC=")); Serial2.print(rec);
            Serial2.print(F(" flags=0x")); Serial2.println(flags, HEX);
            break;
        }
    }
}

static void drainDiag() {
    // drain Serial1 RX (diagnostic responses from master)
    while (Serial1.available()) {
        uint8_t b = Serial1.read();
        if (diagPos == 0 && b != DIAG_MAGIC) continue;  // wait for header
        diagBuf[diagPos++] = b;
        if (diagPos == 8) {
            processDiag(diagBuf);
            diagPos = 0;
        }
    }
}

// ── injection helpers ─────────────────────────────────────────────────────────
static void sendPacket(const ControlPacket& pkt) {
    Serial1.write(reinterpret_cast<const uint8_t*>(&pkt), 4);
}

static void sendSeq(uint16_t seq) {
    ControlPacket p{0xFFFF, seq};
    rttSendTime[seq % T_COUNT] = millis();
    sendPacket(p);
}

// ── setup / loop ──────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(DCS_BIOS_BAUD);
    Serial1.begin(250000);  // STM32 master UART
    Serial2.begin(DEBUG_BAUD);  // debug console / analytics tap

    Serial2.println(F("CAN_Test_Arduino ready."));
    if (START_IN_DCS_CAPTURE) {
        uint32_t now = millis();
        mode = Mode::DCS_CAPTURE;
        dcsResetStats(now);
        Serial2.println(F("DCS-BIOS capture started at boot. USB Serial is fixed at 250000 baud."));
    }
    Serial2.println(F("Commands on Serial2: D=DCS-capture I=idle S=slow F=fast X=extreme T=throughput"));
}

static uint32_t lastInject = 0;
static uint32_t lastSecReport = 0;

void loop() {
    uint32_t now = millis();

    // ── command input ──────────────────────────────────────────────────────
    if (Serial2.available()) {
        char c = toupper(Serial2.read());
        switch (c) {
            case 'D':
                mode = Mode::DCS_CAPTURE;
                dcsResetStats(now);
                Serial2.println(F("[D] DCS-BIOS capture started. USB Serial is fixed at 250000 baud."));
                break;
            case 'I':
                mode = Mode::IDLE;
                Serial2.println(F("[I] Idle."));
                break;
            case 'S':
                mode = Mode::SLOW; sweepIdx = 0;
                Serial2.println(F("[S] Slow sweep (100ms/pkt)."));
                break;
            case 'F':
                mode = Mode::FAST; sweepIdx = 0;
                Serial2.println(F("[F] Fast burst (1.6ms/pkt)."));
                break;
            case 'X':
                mode = Mode::EXTREME; sweepIdx = 0;
                Serial2.println(F("[X] Extreme burst (no delay)."));
                break;
            case 'T':
                mode = Mode::THROUGHPUT;
                tSent = tEchoed = 0; tStart = now;
                memset(rttBucket, 0, sizeof(rttBucket));
                Serial2.println(F("[T] Throughput test (1000 SEQ)."));
                break;
        }
        lastInject = now;
    }

    // ── DCS-BIOS capture relay ─────────────────────────────────────────────
    if (mode == Mode::DCS_CAPTURE) {
        while (Serial.available()) {
            dcsProcessByte((uint8_t)Serial.read(), now);
        }
        if (now - lastSecReport >= 1000) {
            dcsSecReport(now);
            lastSecReport = now;
        }
        if (now - dcsStartMs >= DCS_CAPTURE_MS) {
            dcsSummary();
            dcsResetStats(now);
        }
        drainDiag();
        return;
    }

    drainDiag();

    // ── injection ──────────────────────────────────────────────────────────
    uint32_t interval = (mode == Mode::SLOW) ? 100 :
                        (mode == Mode::FAST) ? 2 : 0;

    if (mode == Mode::THROUGHPUT) {
        if (tSent < T_COUNT) {
            sendSeq(tSent++);
        } else if (tEchoed == T_COUNT || (now - tStart > 10000)) {
            printRttHistogram();
            mode = Mode::IDLE;
        }
        return;
    }

    if (mode == Mode::IDLE) return;

    if (interval == 0 || (now - lastInject >= interval)) {
        sendPacket(sweepTable[sweepIdx]);
        sweepIdx = (sweepIdx + 1) % SWEEP_LEN;
        lastInject = now;
    }
}
