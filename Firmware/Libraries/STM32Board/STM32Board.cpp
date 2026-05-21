#ifdef ARDUINO_ARCH_STM32

#include "STM32Board.h"

static constexpr uint8_t LED_PIN = PC13;

static HardwareSerial      _diag(PA10, PA9);   // USART1 RX/TX
static CAN_HandleTypeDef   _hcan;
static CAN_TxHeaderTypeDef _txHdr;
static bool     _debugOn  = false;
static bool     _ledState = false;
static uint32_t _ledMs    = 0;
static uint32_t _txDrops  = 0;

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

namespace STM32Board {

void begin() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // active low — off at startup

    _diag.begin(115200);

    _hcan.Instance                  = CAN1;
    _hcan.Init.Prescaler            = 4;   // 72 MHz / 4 / (1+13+4) = 500 kbps
    _hcan.Init.Mode                 = CAN_MODE_NORMAL;
    _hcan.Init.SyncJumpWidth        = CAN_SJW_4TQ;
    _hcan.Init.TimeSeg1             = CAN_BS1_13TQ;
    _hcan.Init.TimeSeg2             = CAN_BS2_4TQ;
    _hcan.Init.TimeTriggeredMode    = DISABLE;
    _hcan.Init.AutoBusOff           = ENABLE;
    _hcan.Init.AutoWakeUp           = DISABLE;
    _hcan.Init.AutoRetransmission   = DISABLE;  // prevent runaway bus-off cycle
    _hcan.Init.ReceiveFifoLocked    = DISABLE;
    _hcan.Init.TransmitFifoPriority = DISABLE;
    HAL_CAN_Init(&_hcan);

    _txHdr.IDE                = CAN_ID_STD;
    _txHdr.RTR                = CAN_RTR_DATA;
    _txHdr.TransmitGlobalTime = DISABLE;
}

void canStart() {
    HAL_CAN_Start(&_hcan);
}

void update() {
    uint32_t now  = millis();
    uint32_t esr  = CAN1->ESR;
    bool     boff = (esr >> 2) & 1;
    uint8_t  tec_ = (esr >> 16) & 0xFF;

    if (boff) {
        digitalWrite(LED_PIN, LOW);  // solid on = bus-off fault
        return;
    }
    uint32_t period = (tec_ > 0) ? 100 : 500;
    if (now - _ledMs >= period) {
        _ledState = !_ledState;
        digitalWrite(LED_PIN, _ledState ? LOW : HIGH);
        _ledMs = now;
    }
}

void setDebug(bool on) { _debugOn = on; }

void log(const char* msg) {
    if (_debugOn) _diag.println(msg);
}

bool canSend(uint32_t canId, const uint8_t* data, uint8_t len) {
    _txHdr.StdId = canId;
    _txHdr.DLC   = len;
    uint32_t mailbox;
    if (HAL_CAN_AddTxMessage(&_hcan, &_txHdr, const_cast<uint8_t*>(data), &mailbox) != HAL_OK) {
        _txDrops++;
        _diag.print(F("[TXFAIL] id=0x"));
        _diag.print(canId, HEX);
        _diag.print(F(" total="));
        _diag.println(_txDrops);
        return false;
    }
    return true;
}

uint8_t tec()    { return (CAN1->ESR >> 16) & 0xFF; }
uint8_t rec()    { return (CAN1->ESR >> 24) & 0xFF; }
bool    busOff() { return (CAN1->ESR >> 2) & 1; }

CAN_HandleTypeDef* canHandle() { return &_hcan; }
HardwareSerial&    diagSerial() { return _diag; }

} // namespace STM32Board

#endif // ARDUINO_ARCH_STM32
