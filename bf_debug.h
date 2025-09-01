#ifndef BF_DEBUG_H
#define BF_DEBUG_H

#include "bf_ast.h"
#include <stddef.h>

// Debug mapping entry: PC offset -> AST node
typedef struct {
    int pc_label;               // DynASM PC label index (resolved later)
    size_t pc_offset;           // Actual PC offset (filled after encoding)
    ast_node_type_t node_type;  // AST node type
    int source_line;            // Line in original BF source
    int source_column;          // Column in original BF source
    int node_data;              // Node-specific data (count, offset, etc.)
} debug_map_entry_t;

// Debug info for JIT code
typedef struct {
    debug_map_entry_t *entries; // Debug map entries
    int entry_count;            // Number of entries
    int max_entries;            // Capacity
    void *code_start;           // Start of JIT code
    size_t code_size;           // Size of JIT code
} bf_debug_info_t;

// Debug info functions
int bf_debug_init(bf_debug_info_t *debug, void *code_start, size_t code_size);
void bf_debug_add_mapping(bf_debug_info_t *debug, int pc_label, ast_node_t *node, int source_line, int source_column);
debug_map_entry_t *bf_debug_find_by_pc(bf_debug_info_t *debug, void *pc);
void bf_debug_dump_mappings(bf_debug_info_t *debug, FILE *out);
void bf_debug_cleanup(bf_debug_info_t *debug);

// Helper to get node data for display
int bf_debug_get_node_data(ast_node_t *node);
const char* debug_node_type_name(ast_node_type_t type);

#endif // BF_DEBUG_H