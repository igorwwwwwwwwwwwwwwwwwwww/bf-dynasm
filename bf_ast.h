#ifndef BF_AST_H
#define BF_AST_H

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    AST_MOVE_PTR,       // > or < (with count for run-length)
    AST_ADD_VAL,        // + or - (with count for run-length)
    AST_OUTPUT,         // .
    AST_INPUT,          // ,
    AST_LOOP,           // [...]
    AST_SEQUENCE,       // Multiple statements
    
    // Optimized high-level operations
    AST_CLEAR_CELL,     // Optimized [-]
    AST_COPY_CELL,      // Optimized [-<+>] (copy current to left)
    AST_MUL_CONST,      // Optimized ++++[>+++<-]
    AST_SET_CONST,      // Direct constant assignment
} ast_node_type_t;

typedef struct ast_node {
    ast_node_type_t type;
    int value;                    // Count for moves/adds, constant for SET_CONST
    int offset;                   // Memory offset for operations
    struct ast_node *body;        // For loops and sequences
    struct ast_node *next;        // Next sibling in sequence
    int line, column;             // Source location for debugging
} ast_node_t;

// AST construction functions
ast_node_t* ast_create_move(int count);
ast_node_t* ast_create_add(int count); 
ast_node_t* ast_create_output(void);
ast_node_t* ast_create_input(void);
ast_node_t* ast_create_loop(ast_node_t *body);
ast_node_t* ast_create_sequence(ast_node_t *first, ast_node_t *second);

// Optimized AST nodes
ast_node_t* ast_create_clear_cell(void);
ast_node_t* ast_create_copy_cell(int src_offset, int dst_offset);
ast_node_t* ast_create_mul_const(int multiplier, int src_offset, int dst_offset);
ast_node_t* ast_create_set_const(int value);

// AST manipulation
void ast_free(ast_node_t *node);
void ast_print(ast_node_t *node, int indent);
ast_node_t* ast_optimize(ast_node_t *node);

// AST traversal for code generation is in bf.c to access static DynASM functions

#endif // BF_AST_H