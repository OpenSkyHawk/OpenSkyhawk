// SimGateway — HID enumeration and axis sweep test
//
// Purpose: verify the TinyUSB custom HID descriptor enumerates correctly as a
// DirectInput joystick and all 8 axes + 2 buttons + 1 hat are visible in
// Windows joy.cpl, DIView, and DCS World controls.
//
// HOW TO USE:
//   1. Flash:  pio run -e test_hid_enumeration -t upload
//   2. Device enumerates as: OpenSkyhawk / A-4E Skyhawk (VID 0x2E8A / PID 0x4134)
//   3. Open joy.cpl (or DIView) — verify joystick appears with 8 axes, 128 buttons, 4 hats.
//   4. Open DCS World → Options → Controls → any aircraft — verify all axes are bindable.
//   5. Watch Serial (115200) for which axis/button is currently active.
//
// Sweep pattern (one active at a time, 1.5 s each):
//   Axes 0–7 swept full range (min → 0 → max → 0)
//   Button 0 (CTRL_TRIGGER) pressed/released
//   Hat 0 swept N → NE → E → … → NW → center
//
// No UART, no PanelBridge, no CAN required. Feeds HID frames directly via feedByte().
//
// Flash:
//   pio run -e test_hid_enumeration -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>
#include <HIDControls.h>

// Axis sweep — one controlId per axis, in index order
static const uint16_t AXIS_CTRL_IDS[] = {
    CTRL_ROLL,
    CTRL_PITCH,
    CTRL_THROTTLE,
    CTRL_RUDDER,
    CTRL_BRAKE_L,
    CTRL_BRAKE_R,
    CTRL_ZOOM,
    0x0017, // reserved axis slot 7 — tests descriptor coverage
};
static const char* AXIS_NAMES[] = {
    "CTRL_ROLL (axis 0)",
    "CTRL_PITCH (axis 1)",
    "CTRL_THROTTLE (axis 2)",
    "CTRL_RUDDER (axis 3)",
    "CTRL_BRAKE_L (axis 4)",
    "CTRL_BRAKE_R (axis 5)",
    "CTRL_ZOOM (axis 6)",
    "reserved slot 7",
};

// HID controls declared at file scope — self-register before setup()
OpenSkyhawk::HIDAxis axisRoll    (CTRL_ROLL,    0);
OpenSkyhawk::HIDAxis axisPitch   (CTRL_PITCH,   1);
OpenSkyhawk::HIDAxis axisThrottle(CTRL_THROTTLE,2);
OpenSkyhawk::HIDAxis axisRudder  (CTRL_RUDDER,  3);
OpenSkyhawk::HIDAxis axisBrakeL  (CTRL_BRAKE_L, 4);
OpenSkyhawk::HIDAxis axisBrakeR  (CTRL_BRAKE_R, 5);
OpenSkyhawk::HIDAxis axisZoom    (CTRL_ZOOM,    6);
OpenSkyhawk::HIDAxis axisSlot7   (0x0017,       7); // tests 8th axis slot
OpenSkyhawk::HIDButton trigger   (CTRL_TRIGGER, 0);
OpenSkyhawk::HIDHatSwitch hat0   (CTRL_HAT_0,   0);

// Helper: inject one 6-byte HID frame via feedByte() (SIMGATEWAY_TEST not defined,
// but feedByte() is not available — drive via simulated UART loopback instead).
// In this env we do NOT define SIMGATEWAY_TEST, so we build the real TinyUSB path.
// Drive axes by writing HID frames to Serial1 TX (GP0) looped to Serial1 RX (GP1).
// NOTE: Requires GP0→GP1 jumper for loopback. Without the jumper, axes still sweep
// inside the HID report (no parser dispatch) — device still enumerates for UI testing.

static void sendFrame(HardwareSerial& uart, uint16_t controlId, uint16_t value) {
    const uint8_t frame[] = {
        0xAA, 0x55,
        (uint8_t)(controlId & 0xFF), (uint8_t)(controlId >> 8),
        (uint8_t)(value     & 0xFF), (uint8_t)(value     >> 8)
    };
    uart.write(frame, sizeof(frame));
    uart.flush();
}

void setup() {
    Serial.begin(115200);

    // Initialise the full SimGateway (USB identity + TinyUSB HID + UART)
    SimGateway::setup(Serial1);

    // Wait a moment for CDC to attach (USB CDC may need a moment after HID enumeration)
    delay(3000);

    Serial.println(F("=== hid_enumeration — TinyUSB axis sweep ==="));
    Serial.println(F("Device: OpenSkyhawk / A-4E Skyhawk"));
    Serial.println(F("VID 0x2E8A  PID 0x4134"));
    Serial.println(F("Descriptor: 8 axes, 128 buttons, 4 hat switches"));
    Serial.println(F(""));
    Serial.println(F("Check joy.cpl / DIView / DCS Controls to verify."));
    Serial.println(F("Requires GP0->GP1 jumper for axis sweep; device enumerates either way."));
    Serial.println(F(""));
}

void loop() {
    static uint8_t phase = 0;
    static uint32_t lastPhase = 0;
    static const uint32_t PHASE_MS = 1500;

    uint32_t now = millis();
    if ((now - lastPhase) < PHASE_MS) {
        SimGateway::loop(); // drain any loopback bytes
        return;
    }
    lastPhase = now;

    // 8 axis phases + 1 button phase + 9 hat phases = 18 phases total
    if (phase < 8) {
        // Sweep axis: min → centre → max → centre
        static uint8_t axisStep = 0;
        static const uint16_t AXIS_VALS[] = {0x0000, 0x8000, 0xFFFF, 0x8000};
        uint16_t v = AXIS_VALS[axisStep % 4];
        sendFrame(Serial1, AXIS_CTRL_IDS[phase], v);

        Serial.print(F("[axis] "));
        Serial.print(AXIS_NAMES[phase]);
        Serial.print(F("  value=0x"));
        Serial.println(v, HEX);

        axisStep++;
        if (axisStep % 4 == 0) { axisStep = 0; phase++; }

    } else if (phase == 8) {
        // Button 0 press
        sendFrame(Serial1, CTRL_TRIGGER, 1);
        Serial.println(F("[btn]  CTRL_TRIGGER pressed"));
        phase++;

    } else if (phase == 9) {
        // Button 0 release
        sendFrame(Serial1, CTRL_TRIGGER, 0);
        Serial.println(F("[btn]  CTRL_TRIGGER released"));
        phase++;

    } else {
        // Hat 0 sweep: center → N → NE → E → SE → S → SW → W → NW → center
        static uint8_t hatDir = 0;
        static const char* HAT_NAMES[] = { "center", "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
        sendFrame(Serial1, CTRL_HAT_0, hatDir);

        Serial.print(F("[hat]  hat0 direction="));
        Serial.println(HAT_NAMES[hatDir]);

        hatDir++;
        if (hatDir > 8) {
            hatDir = 0;
            phase = 0; // restart sweep
            Serial.println(F("--- sweep complete, restarting ---"));
            Serial.println();
        }
    }

    SimGateway::loop();
}
