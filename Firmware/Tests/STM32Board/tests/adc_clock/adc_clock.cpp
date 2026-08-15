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
//
// STATUS LED -- this sketch drives it from the test result (see the end of setup()):
//
//   test_adc_clock           PASS -> GREEN 1 Hz (NORMAL).  FAIL -> red/green alternating.
//   test_adc_clock_fallback  Always red/green alternating: the env fakes a dead crystal, and
//                            _clockFault outranks everything. Read that env by DiagSerial.
//
// RED 1 s blink means setup() did not reach the end -- it is BOOTING, the state every
// Tests/STM32Board sketch sits in because none of them call CANProtocol::start().
//
// Also note: halting the core under SWD (dump_image, mdw, a paused upload) freezes the GPIOs
// mid-blink. Catch it on an LED-on phase and it sits SOLID, which resembles BUS_OFF (solid red)
// or CONNECTED (solid green) but is just a stopped CPU.

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

    // Put the RESULT on the status LED. Without this the sketch parks in BOOTING (red 1 s)
    // forever -- _canStatus never leaves its initial STARTING because nothing here calls
    // CANProtocol::start() -- so the board looks like a node that failed to bring CAN up, and
    // the LED says nothing about whether the assertions passed.
    //
    // PASS -> NORMAL, green 1 Hz: the "healthy node" pattern, easy to read across a bench.
    // FAIL -> WARNING, red/green alternating.
    //
    // onCanStatus() is being used as a result indicator, not a claim about the bus. That is
    // established practice in this suite (led_state_machine drives it artificially too), and
    // this sketch never starts CAN, so there is no real bus state to misreport.
    //
    // In test_adc_clock_fallback the crystal is faked dead, so _clockFault outranks both
    // (precedence row 2) and the LED shows WARNING regardless -- correct, the clock IS faulted.
    // Read that env by DiagSerial.
    if (_fails == 0) STM32Board::onCanStatus(CanStatus::NORMAL);
    else             STM32Board::setWarning(true);
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
