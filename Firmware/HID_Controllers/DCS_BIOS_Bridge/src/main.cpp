// DCS-BIOS serial bridge — Tiny2040 (RP2040)
// Relays USB CDC (PC / DCS-BIOS @ 250000) ↔ UART0 (STM32 @ 250000).
//
// Wiring:
//   GP0 (UART0 TX) → STM32 PA3 (RX)
//   GP1 (UART0 RX) ← STM32 PA2 (TX)
//   GND             — GND  (shared; both sides 3.3 V — no level shifter needed)
//
// Serial1 on arduino-pico defaults to GP0/GP1 (UART0). Verify against your
// board package version before wiring.

#include <Arduino.h>

void setup() {
    Serial.begin(250000);   // USB CDC — PC / DCS-BIOS
    Serial1.begin(250000);  // UART0 (GP0/GP1) — STM32
}

void loop() {
    while (Serial.available())  Serial1.write(Serial.read());
    while (Serial1.available()) Serial.write(Serial1.read());
}
