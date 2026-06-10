// SimGateway — RP2040
//
// USB HID (joystick) + USB CDC (DCS-BIOS relay) composite device.
// Relays raw DCS-BIOS bytes between USB CDC and UART (Serial1, GP0 TX / GP1 RX @ 250000).
// Intercepts HID frames (0xAA 0x55 magic) from UART and dispatches to HIDAxis/HIDButton list.
//
// Does NOT run the DCS-BIOS library — PanelBridge owns all DCS-BIOS parsing.
// SimGateway::setup() owns: Serial1.begin(250000), USB identity, and Joystick init.
// Only declarations needed here are HIDAxis and HIDButton objects.

#include <SimGateway.h>
#include <HIDControls.h>

// ── HID axes ─────────────────────────────────────────────────────────────────
// Declare one per joystick axis. Constructor takes (controlId, axisIndex 0–7).
// Value mapping (0–65535 → ±32767) is handled internally by the library.
//
// OpenSkyhawk::HIDAxis roll    (CTRL_ROLL,     0);
// OpenSkyhawk::HIDAxis pitch   (CTRL_PITCH,    1);
// OpenSkyhawk::HIDAxis throttle(CTRL_THROTTLE, 2);
// OpenSkyhawk::HIDAxis rudder  (CTRL_RUDDER,   3);
// OpenSkyhawk::HIDAxis brakeL  (CTRL_BRAKE_L,  4);
// OpenSkyhawk::HIDAxis brakeR  (CTRL_BRAKE_R,  5);
// OpenSkyhawk::HIDAxis zoom    (CTRL_ZOOM,     6);

// ── HID buttons ──────────────────────────────────────────────────────────────
// Declare one per button. Constructor takes (controlId, buttonIndex 0–127).
//
// OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0);

void setup() {
    SimGateway::setup(Serial1);
}

void loop() {
    SimGateway::loop();
}
