#pragma once
#ifdef ARDUINO_ARCH_RP2040

#include <Arduino.h>
#include <Joystick.h>   // exposed so sketches can call Joystick.button() etc.
#include <CANProtocol.h>

// ── SimGateway namespace ──────────────────────────────────────────────────────
// Static singleton for the RP2040 gateway. Owns:
//   USB identity + Joystick HID composite device
//   UART link to PanelBridge (ControlPacket framing, DIAG frame parsing)
//
// DCS-BIOS setup/loop must be called by the sketch directly — they require
// #define DCSBIOS_DEFAULT_SERIAL before #include <DcsBios.h>, which must be
// in the sketch's translation unit.
//
// Application code (DcsBios::IntegerBuffer callbacks, NeoPixel, custom HID
// button wiring) stays in the sketch — SimGateway is infrastructure only.

namespace SimGateway {
    // panelBridgePort: UART connected to PanelBridge STM32 (GP0/GP1 = Serial1)
    // Sets USB identity, inits Joystick, starts UART at 250000 baud.
    // Call DcsBios::setup() in your sketch's setup() after this.
    void setup(HardwareSerial& panelBridgePort);

    // Drain UART from PanelBridge and dispatch DIAG frames.
    // Call DcsBios::loop() in your sketch's loop() after this.
    void loop();

    // Send a raw ControlPacket to PanelBridge over UART.
    // Used by the prototype ping-pong test and by DCS output forwarding.
    void send(uint16_t controlId, uint16_t value);

    // DIAG frame callbacks — set before setup().
    void onDiagRtt(void (*cb)(uint16_t seq, uint32_t sentMs));
    void onDiagHb(void (*cb)(uint8_t nodeId, uint16_t rxCount));
    void onDiagErr(void (*cb)(uint8_t tec, uint8_t rec, uint8_t flags));
}

#endif // ARDUINO_ARCH_RP2040
