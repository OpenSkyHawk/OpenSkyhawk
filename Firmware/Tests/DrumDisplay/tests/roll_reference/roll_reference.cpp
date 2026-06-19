// DrumDisplay — rolling-digit renderer reference (ported from the OledRoll prototype).
//
// SH1106 1.3" @ 0x3C, STM32F103C8, I2C1 remap PB8 = SCL / PB9 = SDA. This is the bench-verified
// drum renderer the DrumDisplay class lifts its math from (drawTape / drawFlag / per-place ease /
// per-cell clip). Kept as a standalone regression reference — drive it and eyeball the cascade.
//
// Each digit sits on a vertical "tape" that eases to its new value, clipped to its column so
// neighbours peek during the roll. Cascade: units rolls every step; tens/hundreds dwell and roll
// only on carry (each place eases toward target / 10^place).

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R2, U8X8_PIN_NONE);

static const float PX_PER_MM = 4.35f;
static const float CELL_H_MM = 8.0f;
static const uint8_t N_DIGITS = 5;            // longest case: LONGITUDE = 5 digits + flag
static const bool HAS_FLAG = true;
static const int  TOTAL    = N_DIGITS + (HAS_FLAG ? 1 : 0);
static const int CELLH = (int)(CELL_H_MM * PX_PER_MM + 0.5f);
static const int GAP    = 4;
static const int COLW   = 16;
static const int FLAGW  = 22;
static const int DPITCH = COLW + GAP;
static const int TOTALW = N_DIGITS * DPITCH + FLAGW;
static const int X0     = (128 - TOTALW) / 2;
static const int CY     = 32;
static const float EASE = 0.30f;

struct FontOpt { const uint8_t *font; const char *name; };
static const FontOpt FONTS[] = {
    { u8g2_font_profont22_mr, "profont22" },
    { u8g2_font_profont29_mr, "profont29" },
};
static const int N_FONTS = sizeof(FONTS) / sizeof(FONTS[0]);
static int fontIdx = 0;

static float pos[N_DIGITS] = {0};
static long  target = 0;
static uint32_t lastInc = 0, lastFrame = 0, lastFont = 0;

static const char *FACE_SETS[] = { "WE", "NS" };
static int   setIdx     = 0;
static float flagPos    = 0;
static long  flagTarget = 0;
static uint32_t lastFlag = 0, lastSet = 0;

static void drawTape(int cx, float p) {
    long c = (long)lroundf(p);
    for (long k = c - 1; k <= c + 1; k++) {
        int glyph = (int)(((k % 10) + 10) % 10);
        char s[2] = { (char)('0' + glyph), 0 };
        int gx = cx + (COLW - (int)oled.getStrWidth(s)) / 2;
        int y = CY + (int)lroundf((k - p) * CELLH);
        oled.drawStr(gx, y, s);
    }
}

static void drawFlag(int cx, float p, const char *faces, int cellw) {
    long c = (long)lroundf(p);
    for (long k = c - 1; k <= c + 1; k++) {
        int idx = (int)(((k % 2) + 2) % 2);
        char s[2] = { faces[idx], 0 };
        int gx = cx + (cellw - (int)oled.getStrWidth(s)) / 2;
        int y = CY + (int)lroundf((k - p) * CELLH);
        oled.drawStr(gx, y, s);
    }
}

void setup() {
    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
}

void loop() {
    uint32_t now = millis();
    if (now - lastInc  >= 400)  { lastInc  = now; target++; }
    if (now - lastFlag >= 2000) { lastFlag = now; flagTarget++; }
    if (now - lastSet  >= 8000) { lastSet  = now; setIdx ^= 1; }
    if (now - lastFont >= 5000) { lastFont = now; fontIdx = (fontIdx + 1) % N_FONTS; }

    if (now - lastFrame >= 16) {
        lastFrame = now;
        long place = 1;
        for (int i = 0; i < N_DIGITS; i++) {
            float step = (float)(target / place);
            pos[i] += (step - pos[i]) * EASE;
            place *= 10;
        }
        flagPos += ((float)flagTarget - flagPos) * EASE;
        oled.clearBuffer();
        oled.setFont(FONTS[fontIdx].font);
        oled.setFontPosCenter();
        for (int col = 0; col < TOTAL; col++) {
            if (col < N_DIGITS) {
                int cx = X0 + col * DPITCH;
                oled.setClipWindow(cx, CY - CELLH / 2, cx + COLW, CY + CELLH / 2);
                drawTape(cx, pos[N_DIGITS - 1 - col]);
            } else {
                int cx = X0 + N_DIGITS * DPITCH;
                oled.setClipWindow(cx, CY - CELLH / 2, cx + FLAGW, CY + CELLH / 2);
                drawFlag(cx, flagPos, FACE_SETS[setIdx], FLAGW);
            }
        }
        oled.setMaxClipWindow();
        oled.setFont(u8g2_font_5x7_tr);
        oled.setFontPosTop();
        oled.drawStr(0, 0, FONTS[fontIdx].name);
        oled.sendBuffer();
    }
}
