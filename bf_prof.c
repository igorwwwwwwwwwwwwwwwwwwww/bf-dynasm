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
    }
}

int bf_prof_init(bf_profiler_t *prof, void *code_start, size_t code_size) {
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