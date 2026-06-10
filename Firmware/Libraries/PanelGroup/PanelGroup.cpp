#ifdef ARDUINO_ARCH_STM32

#include "PanelGroup.h"
#include <STM32Board.h>

// ── Static linked-list roots ──────────────────────────────────────────────────
OpenSkyhawk::OutputBase* OpenSkyhawk::OutputBase::first = nullptr;
OpenSkyhawk::InputBase*  OpenSkyhawk::InputBase::first  = nullptr;

OpenSkyhawk::OutputBase::OutputBase() : next(first) { first = this; }
OpenSkyhawk::InputBase::InputBase()   : next(first) { first = this; }

// ── OpenSkyhawk::LED ──────────────────────────────────────────────────────────
OpenSkyhawk::LED::LED(uint16_t addr, uint16_t mask, uint8_t pin)
    : addr_(addr), mask_(mask), pin_(pin) {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
}

void OpenSkyhawk::LED::onPacket(uint16_t controlId, uint16_t value) {
    if (controlId != addr_) return;
    digitalWrite(pin_, (value & mask_) ? HIGH : LOW);
}

// ── OpenSkyhawk::IntegerOutput ────────────────────────────────────────────────
OpenSkyhawk::IntegerOutput::IntegerOutput(uint16_t addr, void (*cb)(uint16_t))
    : addr_(addr), cb_(cb) {}

void OpenSkyhawk::IntegerOutput::onPacket(uint16_t controlId, uint16_t value) {
    if (controlId != addr_) return;
    cb_(value);
}

// ── OpenSkyhawk::Switch2Pos ───────────────────────────────────────────────────
static constexpr uint32_t DEBOUNCE_MS = 20;

OpenSkyhawk::Switch2Pos::Switch2Pos(uint16_t addr, uint8_t pin)
    : addr_(addr), pin_(pin), debounceMs_(0) {
    pinMode(pin_, INPUT_PULLUP);
    lastRaw_    = digitalRead(pin_);
    lastStable_ = lastRaw_;
}

void OpenSkyhawk::Switch2Pos::poll() {
    bool raw = digitalRead(pin_);
    uint32_t now = millis();
    if (raw != lastRaw_) {
        debounceMs_ = now;
        lastRaw_    = raw;
    }
    if ((now - debounceMs_) >= DEBOUNCE_MS && lastRaw_ != lastStable_) {
        lastStable_ = lastRaw_;
        PanelGroup::sendEvent(addr_, lastStable_ == LOW ? 1 : 0);
    }
}

// ── PanelGroup internals ──────────────────────────────────────────────────────
namespace {
    static uint8_t  _nodeId    = 1;
    static uint32_t _hbTxId    = CAN_ID_HB_1;
    static uint32_t _evtTxId   = CAN_ID_EVT_1;
    static uint32_t _echoTxId  = CAN_ID_ECHO_1;
    static uint32_t _rxCount   = 0;
    static uint32_t _startMs   = 0;
    static uint32_t _lastHbMs  = 0;

    void sendHeartbeat(uint32_t now) {
        if (now - _lastHbMs < 500) return;
        _lastHbMs = now;

        uint32_t esr32 = CAN1->ESR;
        uint8_t  tec   = (esr32 >> 16) & 0xFF;
        uint8_t  rec   = (esr32 >> 24) & 0xFF;
        bool     boff  = (esr32 >> 2) & 1;
        bool     epvf  = (esr32 >> 1) & 1;

        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[HB] node="));  d.print(_nodeId);
            d.print(F(" TEC="));       d.print(tec);
            d.print(F(" REC="));       d.print(rec);
            d.print(F(" BOFF="));      d.print(boff);
            d.print(F(" EPVF="));      d.print(epvf);
            d.print(F(" rx="));        d.println(_rxCount);
        }

        uint8_t  flags  = (boff ? 0x01 : 0) | (epvf ? 0x02 : 0);
        uint16_t uptime = (uint16_t)((now - _startMs) / 1000);
        uint16_t rxc    = (uint16_t)(_rxCount & 0xFFFF);
        uint16_t esr16  = (uint16_t)(esr32 >> 16);

        uint8_t buf[8];
        buf[0] = _nodeId;
        buf[1] = flags;
        memcpy(buf + 2, &uptime, 2);
        memcpy(buf + 4, &rxc,    2);
        memcpy(buf + 6, &esr16,  2);
        STM32Board::canSend(_hbTxId, buf, 8);
    }

    void processCan() {
        static CAN_RxHeaderTypeDef rxHdr;
        uint8_t rxData[8];

        while (HAL_CAN_GetRxFifoFillLevel(STM32Board::canHandle(), CAN_RX_FIFO0) > 0) {
            HAL_CAN_GetRxMessage(STM32Board::canHandle(), CAN_RX_FIFO0, &rxHdr, rxData);

            switch (rxHdr.StdId) {
                case CAN_ID_CTRL_BCAST: {
                    _rxCount++;
                    ControlPacket pkt;
                    memcpy(&pkt, rxData, 4);
                    for (auto* o = OpenSkyhawk::OutputBase::first; o; o = o->next)
                        o->onPacket(pkt.controlId, pkt.value);
                    break;
                }
                case CAN_ID_TEST_SEQ:
                    // Echo all 8 bytes unchanged — preserves the timestamp so the
                    // RP2040 can compute RTT entirely within its own clock domain.
                    STM32Board::canSend(_echoTxId, rxData, 8);
                    break;
            }
        }
    }
}

// ── PanelGroup public API ─────────────────────────────────────────────────────
namespace PanelGroup {

void setup() {
    _startMs = millis();
    STM32Board::begin();

    // Strap pin PA0: HIGH → node_id=1 (tied to 3.3V), LOW → node_id=2 (floating)
    pinMode(PA0, INPUT_PULLDOWN);
    delay(10);
    _nodeId   = digitalRead(PA0) ? 1 : 2;
    _hbTxId   = (_nodeId == 1) ? CAN_ID_HB_1   : CAN_ID_HB_2;
    _evtTxId  = (_nodeId == 1) ? CAN_ID_EVT_1  : CAN_ID_EVT_2;
    _echoTxId = (_nodeId == 1) ? CAN_ID_ECHO_1 : CAN_ID_ECHO_2;

    if (STM32Board::isDebug()) {
        STM32Board::diagSerial().print(F("PanelGroup ready. node_id="));
        STM32Board::diagSerial().println(_nodeId);
    }

    // Accept CTRL_BCAST (0x010) and TEST_SEQ (0x011) only
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
    HAL_CAN_ConfigFilter(STM32Board::canHandle(), &filter);

    STM32Board::canStart();
    STM32Board::log("CAN ready.");
}

void loop() {
    uint32_t now = millis();
    STM32Board::tick();
    processCan();
    for (auto* i = OpenSkyhawk::InputBase::first; i; i = i->next)
        i->poll();
    sendHeartbeat(now);
}

bool sendEvent(uint16_t controlId, uint16_t value) {
    uint8_t buf[8] = {};
    memcpy(buf,     &controlId, 2);
    memcpy(buf + 2, &value,     2);
    uint32_t now = millis();
    memcpy(buf + 4, &now,       4);
    return STM32Board::canSend(_evtTxId, buf, 8);
}

uint8_t nodeId() { return _nodeId; }

} // namespace PanelGroup

#endif // ARDUINO_ARCH_STM32
