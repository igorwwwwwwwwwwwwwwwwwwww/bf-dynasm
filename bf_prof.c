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

    // Install SIGPROF signal handler
    struct sigaction sa;
    sa.sa_sigaction = prof_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    if (sigaction(SIGPROF, &sa, NULL) == -1) {
        fprintf(stderr, "Failed to install SIGPROF handler\n");
        return;
    }

    // Configure timer to generate SIGPROF at specified rate
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000000 / PROF_SAMPLE_RATE_HZ;  // Interval for first signal
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000 / PROF_SAMPLE_RATE_HZ;  // Interval for subsequent signals

    if (setitimer(ITIMER_PROF, &timer, NULL) == -1) {
        fprintf(stderr, "Failed to start profiling timer\n");
        return;
    }

    prof->enabled = true;
    prof->start_time = get_time_us();
    fprintf(stderr, "Profiler started: sampling at %d Hz, code region %p-%p\n", 
            PROF_SAMPLE_RATE_HZ, prof->code_start, prof->code_end);
}

void bf_prof_stop(bf_profiler_t *prof) {
    if (!prof->enabled) return;

    // Disable timer
    struct itimerval timer = {0};
    setitimer(ITIMER_PROF, &timer, NULL);

    // Restore default SIGPROF handler
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPROF, &sa, NULL);

    prof->enabled = false;
    g_profiler = NULL;

    fprintf(stderr, "Profiler stopped: collected %d samples\n", prof->sample_count);
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