// CAN_Test_Master — STM32F103CBT6
//
// USART1 PA9/PA10  @ 115200  — diagnostic tap (DiagSerial, RX=PA10 TX=PA9)
// USART2 PA2/PA3   @ 250000  — RP2040 link    (Serial2, built-in)
// CAN1   PA11/PA12 @ 500kbps — SN65HVD230 transceiver
//
// STM32duino HardwareSerial constructor is (RX, TX).

#include <Arduino.h>
#include <stm32f1xx_hal_can.h>
#include <CANProtocol.h>

// ── serial ports ──────────────────────────────────────────────────────────────
HardwareSerial DiagSerial(PA10, PA9);  // USART1: RX=PA10, TX=PA9

// ── HAL CAN ───────────────────────────────────────────────────────────────────
static CAN_HandleTypeDef   hcan;
static CAN_TxHeaderTypeDef txHeader;
static CAN_RxHeaderTypeDef rxHeader;

// ── LED ───────────────────────────────────────────────────────────────────────
static constexpr uint8_t LED_PIN = PC13;
static uint32_t lastLedMs = 0;
static bool     ledState  = false;

// ── node watchdog ─────────────────────────────────────────────────────────────
static constexpr uint32_t HB_TIMEOUT_MS = 3000;
static bool     nodeAlive   = false;
static uint32_t lastHbMs    = 0;

// ── counters ──────────────────────────────────────────────────────────────────
static uint32_t txFullCount = 0;
static uint32_t ovfCount    = 0;

// ── UART receive buffer ───────────────────────────────────────────────────────
static uint8_t uartBuf[4];
static uint8_t uartPos = 0;

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
    hcan.Init.Prescaler            = 4;   // 500 kbps
    hcan.Init.Mode                 = CAN_MODE_NORMAL;
    hcan.Init.SyncJumpWidth        = CAN_SJW_4TQ;
    hcan.Init.TimeSeg1             = CAN_BS1_13TQ;
    hcan.Init.TimeSeg2             = CAN_BS2_4TQ;
    hcan.Init.TimeTriggeredMode    = DISABLE;
    hcan.Init.AutoBusOff           = ENABLE;
    hcan.Init.AutoWakeUp           = DISABLE;
    hcan.Init.AutoRetransmission   = DISABLE;
    hcan.Init.ReceiveFifoLocked    = DISABLE;
    hcan.Init.TransmitFifoPriority = DISABLE;
    HAL_CAN_Init(&hcan);

    // Accept all frames
    CAN_FilterTypeDef filter = {};
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = 0;
    filter.FilterIdLow          = 0;
    filter.FilterMaskIdHigh     = 0;
    filter.FilterMaskIdLow      = 0;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &filter);

    HAL_CAN_Start(&hcan);

    txHeader.IDE                = CAN_ID_STD;
    txHeader.RTR                = CAN_RTR_DATA;
    txHeader.TransmitGlobalTime = DISABLE;
}

// ── CAN transmit ──────────────────────────────────────────────────────────────
static bool canTx(uint32_t id, const uint8_t* data, uint8_t len) {
    txHeader.StdId = id;
    txHeader.DLC   = len;
    uint32_t mailbox;
    if (HAL_CAN_AddTxMessage(&hcan, &txHeader, const_cast<uint8_t*>(data), &mailbox) != HAL_OK) {
        txFullCount++;
        DiagSerial.print(F("[TXFULL] total="));
        DiagSerial.println(txFullCount);
        return false;
    }
    return true;
}

// ── CAN receive ───────────────────────────────────────────────────────────────
static void processCan() {
    uint8_t rxData[8];
    while (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0) {
        HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader, rxData);
        uint32_t id  = rxHeader.StdId;
        uint32_t now = millis();

        switch (id) {
            case CAN_ID_TEST_SEQ: {
                // Loopback mode only — master receives its own TEST_SEQ frame
                uint32_t seq;
                memcpy(&seq, rxData, 4);
                uint16_t seq16 = (uint16_t)seq;
                DiagSerial.print(F("[LOOPBACK] seq="));
                DiagSerial.println(seq16);
                // Send DIAG_RTT back to RP2040
                uint8_t rtt[8] = {DIAG_MAGIC, DIAG_RTT};
                memcpy(rtt + 2, &seq16,     2);
                memcpy(rtt + 4, rxData + 4, 4);
                Serial2.write(rtt, 8);
                break;
            }
            case CAN_ID_HB_1:
            case CAN_ID_HB_2: {
                nodeAlive = true;
                lastHbMs  = now;
                uint16_t esr16;
                memcpy(&esr16, rxData + 6, 2);
                DiagSerial.print(F("[HB] node="));
                DiagSerial.print(rxData[0]);
                DiagSerial.print(F(" flags=0x"));
                DiagSerial.print(rxData[1], HEX);
                DiagSerial.print(F(" ESR=0x"));
                DiagSerial.println(esr16, HEX);
                break;
            }
            case CAN_ID_ECHO_1:
            case CAN_ID_ECHO_2: {
                uint32_t seq;
                memcpy(&seq, rxData, 4);
                uint16_t seq16 = (uint16_t)seq;
                DiagSerial.print(F("[ECHO] seq="));
                DiagSerial.println(seq16);

                // Forward DIAG_RTT to RP2040
                uint8_t rtt[8] = {DIAG_MAGIC, DIAG_RTT};
                memcpy(rtt + 2, &seq16,      2);
                memcpy(rtt + 4, rxData + 4,  4);  // original send timestamp
                Serial2.write(rtt, 8);
                break;
            }
        }
    }
}

// ── UART packet handler ───────────────────────────────────────────────────────
static void processUartPacket(const ControlPacket& pkt) {
    if (pkt.controlId == CTRL_TEST_SEQ) {
        uint8_t buf[8];
        uint32_t seq32 = pkt.value;
        uint32_t now   = millis();
        memcpy(buf,     &seq32, 4);
        memcpy(buf + 4, &now,   4);
        canTx(CAN_ID_TEST_SEQ, buf, 8);
        DiagSerial.print(F("[SEQ] tx seq="));
        DiagSerial.println(pkt.value);
    } else {
        uint8_t buf[4];
        memcpy(buf, &pkt, 4);
        canTx(CAN_ID_CTRL_BCAST, buf, 4);
    }
}

static void drainUart() {
    while (Serial2.available()) {
        uint8_t b = Serial2.read();
        uartBuf[uartPos++] = b;
        if (uartPos < 4) continue;
        uartPos = 0;

        ControlPacket pkt;
        memcpy(&pkt, uartBuf, 4);
        if (pkt.controlId <= 0x00FF || pkt.controlId == CTRL_TEST_SEQ) {
            processUartPacket(pkt);
        } else {
            ovfCount++;
            DiagSerial.print(F("[OVF] total="));
            DiagSerial.println(ovfCount);
            memmove(uartBuf, uartBuf + 1, 3);
            uartPos = 3;
        }
    }
}

// ── error monitor ─────────────────────────────────────────────────────────────
static uint32_t lastErrMs = 0;

static void monitorErrors(uint32_t now) {
    if (now - lastErrMs < 1000) return;
    lastErrMs = now;

    uint32_t esr = CAN1->ESR;
    uint8_t  tec = (esr >> 16) & 0xFF;
    uint8_t  rec = (esr >> 24) & 0xFF;
    bool     boff = (esr >> 2) & 1;

    if (tec > 0 || rec > 0 || boff) {
        DiagSerial.print(F("[ERRS] TEC="));
        DiagSerial.print(tec);
        DiagSerial.print(F(" REC="));
        DiagSerial.print(rec);
        DiagSerial.print(F(" BOFF="));
        DiagSerial.println(boff);
    }

    if (nodeAlive && (now - lastHbMs > HB_TIMEOUT_MS)) {
        nodeAlive = false;
        DiagSerial.println(F("[DEAD] node=1"));
    }
}

// ── LED ───────────────────────────────────────────────────────────────────────
static void updateLed(uint32_t now) {
    uint32_t period = nodeAlive ? 500 : 200;
    if (now - lastLedMs >= period) {
        lastLedMs = now;
        ledState  = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
    }
}

// ── setup / loop ──────────────────────────────────────────────────────────────
void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    DiagSerial.begin(115200);
    DiagSerial.println(F("CAN_Test_Master ready."));

    Serial2.begin(250000);

    canInit();
    DiagSerial.println(F("CAN ready."));
}

void loop() {
    uint32_t now = millis();
    drainUart();
    processCan();
    monitorErrors(now);
    updateLed(now);
}
