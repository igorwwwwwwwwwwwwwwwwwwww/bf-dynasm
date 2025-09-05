#include "bf_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define DEBUG_INITIAL_CAPACITY 256

int bf_debug_init(bf_debug_info_t *debug, void *code_start, size_t code_size) {
    memset(debug, 0, sizeof(*debug));

    debug->entries = malloc(DEBUG_INITIAL_CAPACITY * sizeof(debug_map_entry_t));
    if (!debug->entries) {
        return -1;
    }

    debug->entry_count = 0;
    debug->max_entries = DEBUG_INITIAL_CAPACITY;
    debug->code_start = code_start;
    debug->code_size = code_size;

    return 0;
}

// Helper to extract relevant data from AST node for debugging
int bf_debug_get_node_data(ast_node_t *node) {
    if (!node) return 0;

    switch (node->type) {
        case AST_MOVE_PTR:
        case AST_ADD_VAL:
        case AST_SET_CONST:
            return node->data.basic.count;
        case AST_OUTPUT:
        case AST_INPUT:
            return node->data.basic.offset;
        case AST_MUL:
            return node->data.mul.multiplier;
        case AST_LOOP:
        default:
            return 0;
    }
}

void bf_debug_add_mapping(bf_debug_info_t *debug, int pc_label, ast_node_t *node, int source_line, int source_column) {
    if (!debug || !node) return;

    // Expand capacity if needed
    if (debug->entry_count >= debug->max_entries) {
        int new_capacity = debug->max_entries * 2;
        debug_map_entry_t *new_entries = realloc(debug->entries,
                                                 new_capacity * sizeof(debug_map_entry_t));
        if (!new_entries) return;

        debug->entries = new_entries;
        debug->max_entries = new_capacity;
    }

    debug_map_entry_t *entry = &debug->entries[debug->entry_count++];
    entry->pc_label = pc_label;
    entry->pc_offset = 0; // Will be resolved later
    entry->node_type = node->type;
    entry->source_line = source_line;
    entry->source_column = source_column;
    entry->node_data = bf_debug_get_node_data(node);
}

// Find debug entry by PC address
debug_map_entry_t *bf_debug_find_by_pc(bf_debug_info_t *debug, void *pc) {
    if (!debug || !pc) return NULL;

    size_t offset = (char *)pc - (char *)debug->code_start;
    if (offset >= debug->code_size) return NULL;

    // Find closest mapping (linear search for now)
    debug_map_entry_t *best = NULL;
    size_t best_distance = SIZE_MAX;

    for (int i = 0; i < debug->entry_count; i++) {
        debug_map_entry_t *entry = &debug->entries[i];
        if (entry->pc_offset <= offset) {
            size_t distance = offset - entry->pc_offset;
            if (distance < best_distance) {
                best = entry;
                best_distance = distance;
            }
        }
    }

    return best;
}

const char* debug_node_type_name(ast_node_type_t type) {
    switch (type) {
        case AST_MOVE_PTR: return "MOVE_PTR";
        case AST_ADD_VAL: return "ADD_VAL";
        case AST_OUTPUT: return "OUTPUT";
        case AST_INPUT: return "INPUT";
        case AST_LOOP: return "LOOP";
        case AST_SET_CONST: return "SET_CONST";
        case AST_MUL: return "MUL";
        default: return "UNKNOWN";
    }
}

void bf_debug_dump_mappings(bf_debug_info_t *debug, FILE *out) {
    if (!debug || !out) return;

    fprintf(out, "# Debug mappings: %d entries\n", debug->entry_count);
    fprintf(out, "# Format: PC_offset AST_node line:col [data]\n");

    for (int i = 0; i < debug->entry_count; i++) {
        debug_map_entry_t *entry = &debug->entries[i];
        fprintf(out, "0x%zx %s %d:%d", entry->pc_offset,
               debug_node_type_name(entry->node_type), entry->source_line, entry->source_column);

        switch (entry->node_type) {
            case AST_MOVE_PTR:
            case AST_ADD_VAL:
            case AST_SET_CONST:
            case AST_MUL:
                fprintf(out, " [%d]", entry->node_data);
                break;
            default:
                break;
        }
        fprintf(out, "\n");
    }
}

void bf_debug_cleanup(bf_debug_info_t *debug) {
    if (!debug) return;

    free(debug->entries);
    debug->entries = NULL;
    debug->entry_count = 0;
    debug->max_entries = 0;
}
