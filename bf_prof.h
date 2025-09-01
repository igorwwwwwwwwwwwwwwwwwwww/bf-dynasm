#ifndef BF_PROF_H
#define BF_PROF_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include "bf_ast.h"

// Profiler configuration
#define PROF_SAMPLE_RATE_HZ 1000

// Profiler state
typedef struct {
    int sample_count;           // Total number of samples collected
    void *code_start;           // Start of JIT code region
    void *code_end;             // End of JIT code region
    bool enabled;               // Profiler enabled flag
    void *debug_info;           // Debug info for PC-to-AST mapping
    void *ast_root;             // AST root for direct sample counting
} bf_profiler_t;

// Profiler functions
int bf_prof_init(bf_profiler_t *prof, void *code_start, size_t code_size, void *debug_info, void *ast_root);
void bf_prof_start(bf_profiler_t *prof);
void bf_prof_stop(bf_profiler_t *prof);
void bf_prof_dump_folded(bf_profiler_t *prof, FILE *out);
void bf_prof_cleanup(bf_profiler_t *prof);

// AST node lookup by location
ast_node_t* bf_prof_find_ast_node(ast_node_t *node, int line, int column);

#endif // BF_PROF_H
