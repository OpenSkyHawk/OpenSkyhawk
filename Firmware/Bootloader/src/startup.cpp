/**
 * @file startup.cpp
 * @brief The bootloader's own startup (#367): a 16-entry vector table and a Reset_Handler that sets
 *        up .data / .bss and calls main() — nothing else.
 *
 * The stock CMSIS startup reserves all 67 vectors (268 B) and runs SystemInit() and the C runtime's
 * init; the bootloader uses only SysTick, sets its own clocks, and has no static constructors. Every
 * peripheral it uses is polled (CAN, flash, USART2 by DMA), so no IRQ vector is needed past SysTick.
 * The linker symbols come from the CMSIS linker script.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */
#include <stdint.h>

extern "C" {
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
int  main();
void SysTick_Handler();

[[noreturn]] void Reset_Handler() {
    const uint32_t* from = &_sidata;
    for (uint32_t* to = &_sdata; to < &_edata;) *to++ = *from++;
    for (uint32_t* to = &_sbss; to < &_ebss;) *to++ = 0;
    main();
    for (;;) {}
}

[[noreturn]] void Default_Handler() { for (;;) {} }

// Cortex-M3 system vectors only: SP, Reset, NMI, HardFault, MemManage, BusFault, UsageFault,
// 4 reserved, SVCall, DebugMon, reserved, PendSV, SysTick.
__attribute__((section(".isr_vector"), used))
const void* const g_vectors[16] = {
    &_estack, (const void*)Reset_Handler,
    (const void*)Default_Handler, (const void*)Default_Handler, (const void*)Default_Handler,
    (const void*)Default_Handler, (const void*)Default_Handler, nullptr, nullptr, nullptr, nullptr,
    (const void*)Default_Handler, (const void*)Default_Handler, nullptr, (const void*)Default_Handler,
    (const void*)SysTick_Handler,
};
}
