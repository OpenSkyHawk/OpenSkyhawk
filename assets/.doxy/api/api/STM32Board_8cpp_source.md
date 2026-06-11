

# File STM32Board.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**STM32Board**](dir_aa1816754c0645981f9c7af905857f7d.md) **>** [**STM32Board.cpp**](STM32Board_8cpp.md)

[Go to the documentation of this file](STM32Board_8cpp.md)


```C++
#ifdef ARDUINO_ARCH_STM32

#include "STM32Board.h"
#include <CANProtocol.h>  // for CanStatus enum values

// ── Private types ─────────────────────────────────────────────────────────────

enum class LedState {
    OFF,       
    BOOTING,   
    NORMAL,    
    CAN_ERROR, 
    BUS_OFF,   
    WARNING,   
};

// ── Private state ─────────────────────────────────────────────────────────────

static LedState          _state      = LedState::OFF;
static bool              _blinkPhase = false;
static uint32_t          _ledLastMs  = 0;
static bool              _debugOn    = false;
static HardwareSerial    _diag(PA10, PA9);  // USART1: RX=PA10, TX=PA9
static CAN_HandleTypeDef _hcan;

// ── Private helpers ───────────────────────────────────────────────────────────

static uint16_t _blinkPeriodFor(LedState s) {
    switch (s) {
        case LedState::BOOTING:   return 1000;
        case LedState::NORMAL:    return 1000;
        case LedState::CAN_ERROR: return 250;
        case LedState::WARNING:   return 500;
        default:                  return 0;  // solid or off — no timer needed
    }
}

static void _applyLed() {
    switch (_state) {
        case LedState::OFF:
            digitalWrite(STM32Board::PIN_LED_RED,   LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, LOW);
            break;
        case LedState::BOOTING:
        case LedState::CAN_ERROR:
            digitalWrite(STM32Board::PIN_LED_RED,   _blinkPhase ? HIGH : LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, LOW);
            break;
        case LedState::NORMAL:
            digitalWrite(STM32Board::PIN_LED_RED,   LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, _blinkPhase ? HIGH : LOW);
            break;
        case LedState::BUS_OFF:
            digitalWrite(STM32Board::PIN_LED_RED,   HIGH);
            digitalWrite(STM32Board::PIN_LED_GREEN, LOW);
            break;
        case LedState::WARNING:
            digitalWrite(STM32Board::PIN_LED_RED,   _blinkPhase ? HIGH : LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, _blinkPhase ? LOW  : HIGH);
            break;
    }
}

static void _setLedState(LedState s) {
    _state      = s;
    _blinkPhase = false;
    _ledLastMs  = millis();
    _applyLed();
}

// HAL weak-symbol override — defines CAN GPIO once for all OpenSkyhawk STM32 boards.
extern "C" void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan_p) {
    if (hcan_p->Instance != CAN1) return;
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin   = GPIO_PIN_12;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin  = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

// ── Public API ────────────────────────────────────────────────────────────────

namespace STM32Board {

void begin() {
    pinMode(PIN_LED_RED,   OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    _setLedState(LedState::BOOTING);

    _diag.begin(115200);

    // 500 kbps on APB1 @ 36 MHz: prescaler=4, BS1=13TQ, BS2=4TQ → 18TQ total.
    // SJW=4TQ required for Blue Pill clone crystal tolerance (validated Experiment B).
    _hcan.Instance                  = CAN1;
    _hcan.Init.Prescaler            = 4;
    _hcan.Init.Mode                 = CAN_MODE_NORMAL;
    _hcan.Init.SyncJumpWidth        = CAN_SJW_4TQ;
    _hcan.Init.TimeSeg1             = CAN_BS1_13TQ;
    _hcan.Init.TimeSeg2             = CAN_BS2_4TQ;
    _hcan.Init.TimeTriggeredMode    = DISABLE;
    _hcan.Init.AutoBusOff           = ENABLE;   // hardware recovery ~3 ms, no firmware action needed
    _hcan.Init.AutoWakeUp           = DISABLE;
    _hcan.Init.AutoRetransmission   = DISABLE;  // prevents runaway bus-off on unACKed frames
    _hcan.Init.ReceiveFifoLocked    = DISABLE;
    _hcan.Init.TransmitFifoPriority = DISABLE;
    HAL_CAN_Init(&_hcan);

}

void setDebug(bool on) { _debugOn = on; }
bool isDebug()         { return _debugOn; }

void tick() {
    uint32_t now    = millis();
    uint16_t period = _blinkPeriodFor(_state);
    if (period > 0 && (now - _ledLastMs) >= (uint32_t)(period / 2)) {
        _blinkPhase = !_blinkPhase;
        _ledLastMs  = now;
        _applyLed();
    }
}

void onCanStatus(CanStatus status) {
    switch (status) {
        case CanStatus::STARTING:  _setLedState(LedState::BOOTING);   break;
        case CanStatus::NORMAL:    _setLedState(LedState::NORMAL);    break;
        case CanStatus::TX_ERROR:  _setLedState(LedState::CAN_ERROR); break;
        case CanStatus::BUS_OFF:   _setLedState(LedState::BUS_OFF);   break;
    }
}

void setWarning() {
    _setLedState(LedState::WARNING);
}

void log(const char* msg) {
    if (_debugOn) _diag.println(msg);
}

HardwareSerial& diagSerial()     { return _diag; }
CAN_HandleTypeDef* canHandle()   { return &_hcan; }

} // namespace STM32Board

#endif // ARDUINO_ARCH_STM32
```


