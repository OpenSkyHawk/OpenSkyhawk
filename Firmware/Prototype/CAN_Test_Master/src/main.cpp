// CAN_Test_Master — STM32F103CBT6
//
// UART2 (PA2 TX / PA3 RX) @ 250000  — ControlPacket structs from Arduino / RP2040
// CAN1  (PA11 RX / PA12 TX)         — SN65HVD230 transceiver, 500 kbps
// UART1 (PA9 TX only) @ 115200       — diagnostic tap → USB-TTL adapter
//
// Bit timing: APB1 36 MHz, prescaler=4, BS1=13TQ, BS2=4TQ → 500 kbps, SP 77.8%
// CAN drives via HAL_CAN (no external library — framework provides stm32yyxx_hal_can)

#include <Arduino.h>
#include <stm32f1xx_hal_can.h>
#include <CANProtocol.h>

// ── peripherals ───────────────────────────────────────────────────────────────
HardwareSerial DiagSerial(PA9, PA10);  // UART1, TX-only diagnostic tap
// PA9 = UART1 TX → USB-TTL adapter. PA10 is declared as RX but never used;
// leave PA10 unconnected on the bench.

// ── HAL CAN state ─────────────────────────────────────────────────────────────
static CAN_HandleTypeDef    hcan;
static CAN_TxHeaderTypeDef  txHeader;
static CAN_RxHeaderTypeDef  rxHeader;

// ── local CAN ID aliases ──────────────────────────────────────────────────────
static constexpr uint32_t ID_CTRL_BCAST = CAN_ID_CTRL_BCAST;
static constexpr uint32_t ID_TEST_SEQ   = CAN_ID_TEST_SEQ;
static constexpr uint32_t ID_HB_1       = CAN_ID_HB_1;
static constexpr uint32_t ID_HB_2       = CAN_ID_HB_2;
static constexpr uint32_t ID_EVT_1      = CAN_ID_EVT_1;
static constexpr uint32_t ID_EVT_2      = CAN_ID_EVT_2;
static constexpr uint32_t ID_ECHO_1     = CAN_ID_ECHO_1;
static constexpr uint32_t ID_ECHO_2     = CAN_ID_ECHO_2;

// ── node state ────────────────────────────────────────────────────────────────
static constexpr uint8_t  NUM_NODES      = 2;
static constexpr uint32_t HB_TIMEOUT_MS  = 3000;

struct NodeState {
    bool     alive;
    uint32_t lastHbMs;
    uint8_t  flags;
    uint16_t rxCount;
    uint16_t esr;
};
static NodeState nodes[NUM_NODES] = {};

// ── counters ──────────────────────────────────────────────────────────────────
static uint32_t txFullCount = 0;
static uint32_t ovfCount    = 0;

// ── UART receive buffer ───────────────────────────────────────────────────────
static uint8_t uartBuf[8];
static uint8_t uartPos = 0;

// ── LED / scope ───────────────────────────────────────────────────────────────
static constexpr uint8_t LED_PIN   = PC13;
static constexpr uint8_t SCOPE_PIN = PB0;

// ── CAN MSP init (GPIO for PA11/PA12) ────────────────────────────────────────
extern "C" void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan_p) {
    if (hcan_p->Instance != CAN1) return;
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};

    // PA12 — CAN1_TX, alternate function push-pull
    gpio.Pin   = GPIO_PIN_12;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    // PA11 — CAN1_RX, input floating
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

    // Accept all frames into FIFO0
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

    // Shared TX header defaults
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
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
    // Scope trigger: brief pulse on PB0 so an oscilloscope can time the frame
    digitalWrite(SCOPE_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(SCOPE_PIN, LOW);
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
            case ID_HB_1:
            case ID_HB_2: {
                uint8_t nodeIdx = (id == ID_HB_1) ? 0 : 1;
                NodeState& n = nodes[nodeIdx];
                n.alive    = true;
                n.lastHbMs = now;
                n.flags    = rxData[1];
                memcpy(&n.rxCount, rxData + 4, 2);
                memcpy(&n.esr,     rxData + 6, 2);

                DiagSerial.print(F("[HB] node="));
                DiagSerial.print(nodeIdx + 1);
                DiagSerial.print(F(" flags=0x"));
                DiagSerial.print(n.flags, HEX);
                DiagSerial.print(F(" rx="));
                DiagSerial.print(n.rxCount);
                DiagSerial.print(F(" ESR=0x"));
                DiagSerial.println(n.esr, HEX);

                // Structured diag → Arduino
                uint8_t pl[6] = {(uint8_t)(nodeIdx + 1), n.flags};
                memcpy(pl + 2, &n.rxCount, 2);
                memcpy(pl + 4, &n.esr,     2);
                uint8_t pkt[8] = {DIAG_MAGIC, DIAG_HB};
                memcpy(pkt + 2, pl, 6);
                Serial.write(pkt, 8);
                break;
            }
            case ID_EVT_1:
            case ID_EVT_2: {
                uint16_t ctrlId, val;
                memcpy(&ctrlId, rxData,     2);
                memcpy(&val,    rxData + 2, 2);
                DiagSerial.print(F("[EVT] node="));
                DiagSerial.print((id == ID_EVT_1) ? 1 : 2);
                DiagSerial.print(F(" ctrl=0x"));
                DiagSerial.print(ctrlId, HEX);
                DiagSerial.print(F(" val="));
                DiagSerial.println(val);
                break;
            }
            case ID_ECHO_1:
            case ID_ECHO_2: {
                uint32_t seq, rxMs;
                memcpy(&seq,  rxData,     4);
                memcpy(&rxMs, rxData + 4, 4);
                uint32_t rtt = now - rxMs;
                DiagSerial.print(F("[RTT] seq="));
                DiagSerial.print((uint16_t)seq);
                DiagSerial.print(F(" ~rtt="));
                DiagSerial.print(rtt);
                DiagSerial.println(F("ms"));

                uint8_t pl[6];
                uint16_t seq16 = (uint16_t)seq;
                memcpy(pl,     &seq16, 2);
                memcpy(pl + 2, &rxMs,  4);
                uint8_t pkt[8] = {DIAG_MAGIC, DIAG_RTT};
                memcpy(pkt + 2, pl, 6);
                Serial.write(pkt, 8);
                break;
            }
        }
    }
}

// ── UART receive + CAN broadcast ─────────────────────────────────────────────
static void processUartPacket(const ControlPacket& pkt) {
    if (pkt.controlId == CTRL_TEST_SEQ) {
        uint8_t buf[8];
        uint32_t seq32 = pkt.value;
        uint32_t now   = millis();
        memcpy(buf,     &seq32, 4);
        memcpy(buf + 4, &now,   4);
        canTx(ID_TEST_SEQ, buf, 8);
        DiagSerial.print(F("[SEQ] tx seq="));
        DiagSerial.println(pkt.value);
    } else {
        uint8_t buf[4];
        memcpy(buf, &pkt, 4);
        canTx(ID_CTRL_BCAST, buf, 4);
    }
}

static void drainUart() {
    while (Serial.available()) {
        uint8_t b = Serial.read();
        uartBuf[uartPos++] = b;
        if (uartPos == 4) {
            ControlPacket pkt;
            memcpy(&pkt, uartBuf, 4);
            if (pkt.controlId <= 0x00FF || pkt.controlId == CTRL_TEST_SEQ) {
                processUartPacket(pkt);
            } else {
                ovfCount++;
                DiagSerial.print(F("[OVF] align total="));
                DiagSerial.println(ovfCount);
                memmove(uartBuf, uartBuf + 1, 3);
                uartPos = 3;
                return;
            }
            uartPos = 0;
        }
    }
}

// ── error monitoring ──────────────────────────────────────────────────────────
static uint32_t lastErrReport = 0;
static uint32_t lastLedMs     = 0;
static bool     ledState       = false;
static bool     busOff         = false;

static void monitorErrors(uint32_t now) {
    if (now - lastErrReport < 1000) return;
    lastErrReport = now;

    uint32_t esr = CAN1->ESR;
    uint8_t  tec = (esr >> 16) & 0xFF;  // ESR[23:16]
    uint8_t  rec = (esr >> 24) & 0xFF;  // ESR[31:24]
    uint8_t  flags = 0;
    if (esr & (1 << 2)) flags |= 0x01;  // BOFF — ESR bit 2
    if (esr & (1 << 1)) flags |= 0x02;  // EPVF — ESR bit 1
    if (esr & (1 << 0)) flags |= 0x04;  // EWGF — ESR bit 0
    busOff = (flags & 0x01);

    if (tec > 0 || rec > 0 || flags) {
        DiagSerial.print(F("[ERRS] TEC="));
        DiagSerial.print(tec);
        DiagSerial.print(F(" REC="));
        DiagSerial.print(rec);
        DiagSerial.print(F(" flags=0x"));
        DiagSerial.println(flags, HEX);

        uint8_t pkt[8] = {DIAG_MAGIC, DIAG_ERR, tec, rec, flags, 0, 0, 0};
        Serial.write(pkt, 8);
    }

    for (uint8_t i = 0; i < NUM_NODES; i++) {
        if (nodes[i].alive && (now - nodes[i].lastHbMs > HB_TIMEOUT_MS)) {
            nodes[i].alive = false;
            DiagSerial.print(F("[DEAD] node="));
            DiagSerial.println(i + 1);
        }
    }
}

static void updateLed(uint32_t now) {
    if (busOff) { digitalWrite(LED_PIN, LOW); return; }
    bool anyDead = (!nodes[0].alive || !nodes[1].alive);
    uint32_t period = anyDead ? 100 : 500;
    if (now - lastLedMs >= period) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastLedMs = now;
    }
}

// ── setup / loop ──────────────────────────────────────────────────────────────
void setup() {
    pinMode(LED_PIN,   OUTPUT); digitalWrite(LED_PIN, HIGH);
    pinMode(SCOPE_PIN, OUTPUT); digitalWrite(SCOPE_PIN, LOW);

    DiagSerial.begin(115200);
    DiagSerial.println(F("CAN_Test_Master starting..."));

    Serial.begin(250000);
    canInit();

    DiagSerial.println(F("CAN_Test_Master ready."));
}

void loop() {
    uint32_t now = millis();
    drainUart();
    processCan();
    monitorErrors(now);
    updateLed(now);
}
