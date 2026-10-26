/**
 * @file main.cpp
 * @brief OpenSkyhawk CAN bootloader (#363 core, #364 safety). STM32F103, bare-metal CMSIS, no
 *        Arduino / HAL.
 *
 * Lives in the first 4 KB of flash; every app starts at 0x08001000. At each reset:
 *   1. Option bytes — Data0 / Data1 = the valid app's NODE_ID / NodeKind, and the bootloader's own
 *      4 KB write-protected (WRP0 bit 0). Any difference: rewrite them (all others kept) and reset.
 *   2. Entry flag in backup register DR1 (set by the app, cleared here) → stay.
 *   3. No valid app (identity block magic + hardware CRC over imageLength) → stay.
 *   4. Crash-loop counter (DR2) at ATTEMPT_LIMIT → stay: the app keeps resetting before it reports
 *      healthy.
 *   5. Catch at boot: a CONNECT within CATCH_WINDOW_MS → stay. Otherwise count the attempt and jump.
 * Staying: announce every second on BOOT_RSP_n (n = Data0) — or, with no NODE_ID yet, on
 * UNCONFIGURED_RSP_ID — and take an image over CAN. Entered on request or caught with the app still
 * valid, IDLE_TIMEOUT_MS without a command goes back to the app.
 *
 * Transfer (BootProtocol.h): CONNECT → BEGIN → BLOCK + 10×DATA per 64-byte block → END → GO.
 * BEGIN erases the app's first page, so the old app is invalid from then on. Block 0 (the vector
 * table) is held in RAM and written only after END's CRC check — commit last: an interrupted
 * transfer always leaves "in bootloader, flashable". Pages are erased only as blocks reach them, so
 * flash past the image (the PDU calibration page) is kept.
 *
 * Protocol and boot flow: FirmwarePlan/12-bootloader.md. Layout: FirmwarePlan/08 (identity block).
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */
#include "stm32f1xx.h"
#include <stdint.h>
#include <string.h>
#include <BootProtocol.h>
#include <FirmwareImage.h>

using namespace OpenSkyhawk;
using Boot::Cmd;
using Boot::Reply;
using Boot::Status;
using Boot::BootState;

namespace {

constexpr uint32_t APP_ADDR = FLASH_BASE + Boot::APP_OFFSET;
constexpr uint32_t PAGE     = 1024;                       // F103 medium density (C8 and CB)
constexpr uint32_t BLK      = Boot::BLOCK_SIZE;
constexpr uint32_t RAM_END  = SRAM_BASE + 20 * 1024;      // 20 KB on the C8 and the CB
constexpr uint32_t UNSET    = 0xFF;                       // erased option byte

// Option bytes (stored halfwords: value in the low byte, its complement in the high byte).
volatile uint16_t* const OPT = reinterpret_cast<volatile uint16_t*>(OB_BASE);
enum ObIndex { OB_RDP = 0, OB_USER = 1, OB_DATA0 = 2, OB_DATA1 = 3, OB_WRP0 = 4, OB_COUNT = 8 };

volatile uint32_t g_ms;
uint8_t g_node = UNSET;   // Data0 at boot
uint8_t g_kind = UNSET;   // Data1 at boot
Boot::BootState g_state;  // what the announcements say; APP_INVALID once BEGIN erases the app
uint16_t g_uid16;         // AnnounceField::UID16
bool     g_connected;     // a CONNECT opened the session (on UNCONFIGURED_CMD_ID: with our UID16)
uint32_t g_last_cmd;      // for IDLE_TIMEOUT_MS

// Backup registers DR1..DR10 (16 bits each, 32 bits apart).
volatile uint32_t* bkp(uint8_t n) { return &BKP->DR1 + (n - 1); }

uint32_t u32(uint32_t a) { return *reinterpret_cast<volatile uint32_t*>(a); }
uint32_t le32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24); }
uint32_t flash_end() { return FLASH_BASE + uint32_t(*reinterpret_cast<volatile uint16_t*>(0x1FFFF7E0u)) * 1024u; }
uint8_t  flash_kb()  { return uint8_t((flash_end() - FLASH_BASE) >> 10); }
void wait_ms(uint32_t ms) { uint32_t t = g_ms; while (g_ms - t < ms) {} }

}  // namespace

extern "C" void SysTick_Handler(void) { g_ms++; }

// The info block at Boot::INFO_ADDRESS (#365): placed by `--section-start` and kept by `-u` in
// platformio.ini. A running app reads it to know a bootloader is here and which protocol it speaks.
extern "C" __attribute__((section(".osh_bootinfo"), used))
const Boot::BootloaderInfo osh_bootloader_info = {
    Boot::INFO_MAGIC, Boot::PROTOCOL_VERSION, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

namespace {

// ── Clocks ────────────────────────────────────────────────────────────────────
// HSE 8 MHz × 9 = 72 MHz, APB1 36 MHz — STM32Board's setup, which the CAN bit timing needs. If the
// crystal doesn't start, run HSI / 2 × 9 = 36 MHz with APB1 undivided: still 36 MHz for CAN, so the
// same bit timing holds (within HSI's ±1 %), and the board stays reachable.
uint32_t clock_init() {
    RCC->CR |= RCC_CR_HSEON;
    uint32_t n = 0;
    while (!(RCC->CR & RCC_CR_HSERDY) && ++n < 200000u) {}
    bool hse = RCC->CR & RCC_CR_HSERDY;
    FLASH->ACR = FLASH_ACR_PRFTBE | (hse ? FLASH_ACR_LATENCY_2 : FLASH_ACR_LATENCY_1);
    RCC->CFGR = hse ? (RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9 | RCC_CFGR_PPRE1_DIV2) : RCC_CFGR_PLLMULL9;
    if (!hse) RCC->CR &= ~RCC_CR_HSEON;
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
    return hse ? 72000000u : 36000000u;
}
// Back to the reset state, so the app's clock setup starts from what it expects.
void clock_deinit() {
    RCC->CFGR &= ~RCC_CFGR_SW;
    while ((RCC->CFGR & RCC_CFGR_SWS) != 0) {}
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) {}
    RCC->CR &= ~RCC_CR_HSEON;
    RCC->CFGR = 0;
    FLASH->ACR = FLASH_ACR_PRFTBE;   // zero wait states — HSI 8 MHz
}

// ── Status LED: PB14 red, PB15 green (STM32Board), active high ────────────────
void led_init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB->CRH = (GPIOB->CRH & ~(0xFFu << 24)) | (0x22u << 24);   // PB14, PB15: push-pull 2 MHz
}
void led(bool red, bool green) { GPIOB->BSRR = (red ? 1u << 14 : 1u << 30) | (green ? 1u << 15 : 1u << 31); }

// ── bxCAN, 500 kbit/s on PA11 / PA12 ─────────────────────────────────────────
uint16_t g_cmd_id, g_rsp_id;

void can_init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    // PA12 = CAN_TX: alternate push-pull 50 MHz; PA11 = CAN_RX: input, pull-up
    GPIOA->CRH = (GPIOA->CRH & ~(0xFFu << 12)) | (0xB8u << 12);
    GPIOA->ODR |= 1u << 11;
    CAN1->MCR = CAN_MCR_INRQ;
    while (!(CAN1->MSR & CAN_MSR_INAK)) {}
    CAN1->MCR = CAN_MCR_INRQ | CAN_MCR_ABOM | CAN_MCR_TXFP;     // auto bus-off recovery, FIFO-order TX
    // prescaler 4, BS1 13 tq, BS2 4 tq, SJW 4 tq at 36 MHz = STM32Board's 500 kbit/s
    CAN1->BTR = (3u << 24) | (3u << 20) | (12u << 16) | 3u;
    // filter 0: 32-bit mask, exactly our BOOT_CMD_n standard data frame → FIFO0
    CAN1->FMR |= CAN_FMR_FINIT;
    CAN1->FM1R = 0; CAN1->FS1R = 1; CAN1->FFA1R = 0;
    CAN1->sFilterRegister[0].FR1 = uint32_t(g_cmd_id) << 21;
    CAN1->sFilterRegister[0].FR2 = (0x7FFu << 21) | CAN_RI0R_IDE | CAN_RI0R_RTR;
    CAN1->FA1R = 1;
    CAN1->FMR &= ~CAN_FMR_FINIT;
    CAN1->MCR &= ~CAN_MCR_INRQ;
    uint32_t t = g_ms;
    while ((CAN1->MSR & CAN_MSR_INAK) && g_ms - t < 100) {}      // joins after 11 recessive bits
}
void poll_can();
void can_send(const uint8_t* d, uint8_t len) {
    // Our replies (0x780+n) lose arbitration to a host streaming 0x740+n, so a mailbox can stay busy
    // for a while: keep emptying the 3-deep RX FIFO meanwhile, or the stream overruns it (#366). Any
    // of the three mailboxes will do — TXFP keeps them in request order.
    uint32_t t = g_ms;
    while (!(CAN1->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2))) {
        poll_can();
        if (g_ms - t > 20) return;   // nobody ACKs → drop it
    }
    const uint32_t mb = (CAN1->TSR & CAN_TSR_CODE) >> CAN_TSR_CODE_Pos;
    uint8_t b[8] = {0};
    memcpy(b, d, len);
    CAN1->sTxMailBox[mb].TIR  = uint32_t(g_rsp_id) << 21;
    CAN1->sTxMailBox[mb].TDTR = len;
    CAN1->sTxMailBox[mb].TDLR = le32(b);
    CAN1->sTxMailBox[mb].TDHR = le32(b + 4);
    CAN1->sTxMailBox[mb].TIR |= CAN_TI0R_TXRQ;
}
bool can_recv(uint8_t* d, uint8_t* len) {
    if (!(CAN1->RF0R & CAN_RF0R_FMP0)) return false;
    *len = CAN1->sFIFOMailBox[0].RDTR & 0xF;
    uint32_t lo = CAN1->sFIFOMailBox[0].RDLR, hi = CAN1->sFIFOMailBox[0].RDHR;
    for (int i = 0; i < 4; i++) { d[i] = uint8_t(lo >> (8 * i)); d[4 + i] = uint8_t(hi >> (8 * i)); }
    CAN1->RF0R |= CAN_RF0R_RFOM0;
    return true;
}
// Frames arriving while the bootloader writes flash land here (#366): the CAN FIFO holds only 3, and
// a host streams WINDOW_BLOCKS ahead. Polled — between flash writes and in the main loop.
struct RxFrame { uint8_t n; uint8_t d[8]; };
constexpr uint8_t RING = Boot::WINDOW_BLOCKS * (Boot::DATA_FRAMES + 1) + 4;
RxFrame  g_ring[RING];
uint8_t  g_head, g_tail;
void poll_can() {
    while (uint8_t(g_head + 1) % RING != g_tail) {
        RxFrame& f = g_ring[g_head];
        if (!can_recv(f.d, &f.n)) return;
        g_head = uint8_t(g_head + 1) % RING;
    }
}

void reply(Reply code, uint8_t a = 0, uint8_t b = 0, uint8_t c = 0, uint8_t len = 2) {
    uint8_t d[4] = {uint8_t(code), a, b, c};
    can_send(d, len);
}
void announce(Reply code) {
    uint8_t d[Boot::ANNOUNCE_LEN] = {uint8_t(code), Boot::PROTOCOL_VERSION, g_node, g_kind, uint8_t(g_state), flash_kb(),
                                     uint8_t(g_uid16), uint8_t(g_uid16 >> 8)};
    can_send(d, sizeof d);
}

// ── Flash ─────────────────────────────────────────────────────────────────────
void flash_unlock() {
    if (FLASH->CR & FLASH_CR_LOCK) { FLASH->KEYR = 0x45670123u; FLASH->KEYR = 0xCDEF89ABu; }
}
bool flash_wait() {
    while (FLASH->SR & FLASH_SR_BSY) {}
    bool ok = !(FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR));
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
    return ok;
}
bool flash_erase(uint32_t addr) {
    FLASH->CR |= FLASH_CR_PER; FLASH->AR = addr; FLASH->CR |= FLASH_CR_STRT;
    bool ok = flash_wait();
    FLASH->CR &= ~FLASH_CR_PER;
    return ok;
}
bool flash_program(volatile uint16_t* at, uint16_t v, uint32_t mode) {   // mode: FLASH_CR_PG or FLASH_CR_OPTPG
    FLASH->CR |= mode; *at = v;
    bool ok = flash_wait();
    FLASH->CR &= ~mode;
    // An option byte reads back as complement << 8 | value — compare the value only.
    return ok && (mode == FLASH_CR_OPTPG ? uint8_t(*at) == uint8_t(v) : *at == v);
}
bool flash_write(uint32_t addr, const uint8_t* d, uint32_t n) {
    for (uint32_t i = 0; i < n; i += 2) {
        if (!flash_program(reinterpret_cast<volatile uint16_t*>(addr + i), d[i] | (d[i + 1] << 8), FLASH_CR_PG)) return false;
        poll_can();   // keep the stream flowing while we write
    }
    return true;
}

// ── Option bytes: NODE_ID / NodeKind copy and the bootloader's write protection ──
// Erasing the option bytes also erases RDP, and an erased RDP means read protection — so the window
// between erase and the RDP rewrite is the one place a power cut costs an ST-Link unlock (mass
// erase). It is a few milliseconds, and only on a new board, an ST-Link repurpose or SET_NODE_ID.
// Everything reads the *stored* option bytes, not FLASH_OBR: those take effect only once loaded, and
// a stale OBR must not make the bootloader rewrite them on every boot.
bool option_bytes_set(uint8_t data0, uint8_t data1, uint8_t wrp0) {
    uint8_t v[OB_COUNT];
    for (int i = 0; i < OB_COUNT; i++) v[i] = uint8_t(OPT[i]);
    v[OB_DATA0] = data0;
    v[OB_DATA1] = data1;
    v[OB_WRP0]  = wrp0;
    flash_unlock();
    FLASH->OPTKEYR = 0x45670123u; FLASH->OPTKEYR = 0xCDEF89ABu;
    FLASH->CR |= FLASH_CR_OPTER; FLASH->CR |= FLASH_CR_STRT;
    bool ok = flash_wait();
    FLASH->CR &= ~FLASH_CR_OPTER;
    for (int i = 0; i < OB_COUNT; i++)          // RDP first: it is the byte an erase leaves dangerous
        if (v[i] != UNSET && !flash_program(&OPT[i], v[i], FLASH_CR_OPTPG)) ok = false;
    FLASH->CR &= ~FLASH_CR_OPTWRE;
    for (int i = 0; i < OB_COUNT; i++)
        if (uint8_t(OPT[i]) != v[i]) ok = false;
    return ok;
}
uint8_t wrp0() { return uint8_t(OPT[OB_WRP0]); }
constexpr uint8_t WRP_BOOTLOADER = 0x01;        // WRP0 bit 0 = pages 0-3: protected when 0

// ── Image checks ──────────────────────────────────────────────────────────────
// Hardware CRC-32/MPEG-2 over 32-bit little-endian words — the identity block's imageCrc.
void crc_reset() { RCC->AHBENR |= RCC_AHBENR_CRCEN; CRC->CR = CRC_CR_RESET; }
void crc_feed(const uint8_t* d, uint32_t n, uint32_t base, uint32_t skip) {   // skip: the imageCrc word's offset
    for (uint32_t i = 0; i < n; i += 4)
        if (base + i != skip) CRC->DR = le32(d + i);
}

const FirmwareImage::Identity* identity() {
    return reinterpret_cast<const FirmwareImage::Identity*>(APP_ADDR + FirmwareImage::IDENTITY_OFFSET);
}

// Identity block plausible: magic, layout, a length that fits, a stack and an entry point that
// make sense. The CRC is checked by the caller.
bool identity_ok(uint32_t vec_sp, uint32_t vec_pc) {
    const FirmwareImage::Identity* id = identity();
    return id->magic == FirmwareImage::MAGIC && id->layoutVersion >= 1
        && id->imageLength > FirmwareImage::IDENTITY_OFFSET + sizeof(FirmwareImage::Identity)
        && (id->imageLength & 3) == 0 && APP_ADDR + id->imageLength <= flash_end()
        && vec_sp > SRAM_BASE && vec_sp <= RAM_END
        && vec_pc > APP_ADDR && vec_pc < APP_ADDR + id->imageLength;
}

constexpr uint32_t CRC_WORD = FirmwareImage::IDENTITY_OFFSET + FirmwareImage::OFF_CRC;

uint16_t uid16() {
    crc_reset();
    for (uint32_t i = 0; i < 12; i += 4) CRC->DR = u32(Boot::UID_ADDRESS + i);
    return uint16_t(CRC->DR);
}

bool app_valid() {
    if (!identity_ok(u32(APP_ADDR), u32(APP_ADDR + 4))) return false;
    crc_reset();
    crc_feed(reinterpret_cast<const uint8_t*>(APP_ADDR), identity()->imageLength, 0, CRC_WORD);
    return CRC->DR == identity()->imageCrc;
}

// ── Jump ──────────────────────────────────────────────────────────────────────
[[noreturn]] void jump_to_app() {
    uint32_t sp = u32(APP_ADDR), pc = u32(APP_ADDR + 4);
    led(false, false);
    SysTick->CTRL = 0;
    RCC->APB1RSTR |= RCC_APB1RSTR_CAN1RST; RCC->APB1RSTR &= ~RCC_APB1RSTR_CAN1RST;
    RCC->APB1ENR &= ~(RCC_APB1ENR_CAN1EN | RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
    RCC->AHBENR &= ~RCC_AHBENR_CRCEN;
    clock_deinit();
    for (int i = 0; i < 8; i++) { NVIC->ICER[i] = 0xFFFFFFFFu; NVIC->ICPR[i] = 0xFFFFFFFFu; }
    SCB->VTOR = APP_ADDR;
    // Switch to the app's stack and branch in ONE asm block: once MSP is the app's (top of RAM),
    // nothing may touch our stack frame again — a compiler `pop` there bus-faulted on the bench (#361).
    __asm volatile ("msr msp, %0\n\tbx %1\n" : : "r"(sp), "r"(pc) : "memory");
    for (;;) {}
}

// ── Session ───────────────────────────────────────────────────────────────────
uint16_t crc16(const uint8_t* d, uint32_t n) {   // CRC-16/CCITT-FALSE
    uint16_t c = 0xFFFF;
    while (n--) { c ^= uint16_t(*d++) << 8; for (int k = 0; k < 8; k++) c = (c & 0x8000) ? (c << 1) ^ 0x1021 : c << 1; }
    return c;
}

struct Session {
    uint8_t  buf[BLK];
    uint8_t  blk0[BLK];      // the vector table, written last
    uint32_t total = 0;      // BEGIN length, padded to whole blocks
    uint32_t next  = 0;      // next block expected
    uint16_t index = 0, want = 0, have = 0;
    bool     active = false;
};
Session s;
uint32_t g_last_announce;

// LEDs, and the announcement while waiting — called from the main loop and between BEGIN's page erases.
void service() {
    // Red + green together = "in the bootloader" — the app never lights both (#365):
    // waiting: both blink 1 Hz · image arriving: both flicker ~8 Hz · app unstable: both, then
    // red alone · unconfigured: both, then green alone (500 ms each).
    bool half = (g_ms >> 9) & 1;
    if (s.active)                                led((g_ms >> 6) & 1, (g_ms >> 6) & 1);
    else if (g_state == BootState::APP_UNSTABLE) led(true, half);
    else if (g_state == BootState::UNCONFIGURED) led(half, true);
    else                                         led(half, half);
    // Announce while waiting — and during a transfer that has gone quiet, so the bridge still sees
    // the node alive in its bootloader (#365).
    if (g_ms - g_last_announce >= Boot::ANNOUNCE_INTERVAL_MS && (!s.active || g_ms - g_last_cmd >= Boot::ANNOUNCE_INTERVAL_MS)) {
        g_last_announce = g_ms;
        announce(Reply::ANNOUNCE);
    }
}

Status begin(uint32_t len) {
    s.active = false;
    s.total = (len + BLK - 1) / BLK * BLK;
    s.next = 0;
    if (!len || APP_ADDR + s.total > flash_end()) return Status::OUT_OF_RANGE;
    // Every page the image needs, now (#366) — page 0 first, so the old app is invalid from here on —
    // so a streaming host never waits on a 20 ms erase. Pages past the image are never touched.
    for (uint32_t a = APP_ADDR; a < APP_ADDR + s.total; a += PAGE) {
        if (!flash_erase(a)) return Status::FLASH_ERROR;
        service();
    }
    s.active = true;
    return Status::OK;
}

Status block() {
    uint32_t addr = APP_ADDR + uint32_t(s.index) * BLK;
    if (!s.active) return Status::OUT_OF_ORDER;
    if (crc16(s.buf, BLK) != s.want) return Status::BAD_CRC;
    if (s.index < s.next) return Status::OK;   // a resend (go-back-N after a lost ack) — already written
    if (s.index != s.next || addr + BLK > APP_ADDR + s.total) return Status::OUT_OF_ORDER;
    if (s.index == 0) memcpy(s.blk0, s.buf, BLK);
    else if (!flash_write(addr, s.buf, BLK)) return Status::FLASH_ERROR;
    s.next++;
    return Status::OK;
}

Status end(uint32_t want, uint32_t* got) {
    if (!s.active || s.next * BLK < s.total) return Status::OUT_OF_ORDER;
    s.active = false;
    const FirmwareImage::Identity* id = identity();
    if (id->magic != FirmwareImage::MAGIC || id->imageLength > s.total || id->imageCrc != want
        || !identity_ok(le32(s.blk0), le32(s.blk0 + 4))) return Status::BAD_IMAGE;
    crc_reset();                                  // block 0 from RAM, the rest from flash
    crc_feed(s.blk0, BLK, 0, CRC_WORD);
    crc_feed(reinterpret_cast<const uint8_t*>(APP_ADDR + BLK), id->imageLength - BLK, BLK, CRC_WORD);
    *got = CRC->DR;
    if (*got != want) return Status::BAD_CRC;
    // NODE_ID rule (#364): an upload never changes a board's identity — SET_NODE_ID does that,
    // separately. A board with no NODE_ID yet takes its first image's.
    if (g_node != UNSET && id->nodeId != g_node) return Status::WRONG_NODE;
    if (g_node != UNSET && id->nodeKind != g_kind) return Status::WRONG_KIND;
    if (!flash_write(APP_ADDR, s.blk0, BLK)) return Status::FLASH_ERROR;   // commit: valid only now
    *bkp(Boot::ATTEMPTS_BKP_REGISTER) = 0;        // a new image starts with a clean crash-loop count
    return Status::OK;
}

// SET_NODE_ID: invalidate the app first — a valid app would put its own ID back at the next boot —
// then Data0 in one option-byte write, and reset so the board answers on its new BOOT_RSP_n.
Status set_node_id(uint8_t id) {
    if (!g_connected || g_node == UNSET) return Status::OUT_OF_ORDER;
    if (id == 0 || id > 63 || g_kind == uint8_t(::NodeKind::BRIDGE)) return Status::OUT_OF_RANGE;
    if (!flash_erase(APP_ADDR)) return Status::FLASH_ERROR;
    if (!option_bytes_set(id, g_kind, wrp0())) return Status::FLASH_ERROR;
    return Status::OK;
}

void handle(const uint8_t* m, uint8_t n) {
    if (Cmd(m[0]) == Cmd::CONNECT) {
        // Unconfigured boards share one command ID: only the one whose UID16 is named binds.
        if (g_node == UNSET && (n < 3 || (m[1] | (m[2] << 8)) != g_uid16)) return;
        g_connected = true;
    }
    if (g_node == UNSET && !g_connected) return;
    g_last_cmd = g_ms;
    switch (Cmd(m[0])) {
    case Cmd::CONNECT:
        announce(Reply::CONNECTED);
        break;
    case Cmd::BEGIN:
        if (n >= 5) {
            Status st = begin(le32(m + 1));
            if (st == Status::OK) g_state = BootState::APP_INVALID;   // the old app's first page is gone
            reply(Reply::BEGUN, uint8_t(st));
        }
        break;
    case Cmd::BLOCK:
        if (n >= 5) { s.index = m[1] | (m[2] << 8); s.want = m[3] | (m[4] << 8); s.have = 0; memset(s.buf, 0xFF, BLK); }
        break;
    case Cmd::END:
        if (n >= 5) {
            uint32_t got = 0;
            Status st = end(le32(m + 1), &got);
            uint8_t d[6] = {uint8_t(Reply::ENDED), uint8_t(st), uint8_t(got), uint8_t(got >> 8), uint8_t(got >> 16), uint8_t(got >> 24)};
            can_send(d, sizeof d);
        }
        break;
    case Cmd::GO:
        if (!app_valid()) { reply(Reply::GOING, uint8_t(Status::NO_APP)); break; }
        reply(Reply::GOING, uint8_t(Status::OK));
        *bkp(Boot::ATTEMPTS_BKP_REGISTER) = 0;    // the host asked to start it: an unstable app gets new tries
        wait_ms(5);
        NVIC_SystemReset();                       // boot again: the app passes the check and starts
    case Cmd::SET_NODE_ID: {
        Status st = n >= 2 ? set_node_id(m[1]) : Status::OUT_OF_RANGE;
        reply(Reply::NODE_SET, uint8_t(st));
        if (st == Status::OK) { wait_ms(5); NVIC_SystemReset(); }
        break;
    }
    default:
        if (m[0] >= uint8_t(Cmd::DATA_0) && m[0] <= uint8_t(Cmd::DATA_9)) {
            uint8_t k = m[0] - uint8_t(Cmd::DATA_0);
            uint32_t at = k * Boot::DATA_BYTES, cnt = n - 1u < BLK - at ? n - 1u : BLK - at;
            memcpy(s.buf + at, m + 1, cnt);
            s.have |= 1u << k;
            if (s.have == (1u << Boot::DATA_FRAMES) - 1) {
                s.have = 0;
                uint8_t st = uint8_t(block());
                uint8_t d[6] = {uint8_t(Reply::BLOCK_ACK), uint8_t(s.index), uint8_t(s.index >> 8), st,
                                uint8_t(s.next), uint8_t(s.next >> 8)};
                can_send(d, sizeof d);
            }
        }
    }
}

}  // namespace

int main() {
    // Entry flag and crash-loop count — read before anything else; the flag is one-shot.
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR->CR |= PWR_CR_DBP;
    bool requested = *bkp(Boot::ENTRY_BKP_REGISTER) == Boot::ENTRY_MAGIC;
    *bkp(Boot::ENTRY_BKP_REGISTER) = 0;
    uint32_t attempts = *bkp(Boot::ATTEMPTS_BKP_REGISTER);

    SysTick_Config(clock_init() / 1000);
    led_init();

    g_node = uint8_t(OPT[OB_DATA0]);
    g_kind = uint8_t(OPT[OB_DATA1]);
    bool valid = app_valid();
    g_uid16 = uid16();

    // Data0 / Data1 = the valid app's NODE_ID / kind, so a board whose app breaks later still answers
    // as its node; and our own 4 KB write-protected. Rewritten only when something differs (a new
    // board, an ST-Link repurpose); a failed write leaves them as they were and boot carries on.
    uint8_t d0 = valid ? identity()->nodeId : g_node, d1 = valid ? identity()->nodeKind : g_kind;
    if ((d0 != g_node || d1 != g_kind || (wrp0() & WRP_BOOTLOADER)) && option_bytes_set(d0, d1, wrp0() & ~WRP_BOOTLOADER))
        NVIC_SystemReset();

    g_cmd_id = g_node == UNSET ? Boot::UNCONFIGURED_CMD_ID : Boot::canIdBootCmd(g_node);
    g_rsp_id = g_node == UNSET ? Boot::UNCONFIGURED_RSP_ID : Boot::canIdBootRsp(g_node);
    g_state = requested ? BootState::REQUESTED : !valid ? (g_node == UNSET ? BootState::UNCONFIGURED : BootState::APP_INVALID)
            : attempts >= Boot::ATTEMPT_LIMIT ? BootState::APP_UNSTABLE : BootState::CAUGHT;
    can_init();

    if (valid && !requested && g_state == BootState::CAUGHT) {
        // Catch at boot: a CONNECT now keeps us here — rescues an app that never takes the request.
        bool caught = false;
        uint8_t m[8], n;
        for (uint32_t t0 = g_ms; !caught && g_ms - t0 < Boot::CATCH_WINDOW_MS; )
            caught = can_recv(m, &n) && n && m[0] == uint8_t(Cmd::CONNECT);
        if (!caught) {
            *bkp(Boot::ATTEMPTS_BKP_REGISTER) = attempts + 1;   // the app sets it back to 0 once healthy
            jump_to_app();
        }
        g_connected = true;
        announce(Reply::CONNECTED);
    }
    flash_unlock();

    g_last_announce = g_ms - Boot::ANNOUNCE_INTERVAL_MS;
    g_last_cmd = g_ms;
    for (;;) {
        service();
        // Entered on request or caught, the old app is still valid until BEGIN: nobody came → back to it.
        if ((g_state == BootState::REQUESTED || g_state == BootState::CAUGHT) && g_ms - g_last_cmd >= Boot::IDLE_TIMEOUT_MS)
            NVIC_SystemReset();
        poll_can();
        if (g_tail != g_head) {
            RxFrame& f = g_ring[g_tail];
            if (f.n) handle(f.d, f.n);
            g_tail = uint8_t(g_tail + 1) % RING;
        }
    }
}
