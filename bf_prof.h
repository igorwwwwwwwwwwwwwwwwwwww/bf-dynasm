#ifndef BF_PROF_H
#define BF_PROF_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

// Profiler configuration
#define PROF_SAMPLE_RATE_HZ 1000
#define PROF_MAX_SAMPLES 100000

// Profile sample entry
typedef struct {
    void *pc;           // Program counter (instruction pointer)
    uint64_t timestamp; // Sample timestamp in microseconds
} prof_sample_t;

// Profiler state
typedef struct {
    prof_sample_t *samples;     // Sample buffer
    int sample_count;           // Current number of samples
    int max_samples;            // Maximum samples capacity
    void *code_start;           // Start of JIT code region
    void *code_end;             // End of JIT code region
    bool enabled;               // Profiler enabled flag
    uint64_t start_time;        // Profiling start time
} bf_profiler_t;

// Profiler functions
int bf_prof_init(bf_profiler_t *prof, void *code_start, size_t code_size);
void bf_prof_start(bf_profiler_t *prof);
void bf_prof_stop(bf_profiler_t *prof);
void bf_prof_dump(bf_profiler_t *prof, FILE *out);
void bf_prof_dump_with_debug(bf_profiler_t *prof, FILE *out, void *debug);
void bf_prof_cleanup(bf_profiler_t *prof);

#endif // BF_PROF_H