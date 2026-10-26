#ifdef ARDUINO_ARCH_STM32

#include "CANProtocol.h"
#include <STM32Board.h>
#include <FirmwareVersion.h>  // OPENSKYHAWK_FW_* — the release version READY_n carries (#295)
#include <BootProtocol.h>     // BOOT_CMD_n / CONNECT — entering the bootloader (#365)
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

// The bridge sends the whole sim export stream as CTRL_BCAST, in bursts (one per DCS-BIOS frame):
// it gets the deep TX ring (#389). Override with -DCAN_TX_RING_SIZE=N (2-255).
#ifndef CAN_TX_RING_SIZE
#define CAN_TX_RING_SIZE ((NODE_ID) == GATEWAY_NODE_ID ? 64 : 16)
#endif
static constexpr uint8_t TX_RING_SIZE    = CAN_TX_RING_SIZE;
// The bridge hears every node, so it gets the deep ring; a node hears only the bridge (#387).
// Override with -DCAN_RX_RING_SIZE=N (2-255).
#ifndef CAN_RX_RING_SIZE
#define CAN_RX_RING_SIZE ((NODE_ID) == GATEWAY_NODE_ID ? 128 : 8)
#endif
static constexpr uint8_t RX_RING_SIZE    = CAN_RX_RING_SIZE;
static constexpr uint8_t MAX_FILTER_IDS  = 16;
static constexpr uint8_t MAX_TX_ATTEMPTS = 3;

static TxQueueEntry      _txRing[TX_RING_SIZE];
static volatile uint8_t  _txHead = 0;
static volatile uint8_t  _txTail = 0;

static RxQueueEntry      _rxRing[RX_RING_SIZE];
static volatile uint8_t  _rxHead = 0;
static volatile uint8_t  _rxTail = 0;

// One slot per batched frame ID. Sized to match the registrations in begin() — sendBatched()
// silently discards a frame with no slot here, so adding a batched frame means growing this
// array AND registering it below.
static BatchState         _batches[5];

static CanStatusCallback  _statusCb  = nullptr;
static CanSyncReqCallback _syncReqCb = nullptr;
static bool               _readySent = false;   // the boot READY is out: answer SYNC_REQ with READY (#383)
static bool               _resyncPending = false;   // answered a SYNC_REQ: READY(RESYNC) once TX is idle (#387)
static uint32_t           _resyncSinceMs = 0;
static uint32_t           _rxDrops = 0;
static CanRxCallback      _rxCb      = nullptr;

static CanStatus          _status         = CanStatus::STARTING;
static uint32_t           _txDrops        = 0;
static uint32_t           _filterIds[MAX_FILTER_IDS];
static uint8_t            _filterCount    = 0;
static bool               _filterPassAll  = false;
static bool               _loopback       = false;  // started with startLoopback(): own frames return

// Node status service (serviceNodeStatus) — #340
static constexpr uint32_t HB_PERIOD_MS     = 500;
static constexpr uint32_t HEALTH_PERIOD_MS = 1000;
static uint32_t           _lastHbMs        = 0;
static uint32_t           _lastHealthMs    = 0;
static bool               _statusArmed     = false;  // timers armed on the first service call
static uint32_t           _lastMasterMs    = 0;      // millis() of the last HB_0 seen
static bool               _everSawMaster   = false;  // arm the timeout only after the first HB_0
static bool               _masterHbPending = false;  // HB_0 arrived since the last service call
// WARNING causes this service owns on a non-bridge node. The LED has one shared latch, so the service
// keeps its causes apart and only writes the latch when their OR changes — a returning master clears
// master loss, never an overheat that is still true.
static bool               _masterLost      = false;
static bool               _overheat        = false;
static bool               _warnAsserted    = false;  // what this service last wrote to the latch
// Duplicate NODE_ID check (#344): a frame on our own HB_n came from another board with our NODE_ID.
static uint32_t           _lastDupMs       = 0;      // millis() of the last foreign own-ID HB
static bool               _dupSeen         = false;  // a foreign own-ID HB within DUP_NODE_TIMEOUT_MS
static uint8_t            _ownHbLoopPending = 0;     // loopback only: own HBs sent, not yet looped back
// Last HEALTH_n built (lastNodeHealth()) — not-yet-seen until the first one.
static NodeHealthPayload  _lastHealth      = { 0, DIE_TEMP_UNAVAILABLE, 0, 0, 0, 0, { 0, 0 } };

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
    uint32_t ids[MAX_FILTER_IDS + 6];
    uint8_t  count = 0;
    ids[count++] = CAN_ID_CTRL_BCAST;
    ids[count++] = CAN_ID_TEST_SEQ;
    ids[count++] = CAN_ID_SYNC_REQ;
    if (NODE_ID != GATEWAY_NODE_ID) ids[count++] = canIdHb(GATEWAY_NODE_ID);  // master heartbeat → serviceNodeStatus watchdog
    ids[count++] = canIdHb(NODE_ID);  // own heartbeat ID → duplicate NODE_ID check (#344)
    if (NODE_ID != GATEWAY_NODE_ID) ids[count++] = OpenSkyhawk::Boot::canIdBootCmd(NODE_ID);  // enter request (#365)
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
    if (next == _rxHead) { _rxDrops++; return; }  // software RX ring full — drop, counted (#387)

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
    _batches[4] = { canIdEvtAction(NODE_ID), false, {0, 0}, 0 };  // ActionButton TOGGLE

    // serviceNodeStatus() arms its HB/HEALTH timers on its first call — the first loop() after
    // setup returns — so a restart re-arms them there too (FirmwarePlan 09 step 8).
    _statusArmed = false;
    _loopback         = (mode == CAN_MODE_SILENT_LOOPBACK);
    _ownHbLoopPending = 0;
    _dupSeen          = false;

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

void start() {
    // A clock fault means the CAN bit timing (tuned for 36 MHz APB1) is wrong for the
    // fallback clock, so transmitting would corrupt the live bus. Come up listen-only
    // (silent: RX + error monitoring, TX pin held recessive, no ACK) so a faulted node
    // stays harmless while still signalling via the WARNING LED / diag. See issue #245.
    if (STM32Board::clockFault()) {
        STM32Board::log("[CAN] clock fault -> listen-only (silent) mode, not transmitting");
        _startInternal(CAN_MODE_SILENT);
        return;
    }
    _startInternal(CAN_MODE_NORMAL);
}
void startLoopback() { _startInternal(CAN_MODE_SILENT_LOOPBACK); }

void drain() {
    _drainTxQueueFromMain();
    _pollRxFifo0();

    // The end of our answer to a SYNC_REQ (#387): READY(RESYNC) once everything the answer queued
    // has left — the transmit ring empty and all three mailboxes free, so it can't overtake a
    // higher-priority frame of the burst. 50 ms cap: never hold it back for good.
    if (_resyncPending) {
        const bool idle = _txHead == _txTail
                       && HAL_CAN_GetTxMailboxesFreeLevel(STM32Board::canHandle()) == 3;
        if (idle || (uint32_t)(millis() - _resyncSinceMs) >= 50) {
            _resyncPending = false;
            const ReadyPayload p = makeReadyPayload(static_cast<uint8_t>(ReadyFlag::RESYNC));
            send(canIdReady(NODE_ID), reinterpret_cast<const uint8_t*>(&p), sizeof(p));
        }
    }

    // Drain RX ring — copy out before advancing head so ISR can't clobber in-flight frame
    while (_rxHead != _rxTail) {
        RxQueueEntry frame = _rxRing[_rxHead];
        _rxHead = (_rxHead + 1) % RX_RING_SIZE;

        if (frame.canId == CAN_ID_SYNC_REQ) {
            // For another node (#387): not ours to answer.
            if (frame.len >= SYNC_REQ_LEN_TARGETED && frame.data[0] != NODE_ID) continue;
            if (_syncReqCb) _syncReqCb();
            // Then READY(RESYNC) once the answer has gone out (top of drain()): it re-announces our
            // version to a bridge that booted after us (#383) and tells it we're done (#387). Not
            // before our own boot READY.
            if (_readySent) { _resyncPending = true; _resyncSinceMs = millis(); }
        } else if (frame.canId == CAN_ID_TEST_SEQ) {
            send(canIdEcho(NODE_ID), frame.data, frame.len);
        } else if (NODE_ID != GATEWAY_NODE_ID && frame.canId == OpenSkyhawk::Boot::canIdBootCmd(NODE_ID)) {
            // A host asking for this node's bootloader (#365): reset into it. Without one installed
            // enterBootloader() returns and the request is ignored — a reset would only restart us.
            if (frame.len && frame.data[0] == uint8_t(OpenSkyhawk::Boot::Cmd::CONNECT))
                STM32Board::enterBootloader(OpenSkyhawk::Boot::TRANSPORT_CAN);
        } else {
            if (frame.canId == canIdHb(NODE_ID)) {
                // Our own heartbeat ID. A node never receives its own frames, so another board has
                // our NODE_ID (#344) — except in loopback, where our own heartbeats come back.
                if (_loopback && _ownHbLoopPending > 0) {
                    _ownHbLoopPending--;
                } else {
                    _lastDupMs = millis();
                    _dupSeen   = true;
                }
            } else if (NODE_ID != GATEWAY_NODE_ID && frame.canId == canIdHb(GATEWAY_NODE_ID)) {
                // Master heartbeat: stamp it for serviceNodeStatus()'s watchdog, then still hand it
                // to the node's own handler like any other frame.
                _lastMasterMs    = millis();
                _everSawMaster   = true;
                _masterHbPending = true;
            }
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

    // No CTRL_BCAST coalescing (#389): a frame carries two of ~250 controls, not the whole state, so
    // replacing a queued one with the next dropped two other controls' updates — ~22% of the sim's
    // output under a real export stream. Every frame queues; the bridge's ring is deep enough.

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
    p.flags   = (uint8_t)(((esr & CAN_ESR_BOFF) ? (uint8_t)HeartbeatFlag::BOFF : 0u)
                        | ((esr & CAN_ESR_EPVF) ? (uint8_t)HeartbeatFlag::EPVF : 0u));
    p.uptime  = (uint16_t)(millis() / 1000);
    p.rxCount = rxCount;
    p.esr     = (uint16_t)((((esr >> 16) & 0xFFu) << HB_ESR_TEC_SHIFT)    // ESR[23:16] = TEC
                         | (((esr >> 24) & 0xFFu) << HB_ESR_REC_SHIFT));  // ESR[31:24] = REC
    return p;
}

NodeHealthPayload makeNodeHealthPayload(uint8_t nodeId, NodeKind kind, int8_t dieTempC,
                                        NodeFaultCode fault) {
    NodeHealthPayload p;
    p.nodeId   = nodeId;
    p.dieTempC = dieTempC;
    p.flags    = 0;
#ifdef NODE_OVERHEAT_C
    // Overheat trip is opt-in: the internal sensor is uncalibrated, so a sane threshold
    // needs field data first. Only compute the flag when a build defines NODE_OVERHEAT_C.
    if (dieTempC != DIE_TEMP_UNAVAILABLE && dieTempC >= (int)(NODE_OVERHEAT_C))
        p.flags |= static_cast<uint8_t>(NodeHealthFlag::OVERHEAT);  // bit0 — OR in, never overwrite
#endif
    // Degraded fault (#163): faultId carries the code, DEGRADED bit is derived. faultMask stays 0
    // (reserved for future fault-domain bits — not populated in PR-3).
    p.faultMask = 0;
    p.faultId   = static_cast<uint8_t>(fault);
    if (fault != NodeFaultCode::NONE) p.flags |= static_cast<uint8_t>(NodeHealthFlag::DEGRADED);  // bit1
    p.nodeKind = static_cast<uint8_t>(kind);  // board kind from the node's firmware role (#344)
    p.rsvd[0] = p.rsvd[1] = 0;
    return p;
}

ReadyPayload makeReadyPayload(uint8_t flags) {
    ReadyPayload p = {};
    p.flags = flags;
    p.fwMajor = OPENSKYHAWK_FW_MAJOR;
    p.fwMinor = OPENSKYHAWK_FW_MINOR;
    p.fwPatch = OPENSKYHAWK_FW_PATCH;
    p.bootloader = STM32Board::bootloaderVersion();
    return p;
}

void sendReady() {
    const ReadyPayload p = makeReadyPayload();
    send(canIdReady(NODE_ID), reinterpret_cast<const uint8_t*>(&p), sizeof(p));
    _readySent = true;
}

uint32_t txDropCount() { return _txDrops; }
uint32_t rxDropCount() { return _rxDrops; }

void sendSyncReq(uint8_t nodeId) {
    const uint8_t d[SYNC_REQ_LEN_TARGETED] = {nodeId};
    send(CAN_ID_SYNC_REQ, d, sizeof d);
}

const NodeHealthPayload& lastNodeHealth() { return _lastHealth; }

void serviceNodeStatus(NodeKind kind, uint16_t rxCount) {
    (void)kind;  // only HEALTH_n carries it — unused under -DNODE_HEALTH_TELEM=0
    const uint32_t now = millis();  // after drain(): HB_0 stamps can't be ahead of `now` (#225)

    // Arm on the first call: that is the first loop() after setup() returned, i.e. boot complete
    // (READY_n already sent), whatever a node does between start() and the end of its setup().
    if (!_statusArmed) {
        _statusArmed  = true;
        _lastHbMs     = now;
        _lastHealthMs = now;
    }

    if (now - _lastHbMs >= HB_PERIOD_MS) {
        _lastHbMs = now;
        HeartbeatPayload hb = makeHeartbeatPayload(NODE_ID, rxCount);
        send(canIdHb(NODE_ID), reinterpret_cast<const uint8_t*>(&hb), sizeof(hb));
        if (_loopback) _ownHbLoopPending++;  // will come back — not a duplicate (#344)
    }

    // Duplicate NODE_ID (#344): live until DUP_NODE_TIMEOUT_MS after the last foreign own-ID HB.
    // Signed elapsed, as for the master watchdog (#225); dropping _dupSeen keeps an old stamp from
    // reading as fresh again after millis() wraps.
    if (_dupSeen && (int32_t)(now - _lastDupMs) > (int32_t)DUP_NODE_TIMEOUT_MS) _dupSeen = false;

    // HEALTH: half the heartbeat rate — trend data. NodeFaultCode / aggregateFaults come via
    // NodeStatus.h (included by CANProtocol.h) (#163).
#if !defined(NODE_HEALTH_TELEM) || (NODE_HEALTH_TELEM)
    if (now - _lastHealthMs >= HEALTH_PERIOD_MS) {
        _lastHealthMs = now;
        // A duplicate NODE_ID outranks every FaultSource: with two boards on one ID, nothing else
        // reported under it can be trusted, so it must never be masked by a lesser fault.
        const char* faultDetail = nullptr;   // never null after aggregateFaults()
        NodeFaultCode fault;
        if (_dupSeen) {
            fault       = NodeFaultCode::DUPLICATE_NODE_ID;
            faultDetail = "another board transmits on this NODE_ID";
        } else {
            fault = OpenSkyhawk::aggregateFaults(&faultDetail);
        }
        NodeHealthPayload h = makeNodeHealthPayload(NODE_ID, kind, STM32Board::readDieTempC(), fault);
        _lastHealth = h;
        send(canIdHealth(NODE_ID), reinterpret_cast<const uint8_t*>(&h), sizeof(h));
        STM32Board::logNodeFaultEdge(NODE_ID == GATEWAY_NODE_ID ? "BRIDGE" : "NODE", fault, faultDetail);
#ifdef NODE_OVERHEAT_C
        _overheat = (h.flags & static_cast<uint8_t>(NodeHealthFlag::OVERHEAT)) != 0;
        // The bridge's latch is also PanelBridge's dead-node WARNING, which this service can't see —
        // so on the bridge overheat stays raise-only rather than risk clearing that.
        if (NODE_ID == GATEWAY_NODE_ID && _overheat) STM32Board::setWarning(true);
#endif
    }
#endif

    if (NODE_ID == GATEWAY_NODE_ID) return;  // PanelBridge is the master: no master-loss watchdog

    // Master-loss watchdog. Signed elapsed so a stamp a hair ahead of `now` reads as fresh, never as
    // overdue (#225).
    if (_masterHbPending) {
        _masterHbPending = false;
        _masterLost      = false;
    }
    if (_everSawMaster && (int32_t)(now - _lastMasterMs) > (int32_t)MASTER_TIMEOUT_MS)
        _masterLost = true;

    // Write the latch only when the OR of this service's causes changes (master loss | overheat).
    const bool want = _masterLost || _overheat;
    if (want != _warnAsserted) {
        _warnAsserted = want;
        STM32Board::setWarning(want);
    }
}

} // namespace CANProtocol

#endif // ARDUINO_ARCH_STM32
