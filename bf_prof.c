#include "bf_prof.h"
#include "bf_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

// Global profiler instance (needed for signal handler)
static bf_profiler_t *g_profiler = NULL;

// Forward declarations
static void dump_folded_ast_node(ast_node_t *node, FILE *out, const char *stack_prefix);

static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// SIGPROF signal handler - samples the program counter
static void prof_signal_handler(int sig, siginfo_t *info, void *context) {
    (void)sig;
    (void)info;

    if (!g_profiler || !g_profiler->enabled || g_profiler->sample_count >= g_profiler->max_samples) {
        return;
    }

    // Extract program counter from signal context
    void *pc = NULL;
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
    ucontext_t *uc = (ucontext_t *)context;
    pc = (void *)uc->uc_mcontext->__ss.__rip;
#elif defined(__aarch64__) || defined(__arm64__)
    ucontext_t *uc = (ucontext_t *)context;
    pc = (void *)uc->uc_mcontext->__ss.__pc;
#endif

    // Only sample if PC is within our JIT code region
    if (pc >= g_profiler->code_start && pc < g_profiler->code_end) {
        prof_sample_t *sample = &g_profiler->samples[g_profiler->sample_count++];
        sample->pc = pc;
        sample->timestamp = get_time_us() - g_profiler->start_time;
        
        // Directly increment AST node sample count
        if (g_profiler->debug_info && g_profiler->ast_root) {
            bf_debug_info_t *debug = (bf_debug_info_t *)g_profiler->debug_info;
            debug_map_entry_t *entry = bf_debug_find_by_pc(debug, pc);
            if (entry) {
                ast_node_t *node = bf_prof_find_ast_node((ast_node_t *)g_profiler->ast_root, 
                                                         entry->source_line, entry->source_column);
                if (node) {
                    node->profile_samples++;
                }
            }
        }
    }
}

ast_node_t* bf_prof_find_ast_node(ast_node_t *node, int line, int column) {
    if (!node) return NULL;
    
    if (node->line == line && node->column == column) {
        return node;
    }
    
    // Search in loop body
    if (node->type == AST_LOOP && node->data.loop.body) {
        ast_node_t *found = bf_prof_find_ast_node(node->data.loop.body, line, column);
        if (found) return found;
    }
    
    // Search in next sibling
    if (node->next) {
        return bf_prof_find_ast_node(node->next, line, column);
    }
    
    return NULL;
}

int bf_prof_init(bf_profiler_t *prof, void *code_start, size_t code_size, void *debug_info, void *ast_root) {
    memset(prof, 0, sizeof(*prof));

    prof->samples = malloc(PROF_MAX_SAMPLES * sizeof(prof_sample_t));
    if (!prof->samples) {
        return -1;
    }

    prof->sample_count = 0;
    prof->max_samples = PROF_MAX_SAMPLES;
    prof->code_start = code_start;
    prof->code_end = (char *)code_start + code_size;
    prof->enabled = false;
    prof->start_time = 0;
    prof->debug_info = debug_info;
    prof->ast_root = ast_root;

    return 0;
}

void bf_prof_start(bf_profiler_t *prof) {
    if (prof->enabled) return;

    g_profiler = prof;

    struct sigaction sa;
    sa.sa_sigaction = prof_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGPROF, &sa, NULL);

    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000 / PROF_SAMPLE_RATE_HZ;  // Convert Hz to microseconds
    timer.it_value = timer.it_interval;
    setitimer(ITIMER_PROF, &timer, NULL);

    prof->enabled = true;
    prof->sample_count = 0;
    prof->start_time = get_time_us();

    fprintf(stderr, "Profiler started: sampling at %d Hz, code region %p-%p\n",
            PROF_SAMPLE_RATE_HZ, prof->code_start, prof->code_end);
}

void bf_prof_stop(bf_profiler_t *prof) {
    if (!prof->enabled) return;

    struct itimerval timer;
    memset(&timer, 0, sizeof(timer));
    setitimer(ITIMER_PROF, &timer, NULL);

    signal(SIGPROF, SIG_DFL);

    prof->enabled = false;
    g_profiler = NULL;

    fprintf(stderr, "Profiler stopped: collected %d samples\n", prof->sample_count);
}

void bf_prof_dump(bf_profiler_t *prof, FILE *out) {
    fprintf(out, "# Profiler dump: %d samples\n", prof->sample_count);
    fprintf(out, "# Code region: %p-%p (size: %zu bytes)\n",
            prof->code_start, prof->code_end,
            (char *)prof->code_end - (char *)prof->code_start);
    fprintf(out, "# Sample rate: %d Hz\n", PROF_SAMPLE_RATE_HZ);
    fprintf(out, "#\n");
    fprintf(out, "# Format: PC_offset_hex timestamp_us\n");

    for (int i = 0; i < prof->sample_count; i++) {
        prof_sample_t *sample = &prof->samples[i];
        uintptr_t offset = (char *)sample->pc - (char *)prof->code_start;
        fprintf(out, "0x%lx %llu\n", offset, (unsigned long long)sample->timestamp);
    }
}

// Dump profile data with debug info
void bf_prof_dump_with_debug(bf_profiler_t *prof, FILE *out, void *debug_ptr) {
    bf_debug_info_t *debug = (bf_debug_info_t *)debug_ptr;
    fprintf(out, "# Profiler dump: %d samples\n", prof->sample_count);
    fprintf(out, "# Code region: %p-%p (size: %zu bytes)\n",
            prof->code_start, prof->code_end,
            (char *)prof->code_end - (char *)prof->code_start);
    fprintf(out, "# Sample rate: %d Hz\n", PROF_SAMPLE_RATE_HZ);
    fprintf(out, "#\n");
    fprintf(out, "# Format: PC_offset_hex timestamp_us [AST_node_type line:col]\n");

    for (int i = 0; i < prof->sample_count; i++) {
        prof_sample_t *sample = &prof->samples[i];
        uintptr_t offset = (char *)sample->pc - (char *)prof->code_start;

        fprintf(out, "0x%lx %llu", offset, (unsigned long long)sample->timestamp);

        if (debug) {
            debug_map_entry_t *entry = bf_debug_find_by_pc(debug, sample->pc);
            if (entry) {
                fprintf(out, " [%s %d:%d]",
                       debug_node_type_name(entry->node_type), entry->source_line, entry->source_column);
            }
        }
        fprintf(out, "\n");
    }
}

void bf_prof_cleanup(bf_profiler_t *prof) {
    if (prof->enabled) {
        bf_prof_stop(prof);
    }

    free(prof->samples);
    prof->samples = NULL;
    prof->sample_count = 0;
    prof->max_samples = 0;
}

static int find_max_samples(ast_node_t *node) {
    if (!node) return 0;
    
    int max_samples = node->profile_samples;
    
    // Check loop body
    if (node->type == AST_LOOP && node->data.loop.body) {
        int loop_max = find_max_samples(node->data.loop.body);
        if (loop_max > max_samples) max_samples = loop_max;
    }
    
    // Check next sibling
    if (node->next) {
        int next_max = find_max_samples(node->next);
        if (next_max > max_samples) max_samples = next_max;
    }
    
    return max_samples;
}

static void print_heat_indicator(int sample_count, int max_samples, FILE *out) {
    if (max_samples == 0) {
        return;
    }

    double heat_ratio = (double)sample_count / max_samples;

    if (heat_ratio >= 0.8) {
        fprintf(out, " \033[41mHOT\033[0m");  // Red background
    } else if (heat_ratio >= 0.5) {
        fprintf(out, " \033[43mWARM\033[0m"); // Yellow background
    } else if (heat_ratio >= 0.2) {
        fprintf(out, " \033[42mCOOL\033[0m");     // Green background
    } else if (sample_count > 0) {
        fprintf(out, " \033[44mLOW\033[0m");     // Blue background
    }

    if (sample_count > 0) {
        fprintf(out, "(%d)", sample_count);
    }
}

static void print_heat_ast_node(ast_node_t *node, int indent, bf_profiler_t *prof, bf_debug_info_t *debug, int max_samples, FILE *out) {
    if (!node) return;

    for (int i = 0; i < indent; i++) fprintf(out, "  ");

    fprintf(out, "%s", debug_node_type_name(node->type));

    if (node->line > 0 || node->column > 0) {
        fprintf(out, " @%d:%d", node->line, node->column);
        print_heat_indicator(node->profile_samples, max_samples, out);
    }

    switch (node->type) {
        case AST_MOVE_PTR:
            if (node->data.basic.count != 0) fprintf(out, " (count: %d)", node->data.basic.count);
            break;
        case AST_ADD_VAL:
            fprintf(out, " (count: %d, offset: %d)", node->data.basic.count, node->data.basic.offset);
            break;
        case AST_INPUT:
        case AST_OUTPUT:
            if (node->data.basic.offset != 0) fprintf(out, " (offset: %d)", node->data.basic.offset);
            break;
        case AST_SET_CONST:
            fprintf(out, " (value: %d, offset: %d)", node->data.basic.count, node->data.basic.offset);
            break;
        case AST_COPY_CELL:
            fprintf(out, " (src: %d, dst: %d)", node->data.copy.src_offset, node->data.copy.dst_offset);
            break;
        case AST_MUL:
            fprintf(out, " (mul: %d, src: %d, dst: %d)",
                   node->data.mul.multiplier, node->data.mul.src_offset, node->data.mul.dst_offset);
            break;
        case AST_LOOP:
            break;
    }
    fprintf(out, "\n");

    if (node->type == AST_LOOP && node->data.loop.body) {
        print_heat_ast_node(node->data.loop.body, indent + 1, prof, debug, max_samples, out);
    }

    if (node->next) {
        print_heat_ast_node(node->next, indent, prof, debug, max_samples, out);
    }
}

void bf_prof_print_heat_ast(bf_profiler_t *prof, void *debug_ptr, void *ast_ptr, FILE *out) {
    bf_debug_info_t *debug = (bf_debug_info_t *)debug_ptr;
    ast_node_t *ast = (ast_node_t *)ast_ptr;

    if (!prof || !debug || !ast) {
        fprintf(out, "Error: Missing profiler, debug info, or AST data\n");
        return;
    }

    // Find max sample count for any AST node for heat scaling
    int max_samples = find_max_samples(ast);

    fprintf(out, "\n=== HEAT MAP AST (Total samples: %d, Max per location: %d) ===\n",
           prof->sample_count, max_samples);
    fprintf(out, "Legend: \033[41mHOT\033[0m(≥80%%) \033[43mWARM\033[0m(≥50%%) \033[42mCOOL\033[0m(≥20%%) \033[44mLOW\033[0m(<20%%)\n\n");

    print_heat_ast_node(ast, 0, prof, debug, max_samples, out);
    fprintf(out, "\n");
}

void bf_prof_dump_folded(bf_profiler_t *prof, void *debug_ptr, FILE *out) {
    bf_debug_info_t *debug = (bf_debug_info_t *)debug_ptr;
    
    if (!prof || !debug) {
        fprintf(out, "Error: Missing profiler or debug info\n");
        return;
    }
    
    fprintf(out, "# Folded stack format for flame graphs\n");
    fprintf(out, "# Format: @line:col AST_NODE count\n\n");
    
    // Count samples by location using the AST nodes directly
    ast_node_t *ast = (ast_node_t *)prof->ast_root;
    if (ast) {
        dump_folded_ast_node(ast, out, "");
    }
}

static void dump_folded_ast_node(ast_node_t *node, FILE *out, const char *stack_prefix) {
    if (!node) return;
    
    char current_entry[256];
    snprintf(current_entry, sizeof(current_entry), "@%5d:%5d %s", 
             node->line, node->column, debug_node_type_name(node->type));
    
    if (node->type == AST_LOOP) {
        // Build new stack with this loop added
        char new_stack[2048];
        if (strlen(stack_prefix) > 0) {
            snprintf(new_stack, sizeof(new_stack), "%s;%s", stack_prefix, current_entry);
        } else {
            snprintf(new_stack, sizeof(new_stack), "%s", current_entry);
        }
        
        // Recurse into loop body with extended stack
        if (node->data.loop.body) {
            dump_folded_ast_node(node->data.loop.body, out, new_stack);
        }
    } else {
        // For non-loop nodes with samples, output the full stack
        if (node->profile_samples > 0) {
            if (strlen(stack_prefix) > 0) {
                fprintf(out, "%s;%s %d\n", stack_prefix, current_entry, node->profile_samples);
            } else {
                fprintf(out, "%s %d\n", current_entry, node->profile_samples);
            }
        }
    }
    
    // Continue with next sibling (same stack level)
    if (node->next) {
        dump_folded_ast_node(node->next, out, stack_prefix);
    }
}
