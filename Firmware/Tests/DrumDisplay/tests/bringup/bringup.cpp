// DrumDisplay — OLED bring-up (ported from the OledBringup prototype).
//
// 1.3" I2C OLED @ 0x3C on STM32F103C8. I2C1 remapped PB8 = SCL, PB9 = SDA. Draws a title +
// a big 000–999 counter so you can confirm a panel works and eyeball the digit rendering.
//
// @note 1.3" modules are almost always SH1106 (132x64 RAM, 2-px column offset). If the image is
//       shifted ~2 px or garbled, switch to the SSD1306 constructor below.

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

static uint16_t value = 0;       // rolling 000–999 demo value
static uint32_t lastTick = 0;    // last update time (ms)

void setup() {
    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    oled.setContrast(255);
}

void loop() {
    if (millis() - lastTick < 150) return;  // ~6.7 Hz tick
    lastTick = millis();
    value = (value + 1) % 1000;

    oled.clearBuffer();
    oled.setFont(u8g2_font_6x12_tr);
    oled.drawStr(0, 9, "OpenSkyhawk OLED test");
    oled.drawHLine(0, 12, 128);

    char buf[4];
    snprintf(buf, sizeof(buf), "%03u", value);
    oled.setFont(u8g2_font_logisoso32_tn);   // big, numbers-only
    oled.setFontPosTop();
    int w = oled.getStrWidth(buf);
    oled.drawStr((128 - w) / 2, 20, buf);
    oled.sendBuffer();
}
