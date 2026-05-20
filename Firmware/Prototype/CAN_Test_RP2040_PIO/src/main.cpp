// NeoPixel hello-world — Waveshare RP2040-Plus
//
// Cycles the onboard WS2812 RGB LED through red → green → blue → off,
// one colour per second. Confirms PlatformIO + arduino-pico + picotool
// upload all work before writing the full injector firmware.
//
// Onboard WS2812 data pin: GP23
// Plain LED: GP25  (also toggled for a visual double-check without the RGB)
// User button: GP24 (active-low, internal pull-up)

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

static constexpr uint8_t  NEO_PIN   = 23;
static constexpr uint8_t  NEO_COUNT = 1;
static constexpr uint8_t  BRIGHTNESS = 30;   // 0–255; keep low, it's bright

static constexpr uint8_t  LED_PIN   = 25;   // plain green LED

Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

static const uint32_t COLOURS[] = {
    0xFF0000,   // red
    0x00FF00,   // green
    0x0000FF,   // blue
    0x000000,   // off
};
static constexpr uint8_t N_COLOURS = sizeof(COLOURS) / sizeof(COLOURS[0]);

void setup() {
    pinMode(LED_PIN, OUTPUT);
    strip.begin();
    strip.setBrightness(BRIGHTNESS);
    strip.show();   // initialise to off
}

void loop() {
    static uint8_t idx = 0;

    // NeoPixel — cycle colours
    strip.setPixelColor(0, COLOURS[idx]);
    strip.show();

    // Plain LED — toggle in sync so there's a visible heartbeat even if
    // the NeoPixel library has the wrong pin
    digitalWrite(LED_PIN, (idx & 1) ? HIGH : LOW);

    idx = (idx + 1) % N_COLOURS;
    delay(1000);
}
