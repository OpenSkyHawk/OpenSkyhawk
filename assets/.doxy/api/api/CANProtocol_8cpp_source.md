

# File CANProtocol.cpp

[**File List**](files.md) **>** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md) **>** [**CANProtocol.cpp**](CANProtocol_8cpp.md)

[Go to the documentation of this file](CANProtocol_8cpp.md)


```C++
#ifdef ARDUINO_ARCH_STM32

#include "CANProtocol.h"
#include <STM32Board.h>
#include <Arduino.h>
#include <string.h>

// ── Private types ─────────────────────────────────────────────────────────────

struct TxQueueEntry {
    uint32_t canId;
    uint8_t  len;
    uint8_t  data[8];
    uint8_t  attempts;
};

struct RxQueueEntry {
    uint32_t canId;
    uint8_t  len;
    uint8_t  data[8];
};

struct BatchState {
    uint32_t      canId;
    bool          hasA;
    ControlPacket a;
    uint8_t       loopsWaited;
};

// ── Private state ─────────────────────────────────────────────────────────────

static constexpr uint8_t TX_RING_SIZE    = 16;
static constexpr uint8_t RX_RING_SIZE    = 8;
static constexpr uint8_t MAX_FILTER_IDS  = 16;
static constexpr uint8_t MAX_TX_ATTEMPTS = 3;

static TxQueueEntry      _txRing[TX_RING_SIZE];
static volatile uint8_t  _txHead = 0;
static volatile uint8_t  _txTail = 0;

static RxQueueEntry      _rxRing[RX_RING_SIZE];
static volatile uint8_t  _rxHead = 0;
static volatile uint8_t  _rxTail = 0;

static BatchState         _batches[4];

static CanStatusCallback  _statusCb  = nullptr;
static CanSyncReqCallback _syncReqCb = nullptr;
static CanRxCallback      _rxCb      = nullptr;

static CanStatus          _status         = CanStatus::STARTING;
static uint32_t           _txDrops        = 0;
static uint32_t           _filterIds[MAX_FILTER_IDS];
static uint8_t            _filterCount    = 0;
static bool               _filterPassAll  = false;

// ── Private helpers ───────────────────────────────────────────────────────────

// Called from TX-complete ISR — drains the TX ring into available mailboxes.
static void _drainTxQueue() {
    while (_txHead != _txTail) {
        TxQueueEntry& e = _txRing[_txHead];

        if (HAL_CAN_GetTxMailboxesFreeLevel(STM32Board::canHandle()) == 0) {
            break;
        }

        CAN_TxHeaderTypeDef hdr = {};
        hdr.StdId              = e.canId;
        hdr.IDE                = CAN_ID_STD;
        hdr.RTR                = CAN_RTR_DATA;
        hdr.DLC                = e.len;
        hdr.TransmitGlobalTime = DISABLE;

        uint32_t mailbox;
        if (HAL_CAN_AddTxMessage(STM32Board::canHandle(), &hdr, e.data, &mailbox) == HAL_OK) {
            _txHead = (_txHead + 1) % TX_RING_SIZE;
        } else {
            e.attempts++;
            if (e.attempts >= MAX_TX_ATTEMPTS) {
                _txHead = (_txHead + 1) % TX_RING_SIZE;
                _txDrops++;
            }
            break;
        }
    }
}

static void _applyFilters() {
    CAN_HandleTypeDef* hcan = STM32Board::canHandle();

    if (_filterPassAll) {
        CAN_FilterTypeDef f = {};
        f.FilterBank           = 0;
        f.FilterMode           = CAN_FILTERMODE_IDMASK;
        f.FilterScale          = CAN_FILTERSCALE_32BIT;
        f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        f.FilterActivation     = ENABLE;
        HAL_CAN_ConfigFilter(hcan, &f);
        return;
    }

    // Build combined ID list: mandatory + caller-registered
    uint32_t ids[MAX_FILTER_IDS + 3];
    uint8_t  count = 0;
    ids[count++] = CAN_ID_CTRL_BCAST;
    ids[count++] = CAN_ID_TEST_SEQ;
    ids[count++] = CAN_ID_SYNC_REQ;
    for (uint8_t i = 0; i < _filterCount; i++) {
        ids[count++] = _filterIds[i];
    }

    // 16-bit IDLIST: 4 IDs per filter bank (standard frame ID shifted left 5)
    // Pad incomplete banks with a duplicate of ids[0] — harmless extra accept.
    uint8_t banks = (count + 3) / 4;
    for (uint8_t b = 0; b < banks; b++) {
        uint8_t  base = b * 4;
        uint32_t i0   = ids[base + 0];
        uint32_t i1   = (count > base + 1) ? ids[base + 1] : ids[0];
        uint32_t i2   = (count > base + 2) ? ids[base + 2] : ids[0];
        uint32_t i3   = (count > base + 3) ? ids[base + 3] : ids[0];

        CAN_FilterTypeDef f = {};
        f.FilterBank           = b;
        f.FilterMode           = CAN_FILTERMODE_IDLIST;
        f.FilterScale          = CAN_FILTERSCALE_16BIT;
        f.FilterIdHigh         = (uint16_t)(i0 << 5);
        f.FilterIdLow          = (uint16_t)(i1 << 5);
        f.FilterMaskIdHigh     = (uint16_t)(i2 << 5);
        f.FilterMaskIdLow      = (uint16_t)(i3 << 5);
        f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        f.FilterActivation     = ENABLE;
        HAL_CAN_ConfigFilter(hcan, &f);
    }
}

static void _updateStatus() {
    uint32_t esr  = CAN1->ESR;
    bool     boff = (esr >> 2) & 1;
    uint8_t  t    = (esr >> 16) & 0xFF;

    CanStatus s;
    if (boff)   s = CanStatus::BUS_OFF;
    else if (t) s = CanStatus::TX_ERROR;
    else        s = CanStatus::NORMAL;

    if (s != _status) {
        _status = s;
        if (_statusCb) _statusCb(_status);
    }
}

static void _enqueueRxFrame(uint32_t canId, uint8_t len, const uint8_t* data) {
    uint8_t next = (_rxTail + 1) % RX_RING_SIZE;
    if (next == _rxHead) return;  // software RX ring full — drop

    _rxRing[_rxTail].canId = canId;
    _rxRing[_rxTail].len   = len;
    memcpy(_rxRing[_rxTail].data, data, len);
    _rxTail = next;
}

// Safe to call from main-loop context — guards against TX-complete ISR preemption.
static void _drainTxQueueFromMain() {
    uint32_t m = __get_PRIMASK();
    __disable_irq();
    _drainTxQueue();
    if (!m) __enable_irq();
}

static void _pollRxFifo0() {
    CAN_HandleTypeDef* hcan = STM32Board::canHandle();

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0) {
        CAN_RxHeaderTypeDef hdr;
        uint8_t data[8] = {};
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, data) != HAL_OK) break;
        _enqueueRxFrame(hdr.StdId, hdr.DLC, data);
    }
    if (!primask) __enable_irq();
}

static void _startInternal(uint32_t mode) {
    CAN_HandleTypeDef* hcan = STM32Board::canHandle();
    hcan->Init.Mode = mode;
    HAL_CAN_Init(hcan);

    _applyFilters();

    HAL_CAN_Start(hcan);
    HAL_CAN_ActivateNotification(hcan,
        CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY);

    _batches[0] = { CAN_ID_CTRL_BCAST,    false, {0, 0}, 0 };
    _batches[1] = { canIdEvt(NODE_ID),    false, {0, 0}, 0 };
    _batches[2] = { canIdEvtRel(NODE_ID), false, {0, 0}, 0 };  // RotaryEncoder REL
    _batches[3] = { canIdEvtDir(NODE_ID), false, {0, 0}, 0 };  // RotaryEncoder DIR

    _status = CanStatus::NORMAL;
    if (_statusCb) _statusCb(_status);
}

// ── HAL ISR callbacks (weak-symbol overrides) ─────────────────────────────────

extern "C" void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef*) { _drainTxQueue(); }
extern "C" void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef*) { _drainTxQueue(); }
extern "C" void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef*) { _drainTxQueue(); }

extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
    CAN_RxHeaderTypeDef hdr;
    uint8_t data[8] = {};
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, data) != HAL_OK) return;

    _enqueueRxFrame(hdr.StdId, hdr.DLC, data);
}

// ── Public API ────────────────────────────────────────────────────────────────

namespace CANProtocol {

void filterAcceptAll() { _filterPassAll = true; }

void filterAcceptId(uint32_t canId) {
    if (_filterCount < MAX_FILTER_IDS) {
        _filterIds[_filterCount++] = canId;
    }
}

void start()         { _startInternal(CAN_MODE_NORMAL); }
void startLoopback() { _startInternal(CAN_MODE_SILENT_LOOPBACK); }

void drain() {
    _drainTxQueueFromMain();
    _pollRxFifo0();

    // Drain RX ring — copy out before advancing head so ISR can't clobber in-flight frame
    while (_rxHead != _rxTail) {
        RxQueueEntry frame = _rxRing[_rxHead];
        _rxHead = (_rxHead + 1) % RX_RING_SIZE;

        if (frame.canId == CAN_ID_SYNC_REQ) {
            if (_syncReqCb) _syncReqCb();
        } else if (frame.canId == CAN_ID_TEST_SEQ) {
            send(canIdEcho(NODE_ID), frame.data, frame.len);
        } else {
            if (_rxCb) _rxCb(frame.canId, frame.data, frame.len);
        }
    }

    _drainTxQueueFromMain();

    // Service batch deadlines
    for (auto& b : _batches) {
        if (b.hasA) {
            b.loopsWaited++;
            if (b.loopsWaited >= 2) {
                ControlPacketPair pair;
                pair.a = b.a;
                pair.b = { 0x0000, 0 };
                send(b.canId, reinterpret_cast<const uint8_t*>(&pair), 8);
                b.hasA       = false;
                b.loopsWaited = 0;
            }
        }
    }

    _drainTxQueueFromMain();

    _updateStatus();
}

void send(uint32_t canId, const uint8_t* data, uint8_t len) {
    CAN_TxHeaderTypeDef hdr = {};
    hdr.StdId              = canId;
    hdr.IDE                = CAN_ID_STD;
    hdr.RTR                = CAN_RTR_DATA;
    hdr.DLC                = len;
    hdr.TransmitGlobalTime = DISABLE;

    uint32_t mailbox;
    if (HAL_CAN_AddTxMessage(STM32Board::canHandle(), &hdr,
                              const_cast<uint8_t*>(data), &mailbox) == HAL_OK) {
        return;
    }

    // All mailboxes busy — enqueue in TX ring (protect with critical section)
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (canId == CAN_ID_CTRL_BCAST) {
        // Coalesce: replace existing CTRL_BCAST in ring rather than appending stale state
        uint8_t i = _txHead;
        while (i != _txTail) {
            if (_txRing[i].canId == CAN_ID_CTRL_BCAST) {
                memcpy(_txRing[i].data, data, len);
                _txRing[i].len = len;
                if (!primask) __enable_irq();
                return;
            }
            i = (i + 1) % TX_RING_SIZE;
        }
    }

    uint8_t next = (_txTail + 1) % TX_RING_SIZE;
    if (next == _txHead) {
        _txDrops++;
        if (!primask) __enable_irq();
        return;
    }

    _txRing[_txTail].canId    = canId;
    _txRing[_txTail].len      = len;
    _txRing[_txTail].attempts = 0;
    memcpy(_txRing[_txTail].data, data, len);
    _txTail = next;

    if (!primask) __enable_irq();
}

void sendBatched(uint32_t canId, const ControlPacket& pkt) {
    BatchState* b = nullptr;
    for (auto& bs : _batches) {
        if (bs.canId == canId) { b = &bs; break; }
    }
    if (!b) return;

    if (!b->hasA) {
        b->a          = pkt;
        b->hasA       = true;
        b->loopsWaited = 0;
    } else {
        ControlPacketPair pair;
        pair.a = b->a;
        pair.b = pkt;
        send(canId, reinterpret_cast<const uint8_t*>(&pair), 8);
        b->hasA       = false;
        b->loopsWaited = 0;
    }
}

void flushBatched(uint32_t canId) {
    for (auto& b : _batches) {
        if (b.canId == canId && b.hasA) {
            ControlPacketPair pair;
            pair.a = b.a;
            pair.b = { 0x0000, 0 };
            send(canId, reinterpret_cast<const uint8_t*>(&pair), 8);
            b.hasA       = false;
            b.loopsWaited = 0;
            return;
        }
    }
}

void onStatusChange(CanStatusCallback cb) {
    _statusCb = cb;
    if (cb) cb(_status);  // fire immediately so caller learns current state
}

void onSyncReq(CanSyncReqCallback cb) { _syncReqCb = cb; }
void onReceive(CanRxCallback cb)      { _rxCb      = cb; }

uint8_t tec()    { return (CAN1->ESR >> 16) & 0xFF; }
uint8_t rec()    { return (CAN1->ESR >> 24) & 0xFF; }
bool    busOff() { return (CAN1->ESR >> 2)  & 1; }

HeartbeatPayload makeHeartbeatPayload(uint8_t nodeId, uint16_t rxCount) {
    HeartbeatPayload p;
    uint32_t esr = CAN1->ESR;
    p.nodeId  = nodeId;
    p.flags   = (uint8_t)(((esr >> 2) & 0x01) | (esr & 0x02));  // bit0=BOFF, bit1=EPVF
    p.uptime  = (uint16_t)(millis() / 1000);
    p.rxCount = rxCount;
    p.esr     = (uint16_t)(esr >> 16);  // low byte=TEC, high byte=REC
    return p;
}

NodeHealthPayload makeNodeHealthPayload(uint8_t nodeId, int8_t dieTempC) {
    NodeHealthPayload p;
    p.nodeId   = nodeId;
    p.dieTempC = dieTempC;
    p.flags    = 0;
#ifdef NODE_OVERHEAT_C
    // Overheat trip is opt-in: the internal sensor is uncalibrated, so a sane threshold
    // needs field data first. Only compute the flag when a build defines NODE_OVERHEAT_C.
    if (dieTempC != INT8_MIN && dieTempC >= (int)(NODE_OVERHEAT_C)) p.flags |= 0x01;  // bit0=overheat
#endif
    p.faultMask = 0;  // reserved for #163 (degraded/fault rollup)
    p.faultId   = 0;  // reserved for #163
    p.rsvd[0] = p.rsvd[1] = p.rsvd[2] = 0;
    return p;
}

uint32_t txDropCount() { return _txDrops; }

} // namespace CANProtocol

#endif // ARDUINO_ARCH_STM32
```


