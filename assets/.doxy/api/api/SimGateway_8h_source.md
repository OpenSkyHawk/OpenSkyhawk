

# File SimGateway.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**SimGateway.h**](SimGateway_8h.md)

[Go to the documentation of this file](SimGateway_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_RP2040

#include <Arduino.h>
#include <HIDControls.h>

namespace OpenSkyhawk {

class HIDAxis {
public:
    HIDAxis(uint16_t controlId, uint8_t axisIndex);

    static HIDAxis* head();       
    uint16_t controlId() const;   
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

void setup(SerialUART& uart,
           uint8_t txPin = DEFAULT_UART_TX_PIN,
           uint8_t rxPin = DEFAULT_UART_RX_PIN);

void loop();

#ifdef SIMGATEWAY_TEST
bool feedByte(uint8_t b);

void resetParser();

void resetCdcCapture();

size_t cdcCaptureCount();

uint8_t cdcCaptureByte(size_t index);

bool cdcCaptureOverflow();
#endif

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
```


