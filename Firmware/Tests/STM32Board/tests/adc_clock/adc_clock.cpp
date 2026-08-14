// STM32Board — ADC prescaler test (#263)
//
// SystemClock_Config owns ADCPRE on F1 (the Arduino core defers it, and variant_generic.cpp
// never set it), and it has two exit paths — the verified 72 MHz path and the HSI-8 MHz fault
// fallback. Both must set /6, so this file is built into two envs:
//
//   test_adc_clock            — normal boot. PCLK2 72 MHz → ADCCLK 12 MHz (exactly 72e6/6).
//   test_adc_clock_fallback   — -DFORCE_CLOCK_FALLBACK. PCLK2 8 MHz → ADCCLK ~1.33 MHz.
//
// The ADCPRE bits are the assertion that matters in both: they are what the change writes, and
// they are exact. The frequency is asserted exactly only on the 72 MHz path, where the integer
// arithmetic is exact; on the fallback path 8e6/6 truncates to an ugly 1333333, so it is printed
// and range-checked against the F103's rated ADCCLK window instead of pinned to a literal.
//
// Expected output: PASS lines then "=== ALL PASS ===" on DiagSerial (USART1, 115200).

#include <STM32Board.h>
#include <CANProtocol.h>

// F103 datasheet ADCCLK limits.
static constexpr uint32_t ADCCLK_MIN_HZ =   600000UL;
static constexpr uint32_t ADCCLK_MAX_HZ = 14000000UL;

static int _fails = 0;

static void check(const char* name, bool ok) {
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("PASS  ") : F("FAIL  "));
    d.println(name);
    if (!ok) _fails++;
}

static void summary() {
    auto& d = STM32Board::diagSerial();
    if (_fails == 0) d.println(F("=== ALL PASS ==="));
    else { d.print(F("=== ")); d.print(_fails); d.println(F(" FAILED ===")); }
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();  // prints CLOCK OK/FAULT: ... ADC=<n>kHz
    auto& d = STM32Board::diagSerial();
    d.println(F("=== adc_clock ==="));

    uint32_t pclk2  = HAL_RCC_GetPCLK2Freq();
    uint32_t adcclk = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_ADC);

    d.print(F("PCLK2="));  d.print(pclk2);
    d.print(F("Hz ADCCLK=")); d.print(adcclk);
    d.print(F("Hz CFGR[15:14]=")); d.println((RCC->CFGR & RCC_CFGR_ADCPRE) >> RCC_CFGR_ADCPRE_Pos);

    // The change itself: the prescaler bits. True on both exit paths.
    check("ADCPRE == /6              ",
          (RCC->CFGR & RCC_CFGR_ADCPRE) == RCC_CFGR_ADCPRE_DIV6);

    // The point of the change: whatever PCLK2 turned out to be, ADCCLK is in spec.
    check("ADCCLK within F103 limits ",
          adcclk >= ADCCLK_MIN_HZ && adcclk <= ADCCLK_MAX_HZ);

#ifndef FORCE_CLOCK_FALLBACK
    check("PCLK2 == 72 MHz           ", pclk2  == 72000000UL);
    check("ADCCLK == 12 MHz exactly  ", adcclk == 12000000UL);  // 72e6/6, no rounding
    check("clock not faulted         ", !STM32Board::clockFault());
#else
    check("PCLK2 == 8 MHz (fallback) ", pclk2 == 8000000UL);
    check("clock faulted (forced)    ", STM32Board::clockFault());
    // 8e6/6 truncates to 1333333 — deterministic, but not worth pinning to a literal.
#endif

    // The internal channels are the ADC consumer whose sample time the prescaler was predicted
    // to break (239.5 cycles = 6.65 us at 36 MHz vs the sensor's 17.1 us minimum). Measured
    // unchanged at both clocks on Rev 1 — these bounds only catch a gross regression. Print the
    // values for the before/after comparison #263 asks for.
    int8_t   temp = STM32Board::readDieTempC();
    uint16_t vdd  = STM32Board::readVddMv();
    d.print(F("liveTemp=")); d.print(temp); d.print(F("C"));
    d.print(F(" liveVdd=")); d.print(vdd);  d.println(F("mV"));

    check("die temp plausible        ", temp == INT8_MIN || (temp >= -40 && temp <= 125));
    check("Vdd plausible             ", vdd  == 0        || (vdd  >= 2700 && vdd  <= 3600));

    summary();
}

// tick() drives the LED animation. It matters here in a way it does not for the other
// STM32Board tests: the fallback env latches _clockFault, so the board sits in WARNING, and
// WARNING is an ALTERNATING pattern that only advances when tick() flips _blinkPhase. With an
// empty loop() the LED freezes on phase 0 -- red off, green on -- which reads as a healthy
// CONNECTED board while the clock is actually faulted. That is the opposite of what a
// fault-injection binary should signal, so drive the animation.
void loop() {
    STM32Board::tick();
}
