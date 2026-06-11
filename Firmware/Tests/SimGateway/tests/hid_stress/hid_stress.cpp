/**
 * @file hid_stress.cpp
 * @brief TinyUSB HID stress test — beyond-production descriptor, no SimGateway library.
 *
 * Descriptor: 16 axes (all Generic Desktop — 9 standard + 7 velocity), 128 buttons, 8 hats.
 * Report: 52 bytes — fits within TinyUSB CFG_TUD_HID_EP_BUFSIZE=64 limit.
 * Purpose: probe how many controls each platform (Mac browser, Windows joy.cpl, DCS)
 * actually surfaces. Platforms bound by DIJOYSTATE2 (8 axes / 128 buttons / 4 hats)
 * will clamp; Mac Gamepad API may surface all 16 GD axes and 8 hats.
 *
 * Axes 0–7  : X/Y/Z/Rx/Ry/Rz/Slider/Dial  (GD 0x30–0x37)
 * Axes 8–15 : Wheel/Vx/Vy/Vz/Vbrx/Vbry/Vbrz/Vno  (GD 0x38, 0x40–0x46)
 * All 16 on GD Usage Page — Chrome/IOKit should surface all vs. ignoring vendor page.
 *
 * Note: 256-button variant requires CFG_TUD_HID_EP_BUFSIZE=128 in tusb_config_rp2040.h
 * (pio package file — resets on pkg update). 128 buttons stays within default limit.
 *
 * Auto-sweeps three modes in rotation:
 *   AXES    (10 s) — all 16 axes run independent sine waves at different periods
 *   BUTTONS (13 s) — cycles through all 128 buttons sequentially, 100 ms each
 *   HATS    ( 8 s) — all 8 hats cycle through 9 positions with per-hat phase offset
 *
 * No UART. No loopback jumper. No SimGateway library. No HIDAxis/HIDButton/HIDHatSwitch.
 * Platform: RP2040 only (TinyUSB).
 *
 * Flash:   pio run -e test_hid_stress -t upload   (hold BOOTSEL)
 * Monitor: USB CDC at 115200 baud
 * Test:    hardwaretester.com/gamepad (Mac / browser), joy.cpl (Windows), DCS Controls
 *
 * VID 0x2E8A / PID 0x4135 ("A-4E Stress") — different PID from production to avoid
 * OS device-cache conflicts when flashing both.
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <math.h>

namespace {

// ── Report struct ─────────────────────────────────────────────────────────────
// Layout must match the descriptor exactly (same order, same bit widths):
//   axes[0..7]    → X/Y/Z/Rx/Ry/Rz/Slider/Dial  (GD 0x30–0x37, 16 bytes)
//   axes[8..15]   → Wheel/Vx/Vy/Vz/Vbrx/Vbry/Vbrz/Vno  (GD 0x38+0x40–0x46, 16 bytes)
//   buttons[0..15]→ 128 buttons, 1-bit each (16 bytes)
//   hats[0..3]    → 8 hat nibbles, 4-bit each (4 bytes) — 0xF = null/centered
// Total: 52 bytes — fits within TinyUSB default CFG_TUD_HID_EP_BUFSIZE=64
struct __attribute__((packed)) StressReport {
    int16_t axes[16];
    uint8_t buttons[16];
    uint8_t hats[4];
};

// ── HID descriptor ────────────────────────────────────────────────────────────
static const uint8_t kDescriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x04,        // Usage (Gamepad) — Chrome Gamepad API filters on this; 0x04=Joystick not reliably picked up
    0xA1, 0x01,        // Collection (Application)

    // 8 standard axes — X Y Z Rx Ry Rz Slider Dial (Generic Desktop)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x09, 0x35,        //   Usage (Rz)
    0x09, 0x36,        //   Usage (Slider)
    0x09, 0x37,        //   Usage (Dial)
    0x16, 0x00, 0x80,  //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    // 8 more Generic Desktop axes — Wheel + 7 velocity usages
    // All on GD page (0x01) so Chrome/IOKit maps them; vendor page (0xFF00) is ignored by most hosts.
    // 0x38=Wheel, 0x40=Vx, 0x41=Vy, 0x42=Vz, 0x43=Vbrx, 0x44=Vbry, 0x45=Vbrz, 0x46=Vno
    0x05, 0x01,        //   Usage Page (Generic Desktop) — re-state after previous section
    0x09, 0x38,        //   Usage (Wheel)
    0x09, 0x40,        //   Usage (Vx)
    0x09, 0x41,        //   Usage (Vy)
    0x09, 0x42,        //   Usage (Vz)
    0x09, 0x43,        //   Usage (Vbrx)
    0x09, 0x44,        //   Usage (Vbry)
    0x09, 0x45,        //   Usage (Vbrz)
    0x09, 0x46,        //   Usage (Vno)
    0x16, 0x00, 0x80,  //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    // 128 buttons (Button page, usages 1–128)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x80,        //   Usage Maximum (128)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x80,        //   Report Count (128)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    // 8 hat switches (Generic Desktop Hat Switch, 4-bit each, 0xF = null)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x39,        //   Usage (Hat Switch) — hat 0
    0x09, 0x39,        //   Usage (Hat Switch) — hat 1
    0x09, 0x39,        //   Usage (Hat Switch) — hat 2
    0x09, 0x39,        //   Usage (Hat Switch) — hat 3
    0x09, 0x39,        //   Usage (Hat Switch) — hat 4
    0x09, 0x39,        //   Usage (Hat Switch) — hat 5
    0x09, 0x39,        //   Usage (Hat Switch) — hat 6
    0x09, 0x39,        //   Usage (Hat Switch) — hat 7
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0 degrees)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315 degrees)
    0x65, 0x14,        //   Unit (SI Rotation: Degrees)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x42,        //   Input (Data, Variable, Absolute, Null State)

    0xC0               // End Collection
};

static StressReport _report = {};
static Adafruit_USBD_HID _hid(kDescriptor, sizeof(kDescriptor),
                               HID_ITF_PROTOCOL_NONE, 1, false);

// ── Report helpers ─────────────────────────────────────────────────────────────

static void clearReport() {
    memset(&_report, 0, sizeof(_report));
    _report.hats[0] = 0xFF; // 0xF per nibble = all hats centered
    _report.hats[1] = 0xFF;
    _report.hats[2] = 0xFF;
    _report.hats[3] = 0xFF;
}

static void setAxis(uint8_t idx, int16_t val) {
    if (idx < 16) _report.axes[idx] = val;
}

static void setButton(uint8_t idx, bool on) {
    if (idx >= 128) return; // 128 buttons, indices 0–127
    uint8_t b = idx >> 3;
    uint8_t m = 1u << (idx & 7u);
    if (on) _report.buttons[b] |=  m;
    else    _report.buttons[b] &= ~m;
}

static void setHat(uint8_t idx, uint8_t dir) {
    // dir: 0 = centered, 1 = N, 2 = NE, …, 8 = NW; >8 = centered
    // HID nibble: 0x0 = N, 0x1 = NE, …, 0x7 = NW, 0xF = null (centered)
    if (idx >= 8) return;
    uint8_t nib      = (dir == 0 || dir > 8) ? 0xFu : (uint8_t)(dir - 1u);
    uint8_t byte_idx = idx >> 1;
    if ((idx & 1u) == 0)
        _report.hats[byte_idx] = (_report.hats[byte_idx] & 0xF0u) | (nib & 0x0Fu);
    else
        _report.hats[byte_idx] = (_report.hats[byte_idx] & 0x0Fu) | ((nib & 0x0Fu) << 4);
}

static bool sendReport() {
    if (!_hid.ready()) return false;
    return _hid.sendReport(0, &_report, sizeof(_report));
}

} // anonymous namespace

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    // USB identity — set before TinyUSB enumerates.
    // PID 0x4135 (stress) vs 0x4134 (production) avoids host cache conflicts.
    TinyUSBDevice.setID(0x2E8A, 0x4135);
    TinyUSBDevice.setManufacturerDescriptor("OpenSkyhawk");
    TinyUSBDevice.setProductDescriptor("A-4E Stress");

    clearReport();
    _hid.begin();

    // Wait up to 2 s for host enumeration (handles benchtop use without a host).
    uint32_t t0 = millis();
    while (!TinyUSBDevice.mounted() && (millis() - t0) < 2000) delay(1);

    delay(2000); // give CDC a moment to attach

    // USR button (GP24, active-low) → button 0 manual trigger.
    // YD-RP2040: USR = GP24, WS2812 = GP23. No simple onboard LED.
    pinMode(24, INPUT_PULLUP);

    // Browser Gamepad API requires at least one button press before it reports the device
    // as connected. Press+release button 0 once here so the page detects on load.
    setButton(0, true);  sendReport(); delay(50);
    setButton(0, false); sendReport();

    Serial.println(F("=== hid_stress — TinyUSB oversized descriptor ==="));
    Serial.println(F("VID 0x2E8A  PID 0x4135  Product: A-4E Stress"));
    Serial.println(F("Descriptor: 16 axes (all GD), 128 buttons, 8 hats"));
    Serial.println(F("  axes 0–7  : X/Y/Z/Rx/Ry/Rz/Slider/Dial  (GD 0x30–0x37)"));
    Serial.println(F("  axes 8–15 : Wheel/Vx/Vy/Vz/Vbrx/Vbry/Vbrz/Vno  (GD 0x38+0x40–0x46)"));
    Serial.println(F("  buttons   : 128 — DirectInput caps at 128; Chrome Gamepad API caps at 32"));
    Serial.println(F("  hats      : 8   — DirectInput caps at 4;   Mac/browser maps as extra axes"));
    Serial.println(F(""));
    Serial.println(F("Modes: AXES (10s) -> BUTTONS (26s) -> HATS (8s) -> repeat"));
    Serial.println(F("Open gamepad-tester.com, joy.cpl, or DCS Controls to observe."));
    Serial.println(F(""));
    Serial.println(F("--- MODE: AXES ---"));
    Serial.println(F("16 axes oscillating at independent frequencies."));
    Serial.println(F("Axis values printed every 2 s. Count how many your tool shows."));
    Serial.println(F(""));
}

// ── Loop ──────────────────────────────────────────────────────────────────────

enum class Mode : uint8_t { AXES, BUTTONS, HATS };

void loop() {
    static Mode     mode       = Mode::AXES;
    static uint32_t modeStart  = 0;
    static bool     firstRun   = true;
    static uint32_t lastPrint  = 0;
    static uint32_t lastStep   = 0;
    static uint8_t  btnIdx     = 0;
    static int16_t  prevBtn    = -1;
    static uint32_t lastHat    = 0;
    static uint8_t  hatStep    = 0;

    uint32_t now = millis();

    if (firstRun) {
        firstRun  = false;
        modeStart = now;
        lastPrint = now;
        lastStep  = now;
        lastHat   = now;
    }

    // ── USR button (GP24, active-low) → button 0 ─────────────────────────────
    {
        static bool prevUsr = false;
        bool usr = !digitalRead(24); // INPUT_PULLUP set in setup(); LOW when pressed
        if (usr != prevUsr) {
            prevUsr = usr;
            setButton(0, usr);
            bool sent = sendReport();
            Serial.print(F("[USR] button 0 "));
            Serial.print(usr ? F("pressed") : F("released"));
            Serial.print(F("  hid_ready="));
            Serial.print(_hid.ready() || sent); // sent=true means ready was true when called
            Serial.print(F("  sent="));
            Serial.println(sent);
        }
    }

    // ── Mode transitions ──────────────────────────────────────────────────────
    if (mode == Mode::AXES && (now - modeStart) >= 10000UL) {
        clearReport(); sendReport();
        mode      = Mode::BUTTONS;
        modeStart = now;
        lastStep  = now;
        btnIdx    = 0;
        prevBtn   = -1;
        Serial.println(F("\n--- MODE: BUTTONS ---"));
        Serial.println(F("Cycling buttons 0–127, 100 ms each. Count how many your tool shows."));
        Serial.println(F(""));

    } else if (mode == Mode::BUTTONS && (now - modeStart) >= 12800UL) {
        clearReport(); sendReport();
        mode      = Mode::HATS;
        modeStart = now;
        lastHat   = now;
        hatStep   = 0;
        Serial.println(F("\n--- MODE: HATS ---"));
        Serial.println(F("8 hats cycling directions with per-hat phase offset."));
        Serial.println(F("Count how many your tool shows."));
        Serial.println(F(""));

    } else if (mode == Mode::HATS && (now - modeStart) >= 8100UL) {
        clearReport(); sendReport();
        mode      = Mode::AXES;
        modeStart = now;
        lastPrint = now;
        Serial.println(F("\n--- MODE: AXES ---"));
        Serial.println(F("16 axes oscillating at independent frequencies."));
        Serial.println(F(""));
    }

    // ── Mode logic ────────────────────────────────────────────────────────────

    if (mode == Mode::AXES) {
        // Each axis runs a sine wave with its own period so all 16 are always visually distinct.
        // Axis i: period = 2000 + i*200 ms (2.0 s … 5.0 s).
        float t_ms = (float)(now - modeStart);
        for (uint8_t i = 0; i < 16; i++) {
            float period = 2000.0f + (float)i * 200.0f;
            float phase  = 2.0f * (float)M_PI * t_ms / period;
            setAxis(i, (int16_t)(sinf(phase) * 32767.0f));
        }
        sendReport();

        if ((now - lastPrint) >= 2000UL) {
            lastPrint = now;
            Serial.print(F("axes: "));
            for (uint8_t i = 0; i < 16; i++) {
                Serial.print(_report.axes[i]);
                Serial.print(i < 15 ? F(", ") : F("\n"));
            }
        }

    } else if (mode == Mode::BUTTONS) {
        if ((now - lastStep) >= 100UL) {
            lastStep = now;
            if (prevBtn >= 0) setButton((uint8_t)prevBtn, false);
            setButton(btnIdx, true);
            sendReport();

            if ((btnIdx & 0x1Fu) == 0) { // print every 32 buttons
                Serial.print(F("[btn] pressing "));
                Serial.println(btnIdx);
            }

            prevBtn = (int16_t)btnIdx;
            if (++btnIdx >= 128) btnIdx = 0;
        }

    } else { // HATS
        if ((now - lastHat) >= 500UL) {
            lastHat = now;
            // hatStep cycles 0–8 (9 states: center + 8 compass points)
            if (++hatStep > 8) hatStep = 0;

            // Hat h is offset by h steps from hatStep, all cycling through 9 positions.
            for (uint8_t h = 0; h < 8; h++) {
                uint8_t dir = (hatStep + h) % 9u; // 0=center, 1-8=N/NE/…/NW
                setHat(h, dir);
            }
            sendReport();

            static const char* const kDirNames[] = {
                "center", "N", "NE", "E", "SE", "S", "SW", "W", "NW"
            };
            Serial.print(F("[hat] step="));
            Serial.print(hatStep);
            Serial.print(F("  hat0="));
            Serial.println(kDirNames[hatStep % 9]);
        }
    }
}
