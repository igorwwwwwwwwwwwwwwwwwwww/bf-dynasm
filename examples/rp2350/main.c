#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "rp2350_psram.h"

#ifndef RP2350_USE_PSRAM_DASM
#define RP2350_USE_PSRAM_DASM 1
#endif
#ifndef RP2350_PSRAM_CS_PIN
#define RP2350_PSRAM_CS_PIN 8
#endif

#if RP2350_USE_PSRAM_DASM
static void *rp2350_dasm_realloc(void *ptr, size_t size);
static void rp2350_dasm_free(void *ptr);
#define DASM_M_GROW(ctx, t, p, sz, need) \
    do { \
        size_t _need = (need); \
        if ((sz) < _need) { \
            if ((p) == NULL) { \
                (p) = (t *)rp2350_dasm_realloc(NULL, _need); \
            } else { \
                (p) = (t *)rp2350_dasm_realloc((p), _need); \
            } \
            if ((p) == NULL) exit(1); \
            (sz) = _need; \
        } \
    } while (0)
#define DASM_M_FREE(ctx, p, sz) rp2350_dasm_free((p))
#endif

#ifndef DASM_CHECKS
#define DASM_CHECKS 1
#endif
#include "dasm_proto.h"
#include "dasm_riscv.h"

#include "../../bf_ast.h"
#include "../../bf_parser.h"

#include "bf_program_generated.h"

#ifndef BF_TAPE_SIZE
#define BF_TAPE_SIZE 65536u
#endif
#ifndef BF_TAPE_MEMORY_OFFSET
#define BF_TAPE_MEMORY_OFFSET 4096u
#endif

#define MAX_NESTING 1000

// Keep optimization off by default on RP2350 until all optimized RV32 paths
// are validated on-device.
#ifndef RP2350_JIT_OPTIMIZE
#define RP2350_JIT_OPTIMIZE 1
#endif

#ifndef RP2350_IDLE_HEARTBEAT
#define RP2350_IDLE_HEARTBEAT 0
#endif

#ifndef RP2350_DEBUG_OUTPUT
#define RP2350_DEBUG_OUTPUT 0
#endif

#ifndef RP2350_USB_WAIT_TIMEOUT_MS
#define RP2350_USB_WAIT_TIMEOUT_MS 3000
#endif
#ifndef RP2350_DASM_CHECKSTEP_EACH_NODE
#define RP2350_DASM_CHECKSTEP_EACH_NODE 0
#endif
#ifndef RP2350_PRINT_START_MARKER
#define RP2350_PRINT_START_MARKER 1
#endif
#ifndef RP2350_WAIT_FOR_START_KEY
#define RP2350_WAIT_FOR_START_KEY 0
#endif

#if RP2350_DEBUG_OUTPUT
#define RP_LOG(...) printf(__VA_ARGS__)
#else
#define RP_LOG(...) ((void)0)
#endif

#if RP2350_USE_PSRAM_DASM
typedef struct {
    size_t size;
} rp2350_psram_alloc_hdr_t;

static uint8_t *g_psram_alloc_ptr = (uint8_t *)RP2350_PSRAM_BASE;
static const uint8_t *g_psram_alloc_end = (const uint8_t *)(RP2350_PSRAM_BASE + RP2350_PSRAM_SIZE);
static bool g_psram_allocator_enabled = false;

static inline uintptr_t rp2350_align_up(uintptr_t value, uintptr_t align) {
    return (value + (align - 1u)) & ~(align - 1u);
}

static void *rp2350_psram_alloc(size_t size, size_t align) {
    uintptr_t p = rp2350_align_up((uintptr_t)g_psram_alloc_ptr, align);
    uintptr_t end = p + size;
    if (end > (uintptr_t)g_psram_alloc_end) {
        return NULL;
    }
    g_psram_alloc_ptr = (uint8_t *)end;
    return (void *)p;
}

static void rp2350_psram_allocator_reset(void) {
    if (!g_psram_allocator_enabled) return;
    g_psram_alloc_ptr = (uint8_t *)RP2350_PSRAM_BASE;
}

static void *rp2350_dasm_realloc(void *ptr, size_t size) {
    if (!size) {
        rp2350_dasm_free(ptr);
        return NULL;
    }

    if (!g_psram_allocator_enabled) {
        return realloc(ptr, size);
    }

    size_t total = sizeof(rp2350_psram_alloc_hdr_t) + size;
    rp2350_psram_alloc_hdr_t *new_hdr =
        (rp2350_psram_alloc_hdr_t *)rp2350_psram_alloc(total, 16u);
    if (!new_hdr) return NULL;
    new_hdr->size = size;
    void *new_ptr = (void *)(new_hdr + 1);

    if (ptr) {
        rp2350_psram_alloc_hdr_t *old_hdr = ((rp2350_psram_alloc_hdr_t *)ptr) - 1;
        size_t copy_size = old_hdr->size < size ? old_hdr->size : size;
        memcpy(new_ptr, ptr, copy_size);
    }
    return new_ptr;
}

static void rp2350_dasm_free(void *ptr) {
    if (!g_psram_allocator_enabled) {
        free(ptr);
    }
}
#endif

// DynASM backend hook
bool g_unsafe_mode = false;

typedef int (*bf_func)(char *memory);

// RISC-V DynASM backend implementation (generated from bf_riscv32_rp.dasc)
#include "bf_riscv32_rp.c"

#if RP2350_DEBUG_OUTPUT
static int g_emit_progress_total = 0;
static int g_emit_progress_done = 0;
static absolute_time_t g_emit_progress_start;
static int g_emit_progress_limit = 0;
volatile uint32_t g_dasm_fail_st = 0;
volatile uint32_t g_dasm_fail_base = 0;
volatile uint32_t g_dasm_fail_idx = 0;
volatile int g_dasm_fail_node = 0;
volatile int g_dasm_fail_next_label = 0;
volatile int g_dasm_fail_type = 0;
#endif

#if RP2350_DEBUG_OUTPUT
#ifndef RP2350_COMPILE_TRACE_EVERY
#define RP2350_COMPILE_TRACE_EVERY 10
#endif

static const char *rp_ast_type_name(ast_node_type_t type) {
    switch (type) {
        case AST_MOVE_PTR: return "MOVE_PTR";
        case AST_ADD_VAL: return "ADD_VAL";
        case AST_OUTPUT: return "OUTPUT";
        case AST_INPUT: return "INPUT";
        case AST_LOOP: return "LOOP";
        case AST_SET_CONST: return "SET_CONST";
        case AST_MUL: return "MUL";
        case AST_DEBUG_LOG: return "DEBUG_LOG";
        default: return "UNKNOWN";
    }
}
#endif

static int ast_compile_direct(ast_node_t *node, dasm_State **Dst, int next_label, bool debug_mode) {
    if (!node) return next_label;

#if RP2350_DEBUG_OUTPUT
    g_emit_progress_done++;
    int this_node_index = g_emit_progress_done;
    if (g_emit_progress_done > g_emit_progress_limit) {
        RP_LOG("[rp2350-jit] compile: visit-limit exceeded at node=%d limit=%d (possible AST cycle)\n",
               g_emit_progress_done, g_emit_progress_limit);
        exit(1);
    }
    if ((g_emit_progress_done % RP2350_COMPILE_TRACE_EVERY) == 0) {
        int64_t us = absolute_time_diff_us(g_emit_progress_start, get_absolute_time());
        RP_LOG("[rp2350-jit] compile: node-enter=%d/%d type=%s next_label=%d t=%lld us\n",
               g_emit_progress_done, g_emit_progress_total, rp_ast_type_name(node->type), next_label, (long long)us);
    }
    if ((g_emit_progress_done % 100) == 0) {
        int64_t us = absolute_time_diff_us(g_emit_progress_start, get_absolute_time());
        RP_LOG("[rp2350-jit] compile: progress %d/%d (%.2f%%) t=%lld us\n",
               g_emit_progress_done, g_emit_progress_total,
               g_emit_progress_total ? (100.0 * (double)g_emit_progress_done) / (double)g_emit_progress_total : 0.0,
               (long long)us);
    }
#endif

    switch (node->type) {
        case AST_MOVE_PTR:
            compile_bf_move_ptr(Dst, node->data.basic.count);
            break;
        case AST_ADD_VAL:
            compile_bf_add_val(Dst, node->data.basic.count, node->data.basic.offset);
            break;
        case AST_OUTPUT:
            compile_bf_output(Dst, node->data.basic.offset);
            break;
        case AST_INPUT:
            compile_bf_input(Dst, node->data.basic.offset);
            break;
        case AST_LOOP: {
            int start_label = next_label++;
            int end_label = next_label++;
            int start_fallthrough_label = next_label++;
            int end_fallthrough_label = next_label++;
#if RP2350_DEBUG_OUTPUT
            RP_LOG("[rp2350-jit] compile: loop labels start=%d end=%d sf=%d ef=%d\n",
                   start_label, end_label, start_fallthrough_label, end_fallthrough_label);
#endif
            compile_bf_loop_start(Dst, end_label, start_fallthrough_label);
            compile_bf_label(Dst, start_fallthrough_label);
            compile_bf_label(Dst, start_label);
            next_label = ast_compile_direct(node->data.loop.body, Dst, next_label, debug_mode);
            compile_bf_loop_end(Dst, start_label, end_fallthrough_label);
            compile_bf_label(Dst, end_fallthrough_label);
            compile_bf_label(Dst, end_label);
            break;
        }
        case AST_SET_CONST:
            compile_bf_set_const(Dst, node->data.basic.count, node->data.basic.offset);
            break;
        case AST_MUL:
            compile_bf_mul(Dst, node->data.mul.multiplier, node->data.mul.src_offset, node->data.mul.dst_offset);
            break;
        case AST_DEBUG_LOG:
            compile_bf_debug_log(Dst, debug_mode, node->line, node->column);
            break;
    }

#if RP2350_DEBUG_OUTPUT && RP2350_DASM_CHECKSTEP_EACH_NODE
    {
        int st = dasm_checkstep(Dst, 0);
        if (st != DASM_S_OK) {
            unsigned int base = (unsigned int)st & 0xff000000u;
            unsigned int idx = (unsigned int)st & 0x00ffffffu;
            g_dasm_fail_st = (uint32_t)st;
            g_dasm_fail_base = base;
            g_dasm_fail_idx = idx;
            g_dasm_fail_node = g_emit_progress_done;
            g_dasm_fail_next_label = next_label;
            g_dasm_fail_type = (int)node->type;
            RP_LOG("[rp2350-jit] compile: dasm_checkstep fail st=0x%08x base=0x%08x idx=%u node=%d type=%s next_label=%d\n",
                   (unsigned int)st, base, idx, g_emit_progress_done, rp_ast_type_name(node->type), next_label);
            exit(1);
        }
    }
#endif

    if (node->next) {
        next_label = ast_compile_direct(node->next, Dst, next_label, debug_mode);
    }

#if RP2350_DEBUG_OUTPUT
    if ((this_node_index % RP2350_COMPILE_TRACE_EVERY) == 0) {
        int64_t us = absolute_time_diff_us(g_emit_progress_start, get_absolute_time());
        RP_LOG("[rp2350-jit] compile: node-done=%d/%d type=%s next_label=%d t=%lld us\n",
               this_node_index, g_emit_progress_total, rp_ast_type_name(node->type), next_label, (long long)us);
    }
#endif

    return next_label;
}

static int ast_count_loops(ast_node_t *node) {
    int loops = 0;
    for (ast_node_t *n = node; n; n = n->next) {
        if (n->type == AST_LOOP) {
            loops++;
            loops += ast_count_loops(n->data.loop.body);
        }
    }
    return loops;
}

static bf_func compile_bf_ast(ast_node_t *ast, bool unsafe_mode, size_t *code_size) {
    g_unsafe_mode = unsafe_mode;
    RP_LOG("[rp2350-jit] compile: init dynasm\n");
#if RP2350_USE_PSRAM_DASM
    rp2350_psram_allocator_reset();
#endif

    dasm_State *state = NULL;
    dasm_State **Dst = &state;
    dasm_init(Dst, 1);
    dasm_setup(Dst, actions);
    int loop_count = ast_count_loops(ast);
    unsigned int pc_labels = (unsigned int)(loop_count * 4 + 16);
    RP_LOG("[rp2350-jit] compile: loops=%d pc_labels=%u\n", loop_count, pc_labels);
    dasm_growpc(Dst, pc_labels);

    RP_LOG("[rp2350-jit] compile: emit prologue/body/epilogue\n");
    int effective_tape_size = (int)(BF_TAPE_SIZE - BF_TAPE_MEMORY_OFFSET);
    compile_bf_prologue(Dst, effective_tape_size);
    int total_nodes = ast_count_nodes(ast);
    RP_LOG("[rp2350-jit] compile: emitting %d AST nodes\n", total_nodes);
#if RP2350_DEBUG_OUTPUT
    absolute_time_t emit_start = get_absolute_time();
    g_emit_progress_total = total_nodes;
    g_emit_progress_done = 0;
    g_emit_progress_start = emit_start;
    g_emit_progress_limit = total_nodes * 64 + 1024;
    RP_LOG("[rp2350-jit] compile: visit_limit=%d trace_every=%d\n",
           g_emit_progress_limit, RP2350_COMPILE_TRACE_EVERY);
#endif
    ast_compile_direct(ast, Dst, 0, false);
#if RP2350_DEBUG_OUTPUT
    {
        int64_t emit_us = absolute_time_diff_us(emit_start, get_absolute_time());
        RP_LOG("[rp2350-jit] compile: emit done t=%lld us\n", (long long)emit_us);
    }
#endif
    compile_bf_epilogue(Dst);

    size_t size;
    int ret = dasm_link(Dst, &size);
    if (ret != 0) {
        RP_LOG("[rp2350-jit] dasm_link failed ret=%d\n", ret);
        dasm_free(Dst);
        return NULL;
    }
    RP_LOG("[rp2350-jit] compile: link ok size=%uB\n", (unsigned)size);

    void *code = malloc(size);
    if (!code) {
        RP_LOG("[rp2350-jit] code alloc failed (%u bytes)\n", (unsigned)size);
        dasm_free(Dst);
        return NULL;
    }
    RP_LOG("[rp2350-jit] compile: code buffer=%p\n", code);

    ret = dasm_encode(Dst, code);
    if (ret != 0) {
        RP_LOG("[rp2350-jit] dasm_encode failed ret=%d\n", ret);
        free(code);
        dasm_free(Dst);
        return NULL;
    }
    // Ensure freshly generated instructions are visible to the fetch unit.
    __builtin___clear_cache((char *)code, (char *)code + size);
#if defined(__riscv)
    __asm__ volatile ("fence.i" ::: "memory");
#endif
    RP_LOG("[rp2350-jit] compile: encode ok\n");

    dasm_free(Dst);
    if (code_size) *code_size = size;
    return (bf_func)code;
}

#if defined(__riscv)
static inline uint32_t rp2350_mask_machine_timer_irq(void) {
    // Avoid falling into the weak default machine-timer ISR entry (ebreak)
    // while executing long-running JIT code.
    uint32_t mie;
    __asm__ volatile ("csrr %0, mie" : "=r"(mie));
    uint32_t new_mie = mie & ~(1u << 7); // MTIE
    __asm__ volatile ("csrw mie, %0" :: "r"(new_mie) : "memory");
    return mie;
}

static inline void rp2350_restore_mie(uint32_t mie) {
    __asm__ volatile ("csrw mie, %0" :: "r"(mie) : "memory");
}

void __not_in_flash_func(isr_riscv_machine_timer)(void) {
    // Defensive: if MTIMER ever fires, mask MTIE and continue.
    rp2350_mask_machine_timer_irq();
}
#endif

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    if (BF_TAPE_MEMORY_OFFSET >= BF_TAPE_SIZE) {
        printf("[rp2350-jit] invalid tape layout: offset=%u size=%u\n",
               (unsigned)BF_TAPE_MEMORY_OFFSET, (unsigned)BF_TAPE_SIZE);
        return 1;
    }

#if defined(__riscv)
    // We don't use machine-timer IRQs in this app; keep them masked globally.
    rp2350_mask_machine_timer_irq();
#endif
#if RP2350_USE_PSRAM_DASM
    g_psram_allocator_enabled = rp2350_psram_init(RP2350_PSRAM_CS_PIN);
    RP_LOG("[rp2350-jit] psram: %s (cs=%u)\n",
           g_psram_allocator_enabled ? "enabled" : "init failed, fallback to SRAM heap",
           (unsigned)RP2350_PSRAM_CS_PIN);
#endif

    absolute_time_t usb_deadline = make_timeout_time_ms(RP2350_USB_WAIT_TIMEOUT_MS);
    while (!stdio_usb_connected() && absolute_time_diff_us(get_absolute_time(), usb_deadline) > 0) {
        busy_wait_ms(10);
    }
    if (stdio_usb_connected()) {
        busy_wait_ms(200);
    }

    RP_LOG("[rp2350-jit] usb connected, program_len=%u\n", g_bf_program_len);

    RP_LOG("[rp2350-jit] parse: start\n");
    ast_node_t *ast = parse_bf_program(g_bf_program);
    int ast_nodes_before = ast_count_nodes(ast);
    RP_LOG("[rp2350-jit] parse: done nodes=%d\n", ast_nodes_before);

#if RP2350_JIT_OPTIMIZE
    RP_LOG("[rp2350-jit] optimize: pass1\n");
    ast = ast_rewrite_sequences(ast);
    ast = ast_optimize(ast);
    RP_LOG("[rp2350-jit] optimize: pass2\n");
    ast = ast_rewrite_sequences(ast);
    ast = ast_optimize(ast);
    RP_LOG("[rp2350-jit] optimize: done nodes=%d\n", ast_count_nodes(ast));
#else
    RP_LOG("[rp2350-jit] optimize: skipped (RP2350_JIT_OPTIMIZE=0)\n");
#endif

    RP_LOG("[rp2350-jit] jit: start\n");
#if defined(__riscv)
    uint32_t saved_mie = rp2350_mask_machine_timer_irq();
    RP_LOG("[rp2350-jit] irq: MTIE masked before JIT compile/execute\n");
#endif
    size_t code_size = 0;
    bf_func fn = compile_bf_ast(ast, false, &code_size);
    if (!fn) {
        ast_free(ast);
#if defined(__riscv)
        rp2350_restore_mie(saved_mie);
#endif
        return 1;
    }
    ast_free(ast);
    ast = NULL;
    RP_LOG("[rp2350-jit] jit: ready fn=%p size=%uB\n", (void *)fn, (unsigned)code_size);

#ifdef BF_ENABLE_TIMING
    absolute_time_t start = get_absolute_time();
#endif

    RP_LOG("[rp2350-jit] tape: alloc %uB\n", (unsigned)BF_TAPE_SIZE);
    char *tape = (char *)calloc(BF_TAPE_SIZE, 1);
    if (!tape) {
        RP_LOG("[rp2350-jit] tape alloc failed\n");
        ast_free(ast);
        return 1;
    }
    RP_LOG("[rp2350-jit] exec: enter\n");
#if RP2350_WAIT_FOR_START_KEY
    printf("[rp2350-jit] press any key to start output\n");
    while (getchar_timeout_us(1000 * 1000) < 0) {
        tight_loop_contents();
    }
#endif
#if RP2350_PRINT_START_MARKER
    // Delimit payload so host terminal can sync at frame start.
    printf("\n[rp2350-jit] output-begin\n");
#endif
    int rc = fn(tape + BF_TAPE_MEMORY_OFFSET);
#if defined(__riscv)
    rp2350_restore_mie(saved_mie);
    RP_LOG("[rp2350-jit] irq: mie restored after JIT execute\n");
#endif
    RP_LOG("[rp2350-jit] exec: return rc=%d\n", rc);

#ifdef BF_ENABLE_TIMING
    int64_t elapsed_us = absolute_time_diff_us(start, get_absolute_time());
    printf("\n[rp2350-jit] rc=%d code=%uB elapsed=%lld us (%.3f ms)\n",
           rc, (unsigned)code_size, (long long)elapsed_us, (double)elapsed_us / 1000.0);
#else
    RP_LOG("\n[rp2350-jit] rc=%d code=%uB\n", rc, (unsigned)code_size);
#endif

    free(tape);
    RP_LOG("[rp2350-jit] done: entering idle loop\n");

#if RP2350_IDLE_HEARTBEAT
    absolute_time_t next_heartbeat = make_timeout_time_ms(1000);
#endif
    while (1) {
#if RP2350_IDLE_HEARTBEAT
        if (absolute_time_diff_us(get_absolute_time(), next_heartbeat) <= 0) {
            RP_LOG("[rp2350-jit] idle\n");
            next_heartbeat = make_timeout_time_ms(1000);
        }
#endif
        tight_loop_contents();
    }
}
