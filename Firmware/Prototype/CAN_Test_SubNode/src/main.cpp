// CAN_Test_SubNode — STM32F103CBT6
//
// Same binary flashed to both Sub-1 and Sub-2.
// Node ID from PA0 strap: Sub-1 ties PA0 to 3.3V → node_id=1; Sub-2 leaves PA0
// floating (internal pull-down reads LOW) → node_id=2.
//
// CAN1 (PA11 RX / PA12 TX) 500 kbps via HAL_CAN — no external library needed.

#include <Arduino.h>
#include <stm32f1xx_hal_can.h>
#include <CANProtocol.h>

static constexpr uint8_t LED_PIN   = PC13;
static constexpr uint8_t STRAP_PIN = PA0;
static constexpr uint8_t BTN_PIN   = PB1;

static uint8_t  node_id   = 1;
static uint32_t hbTxId    = CAN_ID_HB_1;
static uint32_t evtTxId   = CAN_ID_EVT_1;
static uint32_t echoTxId  = CAN_ID_ECHO_1;
static uint32_t rxCount   = 0;
static uint32_t txDrops   = 0;  // mailbox-full TX failures
static uint32_t startMs   = 0;

// ── HAL CAN state ─────────────────────────────────────────────────────────────
static CAN_HandleTypeDef   hcan;
static CAN_TxHeaderTypeDef txHeader;
static CAN_RxHeaderTypeDef rxHeader;

// ── CAN MSP init ──────────────────────────────────────────────────────────────
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

// ── CAN init ──────────────────────────────────────────────────────────────────
static void canInit() {
    hcan.Instance                  = CAN1;
    hcan.Init.Prescaler            = 4;
    hcan.Init.Mode                 = CAN_MODE_NORMAL;
    hcan.Init.SyncJumpWidth        = CAN_SJW_1TQ;
    hcan.Init.TimeSeg1             = CAN_BS1_13TQ;
    hcan.Init.TimeSeg2             = CAN_BS2_4TQ;
    hcan.Init.TimeTriggeredMode    = DISABLE;
    hcan.Init.AutoBusOff           = DISABLE;
    hcan.Init.AutoWakeUp           = DISABLE;
    hcan.Init.AutoRetransmission   = ENABLE;
    hcan.Init.ReceiveFifoLocked    = DISABLE;
    hcan.Init.TransmitFifoPriority = DISABLE;
    HAL_CAN_Init(&hcan);

    // Accept 0x010 (CONTROL_BROADCAST) and 0x011 (TEST_SEQ) only
    CAN_FilterTypeDef filter = {};
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDLIST;
    filter.FilterScale          = CAN_FILTERSCALE_16BIT;
    filter.FilterIdHigh         = CAN_ID_CTRL_BCAST << 5;
    filter.FilterIdLow          = CAN_ID_TEST_SEQ   << 5;
    filter.FilterMaskIdHigh     = 0;
    filter.FilterMaskIdLow      = 0;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &filter);

    HAL_CAN_Start(&hcan);

    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.TransmitGlobalTime = DISABLE;
}

// ── CAN transmit ──────────────────────────────────────────────────────────────
static void canTx(uint32_t id, const uint8_t* data, uint8_t len) {
    txHeader.StdId = id;
    txHeader.DLC   = len;
    uint32_t mailbox;
    if (HAL_CAN_AddTxMessage(&hcan, &txHeader, const_cast<uint8_t*>(data), &mailbox) != HAL_OK) {
        txDrops++;
    }
}

// ── CAN receive ───────────────────────────────────────────────────────────────
static void processCan() {
    uint8_t rxData[8];
    while (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0) {
        HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader, rxData);
        switch (rxHeader.StdId) {
            case CAN_ID_CTRL_BCAST:
                rxCount++;
                break;
            case CAN_ID_TEST_SEQ: {
                uint32_t now = millis();
                uint8_t buf[8];
                memcpy(buf,     rxData, 4);  // echo seq_num unchanged
                memcpy(buf + 4, &now,   4);
                canTx(echoTxId, buf, 8);
                break;
            }
        }
    }
}

// ── heartbeat ─────────────────────────────────────────────────────────────────
static uint32_t lastHbMs = 0;

static void sendHeartbeat(uint32_t now) {
    if (now - lastHbMs < 500) return;
    lastHbMs = now;

    uint32_t esr32 = CAN1->ESR;
    uint8_t  flags = 0;
    if (esr32 & (1 << 2)) flags |= 0x01;  // BOFF  — ESR bit 2
    if (esr32 & (1 << 1)) flags |= 0x02;  // EPVF  — ESR bit 1
    if (txDrops > 0)       flags |= 0x04;  // TXDROP — mailbox-full TX failures

    uint16_t uptime16 = (uint16_t)((now - startMs) / 1000);
    uint16_t rxc16    = (uint16_t)(rxCount & 0xFFFF);
    uint16_t esr16    = (uint16_t)(CAN1->ESR >> 16);

    uint8_t buf[8];
    buf[0] = node_id;
    buf[1] = flags;
    memcpy(buf + 2, &uptime16, 2);
    memcpy(buf + 4, &rxc16,    2);
    memcpy(buf + 6, &esr16,    2);
    canTx(hbTxId, buf, 8);
}

// ── button → INPUT_EVENT ─────────────────────────────────────────────────────
static bool lastBtnState = HIGH;

static void checkButton() {
    bool state = digitalRead(BTN_PIN);
    if (state == lastBtnState) return;
    lastBtnState = state;

    uint16_t ctrlId = 0x0001;
    uint16_t val    = (state == LOW) ? 1 : 0;
    uint32_t now    = millis();

    uint8_t buf[8];
    memcpy(buf,     &ctrlId, 2);
    memcpy(buf + 2, &val,    2);
    memcpy(buf + 4, &now,    4);
    canTx(evtTxId, buf, 8);
}

// ── LED ───────────────────────────────────────────────────────────────────────
static uint32_t lastLedMs = 0;
static bool     ledState  = false;

static void updateLed(uint32_t now) {
    uint32_t esr  = CAN1->ESR;
    uint8_t  tec  = (esr >> 16) & 0xFF;  // ESR[23:16]
    bool     boff = (esr >> 2) & 1;      // ESR bit 2
    if (boff) { digitalWrite(LED_PIN, LOW); return; }
    uint32_t period = (tec > 0) ? 100 : 500;
    if (now - lastLedMs >= period) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastLedMs = now;
    }
}

// ── setup / loop ──────────────────────────────────────────────────────────────
void setup() {
    startMs = millis();
    pinMode(LED_PIN,   OUTPUT); digitalWrite(LED_PIN, HIGH);
    pinMode(BTN_PIN,   INPUT_PULLUP);

    pinMode(STRAP_PIN, INPUT_PULLDOWN);
    delay(10);
    node_id   = digitalRead(STRAP_PIN) ? 1 : 2;
    hbTxId    = (node_id == 1) ? CAN_ID_HB_1   : CAN_ID_HB_2;
    evtTxId   = (node_id == 1) ? CAN_ID_EVT_1  : CAN_ID_EVT_2;
    echoTxId  = (node_id == 1) ? CAN_ID_ECHO_1 : CAN_ID_ECHO_2;

    canInit();
}

void loop() {
    uint32_t now = millis();
    processCan();
    checkButton();
    sendHeartbeat(now);
    updateLed(now);
}
