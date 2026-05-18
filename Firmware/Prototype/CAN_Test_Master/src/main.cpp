// CAN_Test_Master — STM32F103CBT6
//
// UART2 (PA2 TX / PA3 RX) @ 250000  — ControlPacket structs from Arduino / RP2040
// CAN1  (PA11 RX / PA12 TX)         — SN65HVD230 transceiver, 500 kbps
// UART1 (PA9 TX only) @ 115200       — diagnostic tap → USB-TTL adapter
//
// Timing: 8 MHz crystal, 72 MHz sysclock, APB1 36 MHz
// CAN 500 kbps: prescaler=4, BS1=13, BS2=4, SJW=1, sample point 77.8%
//
// CAN message IDs:
//   0x010  TX  CONTROL_BROADCAST  4B: controlId(u16) + value(u16)
//   0x011  TX  TEST_SEQ           8B: seq(u32) + tx_ms(u32)
//   0x100  RX  HEARTBEAT_1        8B: node_id(u8)+flags(u8)+uptime(u16)+rx_count(u16)+ESR(u16)
//   0x101  RX  HEARTBEAT_2        same
//   0x200  RX  INPUT_EVENT_1      8B: controlId(u16)+value(u16)+ts_ms(u32)
//   0x201  RX  INPUT_EVENT_2      same
//   0x210  RX  SEQ_ECHO_1         8B: seq(u32)+rx_ms(u32)
//   0x211  RX  SEQ_ECHO_2         same
//
// Diagnostic UART framing (→ USB-TTL adapter on PA9):
//   plain ASCII lines, prefixed [ERRS] [TXFULL] [OVF] [HB] [RTT] [EVT]

#include <Arduino.h>
#include <STM32_CAN.h>

// ── peripherals ───────────────────────────────────────────────────────────────
STM32_CAN can(CAN1, ALT);  // ALT = PA11/PA12 (default for F103)

HardwareSerial DiagSerial(PA9, PA10);  // UART1, TX-only tap

// ── packet definitions ────────────────────────────────────────────────────────
struct __attribute__((packed)) ControlPacket {
    uint16_t controlId;
    uint16_t value;
};

struct DiagOut {
    uint8_t magic;    // 0xAA
    uint8_t type;
    uint8_t payload[6];
};
static constexpr uint8_t DIAG_MAGIC = 0xAA;
static constexpr uint8_t DIAG_RTT   = 0x01;
static constexpr uint8_t DIAG_HB    = 0x02;
static constexpr uint8_t DIAG_ERR   = 0x03;

// ── CAN message IDs ───────────────────────────────────────────────────────────
static constexpr uint32_t ID_CTRL_BCAST  = 0x010;
static constexpr uint32_t ID_TEST_SEQ    = 0x011;
static constexpr uint32_t ID_HB_1        = 0x100;
static constexpr uint32_t ID_HB_2        = 0x101;
static constexpr uint32_t ID_EVT_1       = 0x200;
static constexpr uint32_t ID_EVT_2       = 0x201;
static constexpr uint32_t ID_ECHO_1      = 0x210;
static constexpr uint32_t ID_ECHO_2      = 0x211;

// ── node state ────────────────────────────────────────────────────────────────
static constexpr uint8_t  NUM_NODES = 2;
static constexpr uint32_t HB_TIMEOUT_MS = 3000;

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
static uint8_t  uartBuf[8];
static uint8_t  uartPos = 0;

// ── LED (PC13 active-low) ─────────────────────────────────────────────────────
static constexpr uint8_t LED_PIN = PC13;

// ── scope trigger (optional) ──────────────────────────────────────────────────
static constexpr uint8_t SCOPE_PIN = PB0;

// ── helpers ───────────────────────────────────────────────────────────────────
static void scopePulse() {
    digitalWrite(SCOPE_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(SCOPE_PIN, LOW);
}

static bool canTx(uint32_t id, const uint8_t* data, uint8_t len) {
    CAN_message_t msg;
    msg.id  = id;
    msg.len = len;
    memcpy(msg.buf, data, len);
    if (!can.write(msg)) {
        txFullCount++;
        DiagSerial.print(F("[TXFULL] total="));
        DiagSerial.println(txFullCount);
        return false;
    }
    scopePulse();
    return true;
}

static void sendDiagToArduino(uint8_t type, const uint8_t* payload6) {
    // Send structured 8-byte diag packet back over UART2 (Serial) to Arduino
    uint8_t pkt[8] = {DIAG_MAGIC, type};
    memcpy(pkt + 2, payload6, 6);
    Serial.write(pkt, 8);
}

// ── UART packet processing ────────────────────────────────────────────────────
static void processUartPacket(const ControlPacket& pkt) {
    if (pkt.controlId == 0xFFFF) {
        // TEST_SEQ — broadcast as 0x011, 8 bytes: seq(u16 padded to u32) + tx_ms
        uint8_t buf[8];
        uint32_t seq32 = pkt.value;
        uint32_t now   = millis();
        memcpy(buf,     &seq32, 4);
        memcpy(buf + 4, &now,   4);
        canTx(ID_TEST_SEQ, buf, 8);
        DiagSerial.print(F("[SEQ] tx seq="));
        DiagSerial.println(pkt.value);
    } else {
        // Normal ControlPacket → CONTROL_BROADCAST
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
            // Alignment check: reject obviously invalid controlId range
            if (pkt.controlId <= 0x00FF || pkt.controlId == 0xFFFF || pkt.controlId == 0xFFFE) {
                processUartPacket(pkt);
            } else {
                // Misalignment — discard first byte and retry with shifted window
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

// ── CAN receive ───────────────────────────────────────────────────────────────
static void processCan() {
    CAN_message_t msg;
    while (can.read(msg)) {
        uint32_t now = millis();
        switch (msg.id) {
            case ID_HB_1:
            case ID_HB_2: {
                uint8_t nodeIdx = (msg.id == ID_HB_1) ? 0 : 1;
                NodeState& n = nodes[nodeIdx];
                n.alive     = true;
                n.lastHbMs  = now;
                n.flags     = msg.buf[1];
                memcpy(&n.rxCount, msg.buf + 4, 2);
                memcpy(&n.esr,     msg.buf + 6, 2);

                // Human-readable diag
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
                sendDiagToArduino(DIAG_HB, pl);
                break;
            }
            case ID_EVT_1:
            case ID_EVT_2: {
                uint16_t ctrlId, val;
                memcpy(&ctrlId, msg.buf,     2);
                memcpy(&val,    msg.buf + 2, 2);
                DiagSerial.print(F("[EVT] node="));
                DiagSerial.print((msg.id == ID_EVT_1) ? 1 : 2);
                DiagSerial.print(F(" ctrl=0x"));
                DiagSerial.print(ctrlId, HEX);
                DiagSerial.print(F(" val="));
                DiagSerial.println(val);
                break;
            }
            case ID_ECHO_1:
            case ID_ECHO_2: {
                uint32_t seq, rxMs;
                memcpy(&seq,  msg.buf,     4);
                memcpy(&rxMs, msg.buf + 4, 4);
                uint32_t rtt = now - rxMs;  // approximate; rxMs is sub-node millis
                DiagSerial.print(F("[RTT] seq="));
                DiagSerial.print((uint16_t)seq);
                DiagSerial.print(F(" ~rtt="));
                DiagSerial.print(rtt);
                DiagSerial.println(F("ms"));

                // Forward RTT to Arduino
                uint8_t pl[6];
                uint16_t seq16 = (uint16_t)seq;
                memcpy(pl,     &seq16, 2);
                memcpy(pl + 2, &rxMs,  4);
                sendDiagToArduino(DIAG_RTT, pl);
                break;
            }
        }
    }
}

// ── node watchdog + error monitoring ─────────────────────────────────────────
static uint32_t lastErrReport = 0;
static uint32_t lastLedMs     = 0;
static bool     ledState       = false;
static bool     busOff         = false;

static void monitorErrors(uint32_t now) {
    if (now - lastErrReport < 1000) return;
    lastErrReport = now;

    uint32_t esr = CAN1->ESR;
    uint8_t  tec = (esr >> 24) & 0xFF;
    uint8_t  rec = (esr >> 16) & 0xFF;
    uint32_t msr = CAN1->MSR;
    uint8_t  flags = 0;
    if (msr & (1 << 2)) flags |= 0x01;  // BOFF
    if (msr & (1 << 1)) flags |= 0x02;  // EPVF
    if (msr & (1 << 0)) flags |= 0x04;  // EWGF
    busOff = (flags & 0x01);

    if (tec > 0 || rec > 0 || flags) {
        DiagSerial.print(F("[ERRS] TEC="));
        DiagSerial.print(tec);
        DiagSerial.print(F(" REC="));
        DiagSerial.print(rec);
        DiagSerial.print(F(" flags=0x"));
        DiagSerial.println(flags, HEX);

        uint8_t pl[6] = {tec, rec, flags, 0, 0, 0};
        sendDiagToArduino(DIAG_ERR, pl);
    }

    // Node liveness watchdog
    for (uint8_t i = 0; i < NUM_NODES; i++) {
        if (nodes[i].alive && (now - nodes[i].lastHbMs > HB_TIMEOUT_MS)) {
            nodes[i].alive = false;
            DiagSerial.print(F("[DEAD] node="));
            DiagSerial.println(i + 1);
        }
    }
}

static void updateLed(uint32_t now) {
    if (busOff) {
        digitalWrite(LED_PIN, LOW);  // solid on (active-low)
        return;
    }
    bool anyDead = (!nodes[0].alive || !nodes[1].alive);
    uint32_t period = anyDead ? 100 : 500;  // fast=100ms, slow=500ms
    if (now - lastLedMs >= period) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastLedMs = now;
    }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
    pinMode(LED_PIN,   OUTPUT); digitalWrite(LED_PIN, HIGH);  // off
    pinMode(SCOPE_PIN, OUTPUT); digitalWrite(SCOPE_PIN, LOW);

    // Diagnostic UART on PA9 (TX-only)
    DiagSerial.begin(115200);
    DiagSerial.println(F("CAN_Test_Master starting..."));

    // DCS-BIOS / test UART on PA2/PA3
    Serial.begin(250000);

    // CAN — 500 kbps, 8 MHz crystal, APB1 36 MHz
    // prescaler=4, BS1=13, BS2=4, SJW=1 → exactly 500 kbps, SP 77.8%
    CAN_bit_timing_config_t timing = {4, 13, 4};
    can.begin();
    can.setBaudRateConfig(timing);
    can.setMBFilter(ACCEPT_ALL);
    can.enableMBInterrupts();

    DiagSerial.println(F("CAN_Test_Master ready. Waiting for packets."));
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    uint32_t now = millis();
    drainUart();
    processCan();
    monitorErrors(now);
    updateLed(now);
}
