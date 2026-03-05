#ifndef PLATFORM_PICO_H
#define PLATFORM_PICO_H

#include <stdint.h>

void platform_pico_init(void);
void platform_pico_wait_for_usb(void);
uint32_t platform_pico_jit_enter(void);
void platform_pico_jit_leave(uint32_t saved_mie);
void platform_pico_wait_for_start_key_if_enabled(void);
void platform_pico_print_output_begin_if_enabled(void);
void platform_pico_idle_forever(void);

#endif
