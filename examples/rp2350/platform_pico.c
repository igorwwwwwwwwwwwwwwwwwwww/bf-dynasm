#include "platform_pico.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"

#ifndef RP2350_USB_WAIT_TIMEOUT_MS
#define RP2350_USB_WAIT_TIMEOUT_MS 3000
#endif

#ifndef RP2350_IDLE_HEARTBEAT
#define RP2350_IDLE_HEARTBEAT 0
#endif

#ifndef RP2350_WAIT_FOR_START_KEY
#define RP2350_WAIT_FOR_START_KEY 0
#endif

#ifndef RP2350_PRINT_START_MARKER
#define RP2350_PRINT_START_MARKER 1
#endif

#ifndef RP2350_DEBUG_OUTPUT
#define RP2350_DEBUG_OUTPUT 0
#endif

#if RP2350_DEBUG_OUTPUT
#define PICO_LOG(...) printf(__VA_ARGS__)
#else
#define PICO_LOG(...) ((void)0)
#endif

void platform_pico_init(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
}

void platform_pico_wait_for_usb(void) {
    absolute_time_t usb_deadline = make_timeout_time_ms(RP2350_USB_WAIT_TIMEOUT_MS);
    while (!stdio_usb_connected() && absolute_time_diff_us(get_absolute_time(), usb_deadline) > 0) {
        busy_wait_ms(10);
    }
    if (stdio_usb_connected()) {
        busy_wait_ms(200);
    }
}

#if defined(__riscv)
static inline uint32_t platform_pico_mask_machine_timer_irq(void) {
    uint32_t mie;
    __asm__ volatile ("csrr %0, mie" : "=r"(mie));
    uint32_t new_mie = mie & ~(1u << 7);
    __asm__ volatile ("csrw mie, %0" :: "r"(new_mie) : "memory");
    return mie;
}

uint32_t platform_pico_jit_enter(void) {
    return platform_pico_mask_machine_timer_irq();
}

void platform_pico_jit_leave(uint32_t saved_mie) {
    __asm__ volatile ("csrw mie, %0" :: "r"(saved_mie) : "memory");
}

void __not_in_flash_func(isr_riscv_machine_timer)(void) {
    platform_pico_mask_machine_timer_irq();
}
#else
uint32_t platform_pico_jit_enter(void) { return 0; }
void platform_pico_jit_leave(uint32_t saved_mie) { (void)saved_mie; }
#endif

void platform_pico_wait_for_start_key_if_enabled(void) {
#if RP2350_WAIT_FOR_START_KEY
    printf("[rp2350-jit] press any key to start output\n");
    while (getchar_timeout_us(1000 * 1000) < 0) {
        tight_loop_contents();
    }
#endif
}

void platform_pico_print_output_begin_if_enabled(void) {
#if RP2350_PRINT_START_MARKER
    printf("\n[rp2350-jit] output-begin\n");
#endif
}

void platform_pico_idle_forever(void) {
#if RP2350_IDLE_HEARTBEAT
    absolute_time_t next_heartbeat = make_timeout_time_ms(1000);
#endif
    while (1) {
#if RP2350_IDLE_HEARTBEAT
        if (absolute_time_diff_us(get_absolute_time(), next_heartbeat) <= 0) {
            PICO_LOG("[rp2350-jit] idle\n");
            next_heartbeat = make_timeout_time_ms(1000);
        }
#endif
        tight_loop_contents();
    }
}
