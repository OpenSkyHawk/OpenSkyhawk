// STM32Board — JTAG release test (#299)
//
// The F103 boots with full SWJ, so PA15 (JTDI) and PB4 (NJTRST) belong to the JTAG-DP, which
// forces its own internal pull-up on them whatever the GPIO registers say. STM32Board::begin()
// releases JTAG (SWD kept); from then on the GPIO controller owns the pins.
//
// The probe: configure both pins as input + PULL-DOWN through HAL_GPIO_Init -- NOT pinMode(),
// because the core's pinMode() releases these pins itself and would hide what begin() does --
// then read them before and after begin():
//
//   before begin()  JTAG owns them  -> its pull-up wins -> reads 1   (proves the probe works)
//   after  begin()  GPIO owns them  -> our pull-down    -> reads 0   (proves the release)
//
// PB3 (JTDO) is printed for information only: its level under JTAG is not a reliable tell.
//
// Rig: the STM32 alone -- ST-Link + DiagSerial, nothing wired to PA15 / PB3 / PB4 (anything
// driving them breaks the pull-down reading). No CAN bus needed.
//
// Expected output: PASS lines then "=== ALL PASS ===" on DiagSerial (USART1, 115200).
// STATUS LED: PASS -> GREEN 1 Hz (NORMAL). FAIL -> red/green alternating (WARNING).

#include <STM32Board.h>
#include <CANProtocol.h>

static int _fails = 0;

static void check(const char* name, bool ok) {
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("PASS  ") : F("FAIL  "));
    d.println(name);
    if (!ok) _fails++;
}

static void pullDownInput(GPIO_TypeDef* port, uint16_t pin) {
    GPIO_InitTypeDef g = {};
    g.Pin  = pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(port, &g);
}

static uint8_t rd(GPIO_TypeDef* port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET ? 1 : 0;
}

void setup() {
    // Before begin(): DiagSerial is not up yet, so sample now and print afterwards.
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    pullDownInput(GPIOA, GPIO_PIN_15);
    pullDownInput(GPIOB, GPIO_PIN_3 | GPIO_PIN_4);
    delay(2);
    const uint8_t pa15Before = rd(GPIOA, GPIO_PIN_15);
    const uint8_t pb4Before  = rd(GPIOB, GPIO_PIN_4);
    const uint8_t pb3Before  = rd(GPIOB, GPIO_PIN_3);

    STM32Board::setDebug(true);
    STM32Board::begin();  // the change under test: JTAG released, SWD kept
    delay(2);
    const uint8_t pa15After = rd(GPIOA, GPIO_PIN_15);
    const uint8_t pb4After  = rd(GPIOB, GPIO_PIN_4);
    const uint8_t pb3After  = rd(GPIOB, GPIO_PIN_3);

    auto& d = STM32Board::diagSerial();
    d.println(F("=== jtag_release ==="));
    d.print(F("before begin(): PA15=")); d.print(pa15Before);
    d.print(F(" PB4="));                 d.print(pb4Before);
    d.print(F(" PB3="));                 d.println(pb3Before);
    d.print(F("after  begin(): PA15=")); d.print(pa15After);
    d.print(F(" PB4="));                 d.print(pb4After);
    d.print(F(" PB3="));                 d.println(pb3After);

    check("PA15 JTAG-owned before begin (pull-up wins)", pa15Before == 1);
    check("PB4  JTAG-owned before begin (pull-up wins)", pb4Before  == 1);
    check("PA15 GPIO after begin (pull-down)          ", pa15After  == 0);
    check("PB4  GPIO after begin (pull-down)          ", pb4After   == 0);

    if (_fails == 0) d.println(F("=== ALL PASS ==="));
    else { d.print(F("=== ")); d.print(_fails); d.println(F(" FAILED ===")); }

    // Result on the status LED (same convention as adc_clock): nothing here starts CAN, so
    // onCanStatus() is a result indicator, not a bus claim.
    if (_fails == 0) STM32Board::onCanStatus(CanStatus::NORMAL);
    else             STM32Board::setWarning(true);
}

void loop() {
    STM32Board::tick();
}
