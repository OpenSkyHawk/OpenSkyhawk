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

        uint8_t tec  = STM32Board::tec();
        uint8_t rec  = STM32Board::rec();
        bool    boff = STM32Board::busOff();
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
        if (pkt.controlId == CTRL_TEST_SEQ) {
            uint8_t buf[8];
            uint32_t seq32 = pkt.value;
            uint32_t now   = millis();
            memcpy(buf,     &seq32, 4);
            memcpy(buf + 4, &now,   4);
            STM32Board::canSend(CAN_ID_TEST_SEQ, buf, 8);
            if (STM32Board::isDebug()) {
                STM32Board::diagSerial().print(F("[SEQ] tx seq="));
                STM32Board::diagSerial().println(pkt.value);
            }
        } else {
            uint8_t buf[4];
            memcpy(buf, &pkt, 4);
            STM32Board::canSend(CAN_ID_CTRL_BCAST, buf, 4);
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
                    || pkt.controlId == CTRL_TEST_SEQ) {
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

    void processCan() {
        static CAN_RxHeaderTypeDef rxHdr;
        uint8_t rxData[8];
        uint32_t now = millis();

        while (HAL_CAN_GetRxFifoFillLevel(STM32Board::canHandle(), CAN_RX_FIFO0) > 0) {
            HAL_CAN_GetRxMessage(STM32Board::canHandle(), CAN_RX_FIFO0, &rxHdr, rxData);

            switch (rxHdr.StdId) {
                case CAN_ID_TEST_SEQ: {
                    // Fires in loopback mode only (no sub-node present).
                    uint32_t seq32; memcpy(&seq32, rxData, 4);
                    uint16_t seq16 = (uint16_t)seq32;
                    if (STM32Board::isDebug()) {
                        STM32Board::diagSerial().print(F("[LOOPBACK] seq="));
                        STM32Board::diagSerial().println(seq16);
                    }
                    forwardDiagRtt(seq16, rxData);
                    break;
                }
                case CAN_ID_HB_1:
                case CAN_ID_HB_2: {
                    uint8_t nodeIdx = (rxHdr.StdId == CAN_ID_HB_1) ? 0 : 1;
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
                    memcpy(hb + 6, rxData + 6, 2); // ESR bytes from sub-node heartbeat
                    _uart->write(hb, 8);
                    break;
                }
                case CAN_ID_EVT_1:
                case CAN_ID_EVT_2: {
                    uint8_t nodeId = (rxHdr.StdId == CAN_ID_EVT_1) ? 1 : 2;
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
                    break;
                }
                case CAN_ID_ECHO_1:
                case CAN_ID_ECHO_2: {
                    uint32_t seq32; memcpy(&seq32, rxData, 4);
                    uint16_t seq16 = (uint16_t)seq32;
                    if (STM32Board::isDebug()) {
                        STM32Board::diagSerial().print(F("[ECHO] seq="));
                        STM32Board::diagSerial().println(seq16);
                    }
                    forwardDiagRtt(seq16, rxData);
                    break;
                }
            }
        }
    }
}

namespace PanelBridge {

void setup(HardwareSerial& uartPort) {
    _uart = &uartPort;
    STM32Board::begin();

    _uart->begin(250000);

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
    HAL_CAN_ConfigFilter(STM32Board::canHandle(), &filter);

    STM32Board::canStart();
    STM32Board::log("PanelBridge ready. CAN ready.");
}

void loop() {
    uint32_t now = millis();
    STM32Board::update();
    drainUart();
    processCan();
    monitorErrors(now);
}

void onNodeAlive(void (*cb)(uint8_t nodeId)) { _cbAlive = cb; }
void onNodeDead(void (*cb)(uint8_t nodeId))  { _cbDead  = cb; }

} // namespace PanelBridge

#endif // ARDUINO_ARCH_STM32
