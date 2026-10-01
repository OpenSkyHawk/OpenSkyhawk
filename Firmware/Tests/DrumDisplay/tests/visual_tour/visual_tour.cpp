// DrumDisplay — visual tour of every supported readout configuration (#137)
//
// Eyeball test, not an assertion test: it walks the descriptor's option set on a real panel so a
// human can confirm each one LOOKS right. Every stage holds for STAGE_MS and announces itself on
// DiagSerial, so the serial log says what you are looking at.
//
// Covered: both fonts, large vs compact digit geometry, leading-zero Keep and Suppress, grouping,
// a decimal glyph, all three flag scales (E/W and N/S at half scale, MagVar at full scale), both
// scroll styles through a big jump, and a runtime offset shift.
//
// Hardware: one SH1106 128x64 @ 0x3C on J_I2C2 (PB10/PB11 — see platformio.ini). No mux, no CAN.
//   upload: ~/.platformio/penv/bin/pio run -d Firmware/Tests/DrumDisplay -e visual_tour -t upload

#include <Arduino.h>
#include <Wire.h>
#include <STM32Board.h>
#include <DrumDisplay.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

static constexpr uint32_t STAGE_MS = 10000;   // hold per stage

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// What DCS puts on the wire for a clean drum digit: floor(d/10 x 65535).
static const uint16_t DIGIT_WORD[10] = {
    0, 6553, 13107, 19660, 26214, 32767, 39321, 45874, 52428, 58981,
};

// ── descriptors, one per configuration on show ────────────────────────────────

// 3 digits, big cells — the APN-153 ground-speed readout.
static const DrumSource SPEED_SRC[] = {
    { A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_X00_AM, 1, 2 },
    { A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_0X0_AM, 1, 1 },
    { A_4E_C_APN153_SPEED_00X, A_4E_C_APN153_SPEED_00X_AM, 1, 0 },
};
static const DrumReadout SPEED_BIG = {
    .sources = SPEED_SRC, .nSources = 3, .nDigits = 3,
    .digitWidthMm = 7.0f, .digitHeightMm = 11.0f, .interDigitGapMm = 1.5f,
};
static const DrumReadout SPEED_JUMP_EASE = {
    .sources = SPEED_SRC, .nSources = 3, .nDigits = 3,
    .digitWidthMm = 7.0f, .digitHeightMm = 11.0f, .interDigitGapMm = 1.5f,
    .scroll = DrumScroll::EASE_ONLY,
};
static const DrumReadout SPEED_JUMP_SNAP = {
    .sources = SPEED_SRC, .nSources = 3, .nDigits = 3,
    .digitWidthMm = 7.0f, .digitHeightMm = 11.0f, .interDigitGapMm = 1.5f,
    .scroll = DrumScroll::SNAP_SETTLE, .snapThreshold = 3.0f,
};

// 5 digits + the E/W hemisphere at HALF scale — current longitude.
static const DrumSource LON_SRC[] = {
    { A_4E_C_NAV_CURPOS_LON_X00000, A_4E_C_NAV_CURPOS_LON_X00000_AM, 1, 4 },
    { A_4E_C_NAV_CURPOS_LON_0X0000, A_4E_C_NAV_CURPOS_LON_0X0000_AM, 1, 3 },
    { A_4E_C_NAV_CURPOS_LON_00X000, A_4E_C_NAV_CURPOS_LON_00X000_AM, 1, 2 },
    { A_4E_C_NAV_CURPOS_LON_000X00, A_4E_C_NAV_CURPOS_LON_000X00_AM, 1, 1 },
    { A_4E_C_NAV_CURPOS_LON_0000X0, A_4E_C_NAV_CURPOS_LON_0000X0_AM, 1, 0 },
};
static const DrumFlag EW_FLAG = {
    .enabled = true, .address = A_4E_C_NAV_CURPOS_LON_00000X,
    .mask = A_4E_C_NAV_CURPOS_LON_00000X_AM, .faces = "WE",
    .atVisualCol = 5, .widthMm = 5.5f, .steps = 2,
};
static const DrumReadout LON_KEEP = {
    .sources = LON_SRC, .nSources = 5, .nDigits = 5,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
    .flag = EW_FLAG,
};
static const DrumReadout LON_SUPPRESS = {
    .sources = LON_SRC, .nSources = 5, .nDigits = 5,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
    .flag = EW_FLAG, .leadingZero = LeadingZero::Suppress,
};
// Same digits, grouped 3+2 with a wider gap — shows groupGapMm / groupSize. The boundary is
// counted from the left, so the extra space lands after the third digit.
static const DrumReadout LON_GROUPED = {
    .sources = LON_SRC, .nSources = 5, .nDigits = 5,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
    .groupGapMm = 3.0f, .groupSize = 3,
    .flag = EW_FLAG,
};

// 4 digits + N/S at half scale — current latitude.
static const DrumSource LAT_SRC[] = {
    { A_4E_C_NAV_CURPOS_LAT_X0000, A_4E_C_NAV_CURPOS_LAT_X0000_AM, 1, 3 },
    { A_4E_C_NAV_CURPOS_LAT_0X000, A_4E_C_NAV_CURPOS_LAT_0X000_AM, 1, 2 },
    { A_4E_C_NAV_CURPOS_LAT_00X00, A_4E_C_NAV_CURPOS_LAT_00X00_AM, 1, 1 },
    { A_4E_C_NAV_CURPOS_LAT_000X0, A_4E_C_NAV_CURPOS_LAT_000X0_AM, 1, 0 },
};
static const DrumReadout LAT_READOUT = {
    .sources = LAT_SRC, .nSources = 4, .nDigits = 4,
    .digitWidthMm = 5.0f, .digitHeightMm = 9.0f, .interDigitGapMm = 1.0f,
    .flag = { .enabled = true, .address = A_4E_C_NAV_CURPOS_LAT_0000X,
              .mask = A_4E_C_NAV_CURPOS_LAT_0000X_AM, .faces = "NS",
              .atVisualCol = 4, .widthMm = 5.5f, .steps = 2 },
};

// 4 digits + E/W at FULL scale, with a decimal glyph — MagVar.
static const DrumSource MAGVAR_SRC[] = {
    { A_4E_C_ASN41_MAGVAR_X0000, A_4E_C_ASN41_MAGVAR_X0000_AM, 1, 3 },
    { A_4E_C_ASN41_MAGVAR_0X000, A_4E_C_ASN41_MAGVAR_0X000_AM, 1, 2 },
    { A_4E_C_ASN41_MAGVAR_00X00, A_4E_C_ASN41_MAGVAR_00X00_AM, 1, 1 },
    { A_4E_C_ASN41_MAGVAR_000X0, A_4E_C_ASN41_MAGVAR_000X0_AM, 1, 0 },
};
static const DrumGlyph MAGVAR_DOT[] = { { '.', 3, 1.8f } };
static const DrumReadout MAGVAR_READOUT = {
    .sources = MAGVAR_SRC, .nSources = 4, .nDigits = 4,
    .digitWidthMm = 5.0f, .digitHeightMm = 9.0f, .interDigitGapMm = 1.0f,
    .glyphs = MAGVAR_DOT, .nGlyphs = 1,
    .flag = { .enabled = true, .address = A_4E_C_ASN41_MAGVAR_0000X,
              .mask = A_4E_C_ASN41_MAGVAR_0000X_AM, .faces = "WE",
              .atVisualCol = 4, .widthMm = 5.5f, .steps = 1 },
};

DrumDisplay speedBig (oled, SPEED_BIG,       DrumFont::LARGE);
DrumDisplay speedEase(oled, SPEED_JUMP_EASE, DrumFont::LARGE);
DrumDisplay speedSnap(oled, SPEED_JUMP_SNAP, DrumFont::LARGE);
DrumDisplay lonKeep  (oled, LON_KEEP,        DrumFont::LARGE);
DrumDisplay lonSupp  (oled, LON_SUPPRESS,    DrumFont::LARGE);
DrumDisplay lonGroup (oled, LON_GROUPED,     DrumFont::SMALL);
DrumDisplay latDisp  (oled, LAT_READOUT,     DrumFont::LARGE);
DrumDisplay magDisp  (oled, MAGVAR_READOUT,  DrumFont::LARGE);

// ── helpers ───────────────────────────────────────────────────────────────────

static void banner(const char* what) {
    auto& d = STM32Board::diagSerial();
    d.print(F("\n--- ")); d.print(what); d.println(F(" ---"));
}

// Spell `value` across `n` addresses, most significant first.
static void sendNumber(DrumDisplay& dd, const uint16_t* addrs, uint8_t n, long value) {
    for (int8_t i = n - 1; i >= 0; i--) {
        dd.onControlPacket(addrs[i], DIGIT_WORD[value % 10]);
        value /= 10;
    }
}

// Hold a stage, stepping `value` once per `stepMs` so the cascade is visible.
static void runStage(DrumDisplay& dd, const uint16_t* addrs, uint8_t n,
                     long from, long step, uint32_t stepMs) {
    const uint32_t t0 = millis();
    uint32_t last = 0;
    long v = from;
    while ((uint32_t)(millis() - t0) < STAGE_MS) {
        if (millis() - last >= stepMs) {
            last = millis();
            sendNumber(dd, addrs, n, v);
            v += step;
            if (v < 0) v = 0;
        }
        dd.update();
    }
}

static const uint16_t SPEED_ADDR[] = {
    A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_00X,
};
static const uint16_t LON_ADDR[] = {
    A_4E_C_NAV_CURPOS_LON_X00000, A_4E_C_NAV_CURPOS_LON_0X0000, A_4E_C_NAV_CURPOS_LON_00X000,
    A_4E_C_NAV_CURPOS_LON_000X00, A_4E_C_NAV_CURPOS_LON_0000X0,
};
static const uint16_t LAT_ADDR[] = {
    A_4E_C_NAV_CURPOS_LAT_X0000, A_4E_C_NAV_CURPOS_LAT_0X000,
    A_4E_C_NAV_CURPOS_LAT_00X00, A_4E_C_NAV_CURPOS_LAT_000X0,
};
static const uint16_t MAGVAR_ADDR[] = {
    A_4E_C_ASN41_MAGVAR_X0000, A_4E_C_ASN41_MAGVAR_0X000,
    A_4E_C_ASN41_MAGVAR_00X00, A_4E_C_ASN41_MAGVAR_000X0,
};

// Hold a flag-bearing stage, rolling the digits and flipping the flag halfway.
static void runFlagStage(DrumDisplay& dd, const uint16_t* addrs, uint8_t n, long from,
                         uint16_t flagAddr, uint16_t faceLo, uint16_t faceHi) {
    const uint32_t t0 = millis();
    uint32_t last = 0;
    bool hi = false;
    long v = from;
    dd.onControlPacket(flagAddr, faceLo);
    while ((uint32_t)(millis() - t0) < STAGE_MS) {
        if (millis() - last >= 700) {
            last = millis();
            sendNumber(dd, addrs, n, v);
            v += 1;
            if ((millis() - t0) > STAGE_MS / 2 && !hi) {   // flip the flag once, mid-stage
                hi = true;
                dd.onControlPacket(flagAddr, faceHi);
            }
        }
        dd.update();
    }
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay visual_tour ==="));
    d.println(F("Watch the panel; each stage holds 10 s."));

    Wire.setSCL(I2C_TEST_SCL);
    Wire.setSDA(I2C_TEST_SDA);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();

    speedBig.configure(); speedEase.configure(); speedSnap.configure();
    lonKeep.configure();  lonSupp.configure();   lonGroup.configure();
    latDisp.configure();  magDisp.configure();
}

void loop() {
    banner("1/10  LARGE font, 3 big digits, rolling 240+");
    speedBig.setFontSize(DrumFont::LARGE);
    speedBig.configure();
    runStage(speedBig, SPEED_ADDR, 3, 240, 1, 600);

    banner("2/10  SMALL font, same readout (runtime setFontSize)");
    speedBig.setFontSize(DrumFont::SMALL);
    runStage(speedBig, SPEED_ADDR, 3, 240, 1, 600);
    speedBig.setFontSize(DrumFont::LARGE);

    banner("3/10  Offset shift (setOffset +4 mm x, -2 mm y)");
    speedBig.setOffset(4.0f, -2.0f);
    runStage(speedBig, SPEED_ADDR, 3, 240, 1, 600);
    speedBig.setOffset(0.0f, 0.0f);

    banner("4/10  Leading zeros KEPT — 5 digits, value 42 → 00042");
    runStage(lonKeep, LON_ADDR, 5, 40, 1, 900);

    banner("5/10  Leading zeros SUPPRESSED — same value → 42");
    runStage(lonSupp, LON_ADDR, 5, 40, 1, 900);

    banner("6/10  Grouped 3+2 with a wider gap, SMALL font");
    runStage(lonGroup, LON_ADDR, 5, 12340, 1, 900);

    banner("7/10  E/W flag at HALF scale — flips W→E mid-stage");
    runFlagStage(lonGroup, LON_ADDR, 5, 12340, A_4E_C_NAV_CURPOS_LON_00000X, 0, 32767);

    banner("8/10  N/S flag at HALF scale — latitude, flips N→S");
    runFlagStage(latDisp, LAT_ADDR, 4, 3240, A_4E_C_NAV_CURPOS_LAT_0000X, 0, 32767);

    banner("9/10  MagVar: decimal glyph + FULL-scale flag, flips W→E");
    runFlagStage(magDisp, MAGVAR_ADDR, 4, 1230, A_4E_C_ASN41_MAGVAR_0000X, 0, 65535);

    banner("10/10 Big jump 130→250: EASE_ONLY then SNAP_SETTLE");
    sendNumber(speedEase, SPEED_ADDR, 3, 130);
    for (uint32_t t = millis(); millis() - t < 1500;) speedEase.update();
    sendNumber(speedEase, SPEED_ADDR, 3, 250);
    for (uint32_t t = millis(); millis() - t < STAGE_MS / 2;) speedEase.update();

    sendNumber(speedSnap, SPEED_ADDR, 3, 130);
    for (uint32_t t = millis(); millis() - t < 1500;) speedSnap.update();
    sendNumber(speedSnap, SPEED_ADDR, 3, 250);
    for (uint32_t t = millis(); millis() - t < STAGE_MS / 2;) speedSnap.update();
}
