#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <stm32f1xx_hal_can.h>

// Fixed hardware pinouts — same on every OpenSkyhawk STM32 board:
//   CAN transceiver : SN65HVD230 on PA11 (RX) / PA12 (TX)
//   Diagnostic UART : USART1 PA9 TX / PA10 RX @ 115200  (TX-only in practice)
//   Status LED      : PC13, active LOW

namespace STM32Board {
    // Initialise LED, DiagSerial, and CAN HAL peripheral.
    // Does NOT call HAL_CAN_Start — caller configures filter first, then calls canStart().
    void begin();

    // Start the CAN peripheral. Call after configuring the RX filter.
    void canStart();

    // Blink status LED based on CAN health. Call every loop iteration.
    void update();

    void setDebug(bool on);
    void log(const char* msg);

    // Send a standard-frame CAN message. Returns false on mailbox-full.
    bool canSend(uint32_t canId, const uint8_t* data, uint8_t len);

    uint8_t tec();
    uint8_t rec();
    bool    busOff();

    // Access the HAL handle for RX filter configuration (PanelGroup / PanelBridge).
    CAN_HandleTypeDef* canHandle();

    // Access DiagSerial for extended formatted output (PanelGroup heartbeat, etc.).
    HardwareSerial& diagSerial();
}

#endif // ARDUINO_ARCH_STM32
