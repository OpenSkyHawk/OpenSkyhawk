// CAN_Test_SubNode — STM32F103CBT6
//
// Same binary flashed to both Sub-1 and Sub-2.
// Node ID determined at startup by PA0 strap:
//   Sub-1: wire PA0 to 3.3V  → node_id = 1
//   Sub-2: leave PA0 floating → node_id = 2 (internal pull-down reads LOW)
//
// CAN1 (PA11 RX / PA12 TX) 500 kbps — SN65HVD230 transceiver
// No UART — all diagnostics are visible on the master's diagnostic tap.
//
// CAN message IDs (this node transmits):
//   0x100 / 0x101  HEARTBEAT     every 500 ms
//   0x200 / 0x201  INPUT_EVENT   on PB1 button press/release
//   0x210 / 0x211  SEQ_ECHO      immediate on receipt of 0x011 TEST_SEQ
//
// CAN message IDs (this node receives):
//   0x010  CONTROL_BROADCAST  — count received frames (rx_count in heartbeat)
//   0x011  TEST_SEQ           — echo back immediately as SEQ_ECHO
//
// PC13 LED (active-low) status:
//   1 Hz blink  = normal operation
//   5 Hz blink  = CAN TEC > 0 (errors detected)
//   Solid on    = bus-off state

#include <Arduino.h>
#include <STM32_CAN.h>

STM32_CAN can(CAN1, ALT);  // PA11/PA12

static constexpr uint8_t LED_PIN   = PC13;
static constexpr uint8_t STRAP_PIN = PA0;  // HIGH → node 1, LOW → node 2
static constexpr uint8_t BTN_PIN   = PB1;  // active-low button for INPUT_EVENT

static uint8_t  node_id    = 1;
static uint32_t hbTxId     = 0x100;
static uint32_t evtTxId    = 0x200;
static uint32_t echoTxId   = 0x210;
static uint32_t rxCount    = 0;
static uint32_t startMs    = 0;

// ── CAN transmit helpers ──────────────────────────────────────────────────────
static void canTx(uint32_t id, const uint8_t* data, uint8_t len) {
    CAN_message_t msg;
    msg.id  = id;
    msg.len = len;
    memcpy(msg.buf, data, len);
    can.write(msg);  // on sub-node, TX errors surface in TEC; no special handling needed
}

// ── heartbeat ─────────────────────────────────────────────────────────────────
static uint32_t lastHbMs = 0;

static void sendHeartbeat(uint32_t now) {
    if (now - lastHbMs < 500) return;
    lastHbMs = now;

    uint32_t esr32  = CAN1->ESR;
    uint32_t msr32  = CAN1->MSR;
    uint8_t  flags  = 0;
    if (msr32 & (1 << 2)) flags |= 0x01;  // BOFF
    if (msr32 & (1 << 1)) flags |= 0x02;  // EPVF

    uint32_t uptime = (now - startMs) / 1000;
    uint16_t esr16  = (uint16_t)(esr32 >> 16);  // REC in high byte, TEC in next
    uint16_t rxc16  = (uint16_t)(rxCount & 0xFFFF);
    uint16_t uptime16 = (uint16_t)(uptime & 0xFFFF);

    uint8_t buf[8];
    buf[0] = node_id;
    buf[1] = flags;
    memcpy(buf + 2, &uptime16, 2);
    memcpy(buf + 4, &rxc16,    2);
    memcpy(buf + 6, &esr16,    2);

    canTx(hbTxId, buf, 8);
}

// ── CAN receive ───────────────────────────────────────────────────────────────
static void processCan() {
    CAN_message_t msg;
    while (can.read(msg)) {
        switch (msg.id) {
            case 0x010:
                rxCount++;
                break;
            case 0x011: {
                // Immediate SEQ_ECHO
                uint32_t now = millis();
                uint8_t buf[8];
                memcpy(buf,     msg.buf, 4);  // echo seq_num unchanged
                memcpy(buf + 4, &now,    4);
                canTx(echoTxId, buf, 8);
                break;
            }
        }
    }
}

// ── button (INPUT_EVENT) ──────────────────────────────────────────────────────
static bool lastBtnState = HIGH;

static void checkButton() {
    bool state = digitalRead(BTN_PIN);
    if (state == lastBtnState) return;
    lastBtnState = state;

    uint16_t ctrlId = 0x0001;  // MASTER_ARM_SW
    uint16_t val    = (state == LOW) ? 1 : 0;  // LOW = pressed = 1
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
    uint32_t esr = CAN1->ESR;
    uint8_t  tec = (esr >> 24) & 0xFF;
    uint32_t msr = CAN1->MSR;
    bool boff    = (msr >> 2) & 1;

    if (boff) {
        digitalWrite(LED_PIN, LOW);  // solid on
        return;
    }
    uint32_t period = (tec > 0) ? 100 : 500;
    if (now - lastLedMs >= period) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastLedMs = now;
    }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
    startMs = millis();

    pinMode(LED_PIN,   OUTPUT); digitalWrite(LED_PIN, HIGH);  // off
    pinMode(BTN_PIN,   INPUT_PULLUP);

    // Read node ID strap — PA0 with pull-down; Sub-1 ties PA0 to 3.3V
    pinMode(STRAP_PIN, INPUT_PULLDOWN);
    delay(10);
    node_id  = digitalRead(STRAP_PIN) ? 1 : 2;
    hbTxId   = 0x100 + (node_id - 1);
    evtTxId  = 0x200 + (node_id - 1);
    echoTxId = 0x210 + (node_id - 1);

    CAN_bit_timing_config_t timing = {4, 13, 4};
    can.begin();
    can.setBaudRateConfig(timing);
    can.setMBFilter(MB0, 0x010, 0x7FF);  // CONTROL_BROADCAST
    can.setMBFilter(MB1, 0x011, 0x7FF);  // TEST_SEQ
    can.enableMBInterrupts();
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    uint32_t now = millis();
    processCan();
    checkButton();
    sendHeartbeat(now);
    updateLed(now);
}
