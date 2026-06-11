

# File PanelGroup.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PanelGroup.cpp**](PanelGroup_8cpp.md)

[Go to the documentation of this file](PanelGroup_8cpp.md)


```C++
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
    static uint8_t  _nodeId   = 1;
    static uint32_t _hbTxId   = canIdHb(1);
    static uint32_t _evtTxId  = canIdEvt(1);
    static uint32_t _rxCount  = 0;
    static uint32_t _startMs  = 0;
    static uint32_t _lastHbMs = 0;

    void sendHeartbeat(uint32_t now) {
        if (now - _lastHbMs < 500) return;
        _lastHbMs = now;

        HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(_nodeId, (uint16_t)(_rxCount & 0xFFFF));

        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[HB] node="));   d.print(_nodeId);
            d.print(F(" TEC="));        d.print(hb.esr & 0xFF);
            d.print(F(" REC="));        d.print(hb.esr >> 8);
            d.print(F(" flags=0x"));    d.print(hb.flags, HEX);
            d.print(F(" rx="));         d.println(_rxCount);
        }

        CANProtocol::send(_hbTxId, reinterpret_cast<const uint8_t*>(&hb), sizeof(hb));
    }

    void onCanRx(uint32_t canId, const uint8_t* data, uint8_t len) {
        if (canId != CAN_ID_CTRL_BCAST) return;
        _rxCount++;
        ControlPacketPair pair;
        memcpy(&pair, data, 8);
        auto dispatch = [](const ControlPacket& pkt) {
            if (pkt.controlId == 0x0000) return;
            for (auto* o = OpenSkyhawk::OutputBase::first; o; o = o->next)
                o->onPacket(pkt.controlId, pkt.value);
        };
        dispatch(pair.a);
        dispatch(pair.b);
    }
}

// ── PanelGroup public API ─────────────────────────────────────────────────────
namespace PanelGroup {

void setup() {
    _startMs = millis();
    STM32Board::begin();
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onCanRx);

    // Strap pin PA0: HIGH → node_id=1 (tied to 3.3V), LOW → node_id=2 (floating)
    pinMode(PA0, INPUT_PULLDOWN);
    delay(10);
    _nodeId  = digitalRead(PA0) ? 1 : 2;
    _hbTxId  = canIdHb(_nodeId);
    _evtTxId = canIdEvt(_nodeId);

    if (STM32Board::isDebug()) {
        STM32Board::diagSerial().print(F("PanelGroup ready. node_id="));
        STM32Board::diagSerial().println(_nodeId);
    }

    // CTRL_BCAST, TEST_SEQ, and SYNC_REQ are added by start() automatically.
    CANProtocol::start();
    STM32Board::log("CAN ready.");
}

void loop() {
    uint32_t now = millis();
    STM32Board::tick();
    CANProtocol::drain();
    for (auto* i = OpenSkyhawk::InputBase::first; i; i = i->next)
        i->poll();
    sendHeartbeat(now);
}

bool sendEvent(uint16_t controlId, uint16_t value) {
    uint8_t buf[8] = {};
    memcpy(buf,     &controlId, 2);
    memcpy(buf + 2, &value,     2);
    uint32_t now = millis();
    memcpy(buf + 4, &now, 4);
    CANProtocol::send(_evtTxId, buf, 8);
    return true;
}

uint8_t nodeId() { return _nodeId; }

} // namespace PanelGroup

#endif // ARDUINO_ARCH_STM32
```


