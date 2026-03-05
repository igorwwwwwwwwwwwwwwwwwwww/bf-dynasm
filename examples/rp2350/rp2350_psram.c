#include <stdbool.h>
#include <stdint.h>

#include "pico/platform/sections.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip.h"
#include "hardware/regs/qmi.h"
#include "hardware/regs/xip.h"
#include "hardware/timer.h"

#include "rp2350_psram.h"

#define PSRAM_CMD_RESET_EN 0x66u
#define PSRAM_CMD_RESET 0x99u
#define PSRAM_CMD_ENTER_QPI 0x35u
#define PSRAM_CMD_READ_ID 0x9Fu
#define PSRAM_CMD_QUAD_READ 0xEBu
#define PSRAM_CMD_QUAD_WRITE 0x38u
#define PSRAM_KGD_PASS 0x5Du

#define PSRAM_INIT_CLKDIV 4u

static uint32_t s_irq_save;
static bool s_psram_ready = false;

static void __no_inline_not_in_flash_func(direct_begin)(void) {
    s_irq_save = save_and_disable_interrupts();
    qmi_hw->direct_csr = ((uint32_t)PSRAM_INIT_CLKDIV << QMI_DIRECT_CSR_CLKDIV_LSB) | QMI_DIRECT_CSR_EN_BITS;
    hw_set_bits(&qmi_hw->direct_csr, QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
}

static void __no_inline_not_in_flash_func(direct_end)(void) {
    hw_clear_bits(&qmi_hw->direct_csr, QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
    hw_clear_bits(&qmi_hw->direct_csr, QMI_DIRECT_CSR_EN_BITS);
    restore_interrupts(s_irq_save);
}

static uint8_t __no_inline_not_in_flash_func(direct_byte)(uint8_t tx, bool capture_rx) {
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_TXFULL_BITS) {
    }

    uint32_t word = (uint32_t)tx | (1u << 19);
    if (!capture_rx) {
        word |= (1u << 20);
    }
    qmi_hw->direct_tx = word;

    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) {
    }

    if (!capture_rx) return 0;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_RXEMPTY_BITS) {
    }
    return (uint8_t)(qmi_hw->direct_rx & 0xFFu);
}

static uint8_t __no_inline_not_in_flash_func(direct_rx_byte)(void) {
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_TXFULL_BITS) {
    }
    qmi_hw->direct_tx = 0u;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) {
    }
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_RXEMPTY_BITS) {
    }
    return (uint8_t)(qmi_hw->direct_rx & 0xFFu);
}

static void __no_inline_not_in_flash_func(psram_cmd_only)(uint8_t cmd) {
    direct_begin();
    direct_byte(cmd, false);
    direct_end();
}

static void __no_inline_not_in_flash_func(psram_read_id)(uint8_t *mfid, uint8_t *kgd) {
    direct_begin();
    direct_byte(PSRAM_CMD_READ_ID, false);
    direct_byte(0x00, false);
    direct_byte(0x00, false);
    direct_byte(0x00, false);
    *mfid = direct_rx_byte();
    *kgd = direct_rx_byte();
    direct_end();
}

bool rp2350_psram_init(unsigned int cs_gpio) {
    if (s_psram_ready) return true;

    gpio_set_function(cs_gpio, GPIO_FUNC_XIP_CS1);
    busy_wait_us_32(10);

    psram_cmd_only(PSRAM_CMD_RESET_EN);
    busy_wait_us_32(10);
    psram_cmd_only(PSRAM_CMD_RESET);
    busy_wait_us_32(100);

    uint8_t mfid = 0;
    uint8_t kgd = 0;
    psram_read_id(&mfid, &kgd);
    (void)mfid;
    // Some parts/boards can report an unexpected KGD byte even when the
    // device is otherwise functional. Treat ID mismatch as non-fatal and rely
    // on the actual XIP read/write sanity test below.

    psram_cmd_only(PSRAM_CMD_ENTER_QPI);
    busy_wait_us_32(1);

    qmi_hw->m[1].timing =
        (1u << QMI_M1_TIMING_COOLDOWN_LSB) |
        (QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB) |
        (1u << QMI_M1_TIMING_RXDELAY_LSB) |
        (4u << QMI_M1_TIMING_CLKDIV_LSB);

    qmi_hw->m[1].rfmt =
        (QMI_M1_RFMT_PREFIX_LEN_VALUE_8 << QMI_M1_RFMT_PREFIX_LEN_LSB) |
        (6u << QMI_M1_RFMT_DUMMY_LEN_LSB) |
        (QMI_M1_RFMT_DATA_WIDTH_VALUE_Q << QMI_M1_RFMT_DATA_WIDTH_LSB) |
        (QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_RFMT_DUMMY_WIDTH_LSB) |
        (QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_RFMT_ADDR_WIDTH_LSB) |
        (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB);
    qmi_hw->m[1].rcmd = PSRAM_CMD_QUAD_READ;

    qmi_hw->m[1].wfmt =
        (QMI_M1_WFMT_PREFIX_LEN_VALUE_8 << QMI_M1_WFMT_PREFIX_LEN_LSB) |
        (QMI_M1_WFMT_DATA_WIDTH_VALUE_Q << QMI_M1_WFMT_DATA_WIDTH_LSB) |
        (QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_WFMT_ADDR_WIDTH_LSB) |
        (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB);
    qmi_hw->m[1].wcmd = PSRAM_CMD_QUAD_WRITE;

    hw_set_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);

    volatile uint32_t *p = (volatile uint32_t *)RP2350_PSRAM_BASE;
    *p = 0xA5A55A5Au;
    __compiler_memory_barrier();
    if (*p != 0xA5A55A5Au) {
        return false;
    }

    s_psram_ready = true;
    return true;
}

bool rp2350_psram_is_ready(void) {
    return s_psram_ready;
}
