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
    node->data.basic.count = count;
    return node;
}

ast_node_t* ast_create_add(int count) {
    ast_node_t *node = ast_create_node(AST_ADD_VAL);
    node->data.basic.count = count;
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
    node->data.loop.body = body;
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

ast_node_t* ast_create_copy_cell(int src_offset, int dst_offset) {
    ast_node_t *node = ast_create_node(AST_COPY_CELL);
    node->data.copy.src_offset = src_offset;
    node->data.copy.dst_offset = dst_offset;
    return node;
}

ast_node_t* ast_create_mul_const(int multiplier, int src_offset, int dst_offset) {
    ast_node_t *node = ast_create_node(AST_MUL_CONST);
    node->data.mul_const.multiplier = multiplier;
    node->data.mul_const.src_offset = src_offset;
    node->data.mul_const.dst_offset = dst_offset;
    return node;
}

ast_node_t* ast_create_set_const(int value) {
    ast_node_t *node = ast_create_node(AST_SET_CONST);
    node->data.set_const.value = value;
    return node;
}

ast_node_t* ast_create_add_at_offset(int value, int offset) {
    ast_node_t *node = ast_create_node(AST_ADD_VAL_AT_OFFSET);
    node->data.add_at_offset.value = value;
    node->data.add_at_offset.offset = offset;
    return node;
}

// Memory management
void ast_free(ast_node_t *node) {
    if (!node) return;
    
    if (node->type == AST_LOOP) {
        ast_free(node->data.loop.body);
    }
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
        case AST_CLEAR_CELL: return "CLEAR_CELL";
        case AST_COPY_CELL: return "COPY_CELL";
        case AST_MUL_CONST: return "MUL_CONST";
        case AST_SET_CONST: return "SET_CONST";
        case AST_ADD_VAL_AT_OFFSET: return "ADD_VAL_AT_OFFSET";
        default: return "UNKNOWN";
    }
}

void ast_print(ast_node_t *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) fprintf(stderr, "  ");
    
    fprintf(stderr, "%s", ast_type_name(node->type));
    switch (node->type) {
        case AST_MOVE_PTR:
        case AST_ADD_VAL:
            if (node->data.basic.count != 0) fprintf(stderr, " (count: %d)", node->data.basic.count);
            break;
        case AST_MUL_CONST:
            fprintf(stderr, " (mult: %d, src: %d, dst: %d)", 
                   node->data.mul_const.multiplier,
                   node->data.mul_const.src_offset, 
                   node->data.mul_const.dst_offset);
            break;
        case AST_COPY_CELL:
            fprintf(stderr, " (src: %d, dst: %d)", 
                   node->data.copy.src_offset, 
                   node->data.copy.dst_offset);
            break;
        case AST_SET_CONST:
            fprintf(stderr, " (value: %d)", node->data.set_const.value);
            break;
        case AST_ADD_VAL_AT_OFFSET:
            fprintf(stderr, " (value: %d, offset: %d)", 
                   node->data.add_at_offset.value, 
                   node->data.add_at_offset.offset);
            break;
        default:
            break;
    }
    fprintf(stderr, "\n");
    
    // For loops, print the body with increased indentation
    if (node->type == AST_LOOP && node->data.loop.body) {
        ast_print(node->data.loop.body, indent + 1);
    }
    
    // For all nodes, continue with the next node at same level
    if (node->next) {
        ast_print(node->next, indent);
    }
}

// AST traversal for code generation is now in bf.c to access static DynASM functions

// Helper function to detect multiplication pattern with clear hint
static int is_safe_multiplication_pattern(ast_node_t *node) {
    // Look for pattern: SET_VAL(n) followed by [>ADD_VAL(m)<ADD_VAL(-1)]
    // where target is at program start (position 0) or after explicit clear
    
    if (!node || node->type != AST_ADD_VAL || node->data.basic.count <= 0) return 0;
    if (!node->next || node->next->type != AST_LOOP) return 0;
    
    ast_node_t *loop_body = node->next->data.loop.body;
    if (!loop_body) return 0;
    
    // Check loop pattern: MOVE_PTR(offset) -> ADD_VAL(m) -> MOVE_PTR(-offset) -> ADD_VAL(-1)
    if (loop_body->type == AST_MOVE_PTR && loop_body->data.basic.count > 0 &&
        loop_body->next && loop_body->next->type == AST_ADD_VAL && loop_body->next->data.basic.count > 0 &&
        loop_body->next->next && loop_body->next->next->type == AST_MOVE_PTR && 
        loop_body->next->next->data.basic.count == -loop_body->data.basic.count &&
        loop_body->next->next->next && loop_body->next->next->next->type == AST_ADD_VAL && 
        loop_body->next->next->next->data.basic.count == -1 &&
        !loop_body->next->next->next->next) {
        
        return 1;
    }
    
    return 0;
}

// AST optimization - combine consecutive operations
ast_node_t* ast_optimize(ast_node_t *node) {
    if (!node) return NULL;
    
    // First optimize children recursively
    if (node->type == AST_LOOP && node->data.loop.body) {
        node->data.loop.body = ast_optimize(node->data.loop.body);
    }
    if (node->next) {
        node->next = ast_optimize(node->next);
    }
    
    // Run-length encoding: combine consecutive ADD_VAL or MOVE_PTR operations
    if (node->next && node->type == node->next->type) {
        if (node->type == AST_ADD_VAL || node->type == AST_MOVE_PTR) {
            // Combine with next node
            node->data.basic.count += node->next->data.basic.count;
            ast_node_t *old_next = node->next;
            node->next = node->next->next;
            free(old_next);
            
            // Continue optimizing from current node (might combine further)
            return ast_optimize(node);
        }
    }
    
    // Clear loop optimization: detect [-] pattern
    if (node->type == AST_LOOP && node->data.loop.body && 
        node->data.loop.body->type == AST_ADD_VAL && node->data.loop.body->data.basic.count == -1 &&
        !node->data.loop.body->next) {
        // Replace loop with clear cell operation
        ast_free(node->data.loop.body);
        node->type = AST_CLEAR_CELL;
    }
    
    // Copy loop optimization: detect [-<+>] pattern (copy current to left)
    if (node->type == AST_LOOP && node->data.loop.body) {
        ast_node_t *curr = node->data.loop.body;
        // Check pattern: ADD_VAL(-1) -> MOVE_PTR(-1) -> ADD_VAL(1) -> MOVE_PTR(1)
        if (curr->type == AST_ADD_VAL && curr->data.basic.count == -1 &&
            curr->next && curr->next->type == AST_MOVE_PTR && curr->next->data.basic.count == -1 &&
            curr->next->next && curr->next->next->type == AST_ADD_VAL && curr->next->next->data.basic.count == 1 &&
            curr->next->next->next && curr->next->next->next->type == AST_MOVE_PTR && curr->next->next->next->data.basic.count == 1 &&
            !curr->next->next->next->next) {
            // This is [-<+>] which copies current cell to left cell and clears current
            // Replace with copy cell operation
            ast_free(node->data.loop.body);
            node->type = AST_COPY_CELL;
            node->data.copy.src_offset = 0;  // src is current position  
            node->data.copy.dst_offset = -1;  // dst is left (-1 offset)
        }
    }
    
    // Multiplication loop optimization: detect SET_VAL + [>MUL<-] patterns  
    if (is_safe_multiplication_pattern(node)) {
        ast_node_t *setup = node;
        ast_node_t *loop = node->next;
        ast_node_t *loop_body = loop->data.loop.body;
        
        // Extract pattern values
        int multiplier = loop_body->next->data.basic.count; // Multiplier  
        int target_offset = loop_body->data.basic.count;    // Target offset
        
        // Create new MUL_CONST node to replace the loop
        ast_node_t *mul_node = ast_create_mul_const(multiplier, 0, target_offset);
        
        // Insert multiplication after setup, replace loop
        mul_node->next = loop->next;
        setup->next = mul_node;
        
        // Free the loop
        ast_free(loop->data.loop.body);
        free(loop);
    }
    
    // Offset ADD optimization: detect MOVE_PTR + ADD_VAL + MOVE_PTR patterns
    if (node->type == AST_MOVE_PTR && node->next && 
        node->next->type == AST_ADD_VAL && node->next->next &&
        node->next->next->type == AST_MOVE_PTR && 
        node->next->next->data.basic.count == -node->data.basic.count) {
        
        int offset = node->data.basic.count;
        int value = node->next->data.basic.count;
        ast_node_t *third = node->next->next;
        
        // Create ADD_VAL_AT_OFFSET node
        ast_node_t *offset_add = ast_create_add_at_offset(value, offset);
        offset_add->next = third->next;
        
        // Free the three nodes we're replacing
        free(node->next);
        free(third);
        
        // Replace current node
        *node = *offset_add;
        free(offset_add);
        
        // Continue optimizing from current node
        return ast_optimize(node);
    }
    
    return node;
}