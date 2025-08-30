#include "bf_ast.h"
#include <string.h>

// AST node creation functions
ast_node_t* ast_create_node(ast_node_type_t type) {
    ast_node_t *node = calloc(1, sizeof(ast_node_t));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    node->type = type;
    return node;
}

ast_node_t* ast_create_move(int count) {
    ast_node_t *node = ast_create_node(AST_MOVE_PTR);
    node->value = count;
    return node;
}

ast_node_t* ast_create_add(int count) {
    ast_node_t *node = ast_create_node(AST_ADD_VAL);
    node->value = count;
    return node;
}

ast_node_t* ast_create_output(void) {
    return ast_create_node(AST_OUTPUT);
}

ast_node_t* ast_create_input(void) {
    return ast_create_node(AST_INPUT);
}

ast_node_t* ast_create_loop(ast_node_t *body) {
    ast_node_t *node = ast_create_node(AST_LOOP);
    node->body = body;
    return node;
}

ast_node_t* ast_create_sequence(ast_node_t *first, ast_node_t *second) {
    if (!first) return second;
    if (!second) return first;
    
    // Find the last node in the first chain and append second to it
    ast_node_t *current = first;
    while (current->next) {
        current = current->next;
    }
    current->next = second;
    return first;
}

// Optimized AST node creation
ast_node_t* ast_create_clear_cell(void) {
    return ast_create_node(AST_CLEAR_CELL);
}


ast_node_t* ast_create_mul_const(int multiplier, int src_offset, int dst_offset) {
    ast_node_t *node = ast_create_node(AST_MUL_CONST);
    node->value = multiplier;
    node->offset = src_offset;
    // Store dst_offset in a different field - we'd need to extend the struct
    return node;
}

ast_node_t* ast_create_set_const(int value) {
    ast_node_t *node = ast_create_node(AST_SET_CONST);
    node->value = value;
    return node;
}

// Memory management
void ast_free(ast_node_t *node) {
    if (!node) return;
    
    ast_free(node->body);
    ast_free(node->next);
    free(node);
}

// Pretty printing for debugging
static const char* ast_type_name(ast_node_type_t type) {
    switch (type) {
        case AST_MOVE_PTR: return "MOVE_PTR";
        case AST_ADD_VAL: return "ADD_VAL";
        case AST_OUTPUT: return "OUTPUT";
        case AST_INPUT: return "INPUT";
        case AST_LOOP: return "LOOP";
        case AST_SEQUENCE: return "SEQUENCE";
        case AST_CLEAR_CELL: return "CLEAR_CELL";
        case AST_MUL_CONST: return "MUL_CONST";
        case AST_SET_CONST: return "SET_CONST";
        default: return "UNKNOWN";
    }
}

void ast_print(ast_node_t *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    printf("%s", ast_type_name(node->type));
    if (node->value != 0) printf(" (value: %d)", node->value);
    if (node->offset != 0) printf(" (offset: %d)", node->offset);
    printf("\n");
    
    // For loops, print the body with increased indentation
    if (node->type == AST_LOOP && node->body) {
        ast_print(node->body, indent + 1);
    }
    
    // For all nodes, continue with the next node at same level
    if (node->next) {
        ast_print(node->next, indent);
    }
}

// AST traversal for code generation is now in bf.c to access static DynASM functions

// AST optimization - combine consecutive operations
ast_node_t* ast_optimize(ast_node_t *node) {
    if (!node) return NULL;
    
    // First optimize children recursively
    if (node->body) {
        node->body = ast_optimize(node->body);
    }
    if (node->next) {
        node->next = ast_optimize(node->next);
    }
    
    // Run-length encoding: combine consecutive ADD_VAL or MOVE_PTR operations
    if (node->next && node->type == node->next->type) {
        if (node->type == AST_ADD_VAL || node->type == AST_MOVE_PTR) {
            // Combine with next node
            node->value += node->next->value;
            ast_node_t *old_next = node->next;
            node->next = node->next->next;
            free(old_next);
            
            // Continue optimizing from current node (might combine further)
            return ast_optimize(node);
        }
    }
    
    // Clear loop optimization: detect [-] pattern
    if (node->type == AST_LOOP && node->body && 
        node->body->type == AST_ADD_VAL && node->body->value == -1 &&
        !node->body->next) {
        // Replace loop with clear cell operation
        ast_free(node->body);
        node->type = AST_CLEAR_CELL;
        node->body = NULL;
    }
    
    
    return node;
}