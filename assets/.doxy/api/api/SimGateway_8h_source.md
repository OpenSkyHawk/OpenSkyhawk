

# File SimGateway.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**SimGateway.h**](SimGateway_8h.md)

[Go to the documentation of this file](SimGateway_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_RP2040

#include <Arduino.h>
#include <HIDControls.h>

#include "AxisCal.h"

namespace OpenSkyhawk {

class HIDAxis {
public:
    HIDAxis(uint16_t controlId, uint8_t axisIndex);

    static HIDAxis* head();       
    uint16_t controlId() const;   
    uint8_t  axisIndex() const;   
    void     dispatch(uint16_t value);
    HIDAxis* next() const;        

private:
    static HIDAxis* _head;
    HIDAxis*        _next;
    uint16_t        _controlId;
    uint8_t         _axisIndex;
};

class HIDButton {
public:
    HIDButton(uint16_t controlId, uint8_t buttonIndex);

    static HIDButton* head();
    uint16_t controlId() const;
    void      dispatch(uint16_t value);
    HIDButton* next() const;

private:
    static HIDButton* _head;
    HIDButton*        _next;
    uint16_t          _controlId;
    uint8_t           _buttonIndex;
};

class HIDHatSwitch {
public:
    HIDHatSwitch(uint16_t controlId, uint8_t hatIndex);

    static HIDHatSwitch* head();
    uint16_t controlId() const;
    void          dispatch(uint16_t value);
    HIDHatSwitch* next() const;

private:
    static HIDHatSwitch* _head;
    HIDHatSwitch*        _next;
    uint16_t             _controlId;
    uint8_t              _hatIndex;
};

} // namespace OpenSkyhawk

namespace SimGateway {

static constexpr uint8_t DEFAULT_UART_TX_PIN = 0; 
static constexpr uint8_t DEFAULT_UART_RX_PIN = 1; 

// Firmware version reported in the calibration protocol's HELLO_ACK, so the client can show
// which build a device is running. Keep in step with library.json.
#define SIMGATEWAY_FW_MAJOR 0
#define SIMGATEWAY_FW_MINOR 1
#define SIMGATEWAY_FW_PATCH 0

void setup(SerialUART& uart,
           uint8_t txPin = DEFAULT_UART_TX_PIN,
           uint8_t rxPin = DEFAULT_UART_RX_PIN);

void loop();

const OpenSkyhawk::CalBlob& calibration();

// ── Status LEDs (Gateway_Bridge board) ────────────────────────────────────────
//
// Two board-mounted status LEDs report the gateway's USB / data / fault state at a
// glance: RED = GP3, GREEN = GP2, board-mounted 0805, active-high (HIGH = on). The
// animator is non-blocking (millis(), never delay()) and is ticked from loop(), so
// sketches need no LED code. The RP2040 module's onboard WS2812 is not used.
// See docs/architecture/sim-gateway.md for the user-facing table.

enum class LedState : uint8_t {
    FAULT,     
    NO_HOST,   
    STREAMING, 
    USB_IDLE,  
    INIT,      
};

enum class Anim : uint8_t {
    OFF,   
    SOLID, 
    SLOW,  
    FAST,  
    ALT,   
    PULSE, 
};

void statusLedBegin();

void statusTick();

#ifdef SIMGATEWAY_TEST
bool feedByte(uint8_t b);

void resetParser();

void resetCdcCapture();

size_t cdcCaptureCount();

uint8_t cdcCaptureByte(size_t index);

bool cdcCaptureOverflow();

// ── Axis calibration test hooks (test builds only) ────────────────────────────
// The transform itself is tested by calling OpenSkyhawk::axisCalApply() directly. These
// hooks exist to cover something that function-level test cannot: that HIDAxis::dispatch()
// actually *applies* it. That path is doubly hidden in test builds — the HID setters are
// no-op stubs, and the flash load is stubbed to a RAM clear so no calibration can be
// installed. Without both a writer and a reader, deleting the transform from dispatch()
// would leave every assertion passing.

void calSetForTest(const OpenSkyhawk::CalBlob& blob);

int16_t lastAxisValue();

uint8_t lastAxisIndex();

void resetAxisCapture();

uint8_t hidSendCount();

void resetHidSendCount();

// ── Calibration transport test hooks (test builds only) ───────────────────────
// The transport's safety property is that a rejected candidate is handed back to the relay
// byte-for-byte. Asserting that needs a capture on the UART side, which had no test seam.

bool feedCdcByte(uint8_t b);

void resetUartCapture();

size_t uartCaptureCount();

uint8_t uartCaptureByte(size_t index);

bool uartCaptureOverflow();

void calResetForTest();

bool calSessionOpen();

uint8_t calStreamAxis();

// ── Status-LED test hooks (test builds only) ──────────────────────────────────
// The pure state-selection + animation logic is exercised without GPIO, TinyUSB,
// or PL011 register reads by injecting inputs and reading back the resolved state /
// captured pin levels. Mirrors the feedByte/cdcCapture pattern above.

void statusInject(uint32_t now, bool mounted, uint32_t lastCdcRxMs, bool faultActive);

void statusResolve();

bool statusFaultStep(uint32_t now, bool rsrError, bool uartRxMoved);

LedState statusState();

Anim statusAnim();

bool statusRedLevel();

bool statusGreenLevel();

void statusResetForTest();
#endif

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
```


