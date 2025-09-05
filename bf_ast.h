#ifndef BF_AST_H
#define BF_AST_H

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    AST_MOVE_PTR,       // > or < (with count for run-length)
    AST_ADD_VAL,        // + or - (with count for run-length, optional offset)
    AST_OUTPUT,         // .
    AST_INPUT,          // ,
    AST_LOOP,           // [...]

    // Optimized high-level operations
    AST_SET_CONST,      // Direct constant assignment (includes clear cell as SET_CONST(0))
    AST_MUL,            // Multiply current cell by multiplier and add to target offset
} ast_node_type_t;

typedef struct ast_node {
    ast_node_type_t type;
    union {
        struct {
            int count;            // For MOVE_PTR, ADD_VAL value, SET_CONST value
            int offset;           // For ADD_VAL, INPUT, OUTPUT, SET_CONST (default 0 for current position)
        } basic;
        struct {
            struct ast_node *body;  // For AST_LOOP only
        } loop;
        struct {
            int multiplier;         // Multiplier value
            int src_offset;         // Source offset to read from
            int dst_offset;         // Destination offset to add result to
        } mul;
    } data;
    struct ast_node *next;        // Next sibling in sequence
    int line, column;             // Source location for debugging
    int profile_samples;          // Sample count for profiler heat map
} ast_node_t;

// AST construction functions
ast_node_t* ast_create_move(int count);
ast_node_t* ast_create_add(int count, int offset);
ast_node_t* ast_create_output(int offset);
ast_node_t* ast_create_input(int offset);
ast_node_t* ast_create_loop(ast_node_t *body);
ast_node_t* ast_create_sequence(ast_node_t *first, ast_node_t *second);

// Optimized AST nodes
ast_node_t* ast_create_set_const(int value, int offset);
ast_node_t* ast_create_mul(int multiplier, int src_offset, int dst_offset);

// AST manipulation
void ast_free(ast_node_t *node);
void ast_print(ast_node_t *node, int indent);
int ast_count_nodes(ast_node_t *node);
void ast_set_location(ast_node_t *node, int line, int column);
void ast_copy_location(ast_node_t *dst, ast_node_t *src);
ast_node_t* ast_optimize(ast_node_t *node);
ast_node_t* ast_rewrite_sequences(ast_node_t *node);

// AST traversal for code generation is in bf.c to access static DynASM functions

#endif // BF_AST_H
