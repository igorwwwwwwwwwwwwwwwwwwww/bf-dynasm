#ifndef RP2350_PSRAM_H
#define RP2350_PSRAM_H

#include <stdbool.h>
#include <stdint.h>

#define RP2350_PSRAM_BASE 0x11000000u
#define RP2350_PSRAM_SIZE (8u * 1024u * 1024u)

bool rp2350_psram_init(unsigned int cs_gpio);
bool rp2350_psram_is_ready(void);

#endif
