// BKA-30 mechanical range measurement v2 (bench) — gentle incremental seat to kill the
// over-drive reversal "jump" that contaminated v1. BARE raw 6-state drive, own PA9/PA10 UART.
//
// Coils PA0/PA1/PA4/PA5 → DRV8833.
//   Phase 1 — seat the RIGHT stop in slow 15-step jumps (low momentum → minimal grind/desync).
//   Phase 2 — creep UP in 15-step jumps + count. Read the count when the needle STOPS at the
//             left stop = stop-to-stop range in steps. degrees ≈ steps/3 (BKA-30 1/3°/step).

#include <Arduino.h>

HardwareSerial dbg(PA10, PA9);               // RX=PA10, TX=PA9 — the board's DiagSerial header

static const uint8_t PINS[4] = { PA0, PA1, PA4, PA5 };
static const uint8_t SEQ[6]  = { 0x9, 0x1, 0x7, 0x6, 0xE, 0x8 };
static uint8_t st = 0;

static void step(int dir) {
    st = (st + (dir > 0 ? 1 : 5)) % 6;
    uint8_t m = SEQ[st];
    for (uint8_t i = 0; i < 4; i++) { digitalWrite(PINS[i], m & 0x1); m >>= 1; }
}

// njump bursts of `per` steps each, pausing between — low-momentum motion that seats a stop
// without the open-loop over-drive grind that desyncs the rotor.
static void creep(int dir, int njump, int per, int pauseMs) {
    for (int j = 0; j < njump; j++) {
        for (int k = 0; k < per; k++) { step(dir); delayMicroseconds(2000); }
        delay(pauseMs);
    }
}

void setup() {
    dbg.begin(115200);
    for (uint8_t i = 0; i < 4; i++) pinMode(PINS[i], OUTPUT);

    dbg.println("=== BKA-30 range cal v3 (gentle seat, guaranteed) ===");
    dbg.println("Phase 1: gentle-seat RIGHT stop (94 x 15 = 1410 steps down)...");
    creep(-1, 94, 15, 90);                   // ~1410 steps > worst-case range → always seats
    delay(800);

    dbg.println("Phase 2: creep UP — read count when needle STOPS at left stop = RANGE:");
    for (int c = 15; c <= 1410; c += 15) {
        for (int k = 0; k < 15; k++) { step(+1); delayMicroseconds(2000); }
        dbg.print("count = "); dbg.println(c);
        delay(300);
    }
    dbg.println("done");
}

void loop() {}
