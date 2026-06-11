#ifdef ARDUINO_ARCH_STM32

#include "PanelBridge.h"
#include <STM32Board.h>

namespace {
    static HardwareSerial* _uart     = nullptr;
    static uint8_t  _uartBuf[4];
    static uint8_t  _uartPos   = 0;
    static uint32_t _ovfCount  = 0;

    static constexpr uint32_t HB_TIMEOUT_MS = 3000;
    static constexpr uint8_t  MAX_NODES    = 2;
    static bool     _nodeAlive[MAX_NODES] = {};
    static uint32_t _lastHbMs[MAX_NODES]  = {};

    static void (*_cbAlive)(uint8_t) = nullptr;
    static void (*_cbDead)(uint8_t)  = nullptr;

    static uint32_t _lastErrMs = 0;
    void monitorErrors(uint32_t now) {
        if (now - _lastErrMs < 1000) return;
        _lastErrMs = now;

        uint8_t tec  = CANProtocol::tec();
        uint8_t rec  = CANProtocol::rec();
        bool    boff = CANProtocol::busOff();
        if (tec > 0 || rec > 0 || boff) {
            if (STM32Board::isDebug()) {
                auto& d = STM32Board::diagSerial();
                d.print(F("[ERRS] TEC=")); d.print(tec);
                d.print(F(" REC="));       d.print(rec);
                d.print(F(" BOFF="));      d.println(boff);
            }
            uint8_t flags = boff ? 0x01 : 0;
            uint8_t errFrame[8] = {DIAG_MAGIC, DIAG_ERR, tec, rec, flags};
            _uart->write(errFrame, 8);
        }

        for (uint8_t i = 0; i < MAX_NODES; i++) {
            if (_nodeAlive[i] && (now - _lastHbMs[i] > HB_TIMEOUT_MS)) {
                _nodeAlive[i] = false;
                uint8_t nodeId = i + 1;
                STM32Board::log("[DEAD] node timeout");
                if (_cbDead) _cbDead(nodeId);
            }
        }
    }

    void processUartPacket(const ControlPacket& pkt) {
        if (pkt.controlId == CTRL_ID_TEST_SEQ) {
            uint8_t buf[8];
            uint32_t seq32 = pkt.value;
            uint32_t now   = millis();
            memcpy(buf,     &seq32, 4);
            memcpy(buf + 4, &now,   4);
            CANProtocol::send(CAN_ID_TEST_SEQ, buf, 8);
            if (STM32Board::isDebug()) {
                STM32Board::diagSerial().print(F("[SEQ] tx seq="));
                STM32Board::diagSerial().println(pkt.value);
            }
        } else {
            CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, pkt);
        }
    }

    void drainUart() {
        while (_uart->available()) {
            _uartBuf[_uartPos++] = _uart->read();
            if (_uartPos < 4) continue;
            _uartPos = 0;

            ControlPacket pkt;
            memcpy(&pkt, _uartBuf, 4);
            if ((pkt.controlId >= 0x0010 && pkt.controlId <= 0x00FF)
                    || pkt.controlId >= 0x8000
                    || pkt.controlId == CTRL_ID_TEST_SEQ) {
                processUartPacket(pkt);
            } else {
                _ovfCount++;
                if (STM32Board::isDebug()) {
                    STM32Board::diagSerial().print(F("[OVF] id=0x"));
                    STM32Board::diagSerial().print(pkt.controlId, HEX);
                    STM32Board::diagSerial().print(F(" total="));
                    STM32Board::diagSerial().println(_ovfCount);
                }
                memmove(_uartBuf, _uartBuf + 1, 3);
                _uartPos = 3;
            }
        }
    }

    void forwardDiagRtt(uint16_t seq16, const uint8_t* rxData) {
        uint8_t rtt[8] = {DIAG_MAGIC, DIAG_RTT};
        memcpy(rtt + 2, &seq16,     2);
        memcpy(rtt + 4, rxData + 4, 4);
        _uart->write(rtt, 8);
    }

    void onCanRx(uint32_t canId, const uint8_t* rxData, uint8_t len) {
        uint32_t now = millis();

        if (canId >= canIdHb(1) && canId <= canIdHb(MAX_NODES)) {
            uint8_t nodeIdx = (uint8_t)(canId - canIdHb(1));
            if (!_nodeAlive[nodeIdx] && _cbAlive) _cbAlive(rxData[0]);
            _nodeAlive[nodeIdx] = true;
            _lastHbMs[nodeIdx]  = now;
            if (STM32Board::isDebug()) {
                uint16_t esr16; memcpy(&esr16, rxData + 6, 2);
                auto& d = STM32Board::diagSerial();
                d.print(F("[HB] node="));  d.print(rxData[0]);
                d.print(F(" flags=0x"));   d.print(rxData[1], HEX);
                d.print(F(" ESR=0x"));     d.println(esr16, HEX);
            }
            uint8_t hb[8] = {DIAG_MAGIC, DIAG_HB, rxData[0], rxData[1]};
            uint16_t rxc; memcpy(&rxc, rxData + 4, 2);
            memcpy(hb + 4, &rxc, 2);
            memcpy(hb + 6, rxData + 6, 2);
            _uart->write(hb, 8);

        } else if (canId >= canIdEvt(1) && canId <= canIdEvt(MAX_NODES)) {
            uint8_t nodeId = (uint8_t)(canId - canIdEvt(0));
            uint16_t controlId, value;
            memcpy(&controlId, rxData,     2);
            memcpy(&value,     rxData + 2, 2);
            if (STM32Board::isDebug()) {
                auto& d = STM32Board::diagSerial();
                d.print(F("[EVT] node=")); d.print(nodeId);
                d.print(F(" ctrl=0x"));   d.print(controlId, HEX);
                d.print(F(" val="));       d.println(value);
            }
            uint8_t evt[8] = {DIAG_MAGIC, DIAG_EVT, 0, 0, 0, 0, nodeId, 0};
            memcpy(evt + 2, &controlId, 2);
            memcpy(evt + 4, &value,     2);
            _uart->write(evt, 8);

        } else if (canId >= canIdEcho(0) && canId <= canIdEcho(MAX_NODES)) {
            // Echo reply from sub-node (or self in loopback via canIdEcho(NODE_ID))
            uint32_t seq32; memcpy(&seq32, rxData, 4);
            uint16_t seq16 = (uint16_t)seq32;
            if (STM32Board::isDebug()) {
                STM32Board::diagSerial().print(F("[ECHO] seq="));
                STM32Board::diagSerial().println(seq16);
            }
            forwardDiagRtt(seq16, rxData);
        }
    }
}

namespace PanelBridge {

void setup(HardwareSerial& uartPort) {
    _uart = &uartPort;
    STM32Board::begin();
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onCanRx);

    _uart->begin(250000);

    CANProtocol::filterAcceptAll();
    CANProtocol::start();
    STM32Board::log("PanelBridge ready. CAN ready.");
}

void loop() {
    uint32_t now = millis();
    STM32Board::tick();
    drainUart();
    CANProtocol::drain();
    monitorErrors(now);
}

void onNodeAlive(void (*cb)(uint8_t nodeId)) { _cbAlive = cb; }
void onNodeDead(void (*cb)(uint8_t nodeId))  { _cbDead  = cb; }

} // namespace PanelBridge

#endif // ARDUINO_ARCH_STM32
