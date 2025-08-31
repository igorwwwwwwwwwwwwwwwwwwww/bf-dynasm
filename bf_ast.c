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

ast_node_t* ast_create_add(int count, int offset) {
    ast_node_t *node = ast_create_node(AST_ADD_VAL);
    node->data.basic.count = count;
    node->data.basic.offset = offset;
    return node;
}

ast_node_t* ast_create_output(int offset) {
    ast_node_t *node = ast_create_node(AST_OUTPUT);
    node->data.basic.offset = offset;
    return node;
}

ast_node_t* ast_create_input(int offset) {
    ast_node_t *node = ast_create_node(AST_INPUT);
    node->data.basic.offset = offset;
    return node;
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
        case AST_COPY_CELL: return "COPY_CELL";
        case AST_MUL_CONST: return "MUL_CONST";
        case AST_SET_CONST: return "SET_CONST";
        default: return "UNKNOWN";
    }
}

void ast_print(ast_node_t *node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) fprintf(stderr, "  ");

    fprintf(stderr, "%s", ast_type_name(node->type));
    switch (node->type) {
        case AST_MOVE_PTR:
            if (node->data.basic.count != 0) fprintf(stderr, " (count: %d)", node->data.basic.count);
            break;
        case AST_ADD_VAL:
            if (node->data.basic.offset != 0) {
                fprintf(stderr, " (count: %d, offset: %d)", node->data.basic.count, node->data.basic.offset);
            } else {
                fprintf(stderr, " (count: %d)", node->data.basic.count);
            }
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
        case AST_INPUT:
        case AST_OUTPUT:
            if (node->data.basic.offset != 0) {
                fprintf(stderr, " (offset: %d)", node->data.basic.offset);
            }
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

    // Run-length encoding: combine consecutive ADD_VAL or MOVE_PTR operations FIRST
    if (node->next && node->type == node->next->type) {
        if (node->type == AST_MOVE_PTR) {
            // MOVE_PTR operations can always be combined
            node->data.basic.count += node->next->data.basic.count;
            ast_node_t *old_next = node->next;
            node->next = node->next->next;
            free(old_next);

            // Continue optimizing from current node (might combine further)
            return ast_optimize(node);
        } else if (node->type == AST_ADD_VAL &&
                   node->data.basic.offset == node->next->data.basic.offset) {
            // ADD_VAL operations can only be combined if they have the same offset
            node->data.basic.count += node->next->data.basic.count;
            ast_node_t *old_next = node->next;
            node->next = node->next->next;
            free(old_next);

            // Continue optimizing from current node (might combine further)
            return ast_optimize(node);
        }
    }

    // Copy loop optimization: detect copy patterns BEFORE recursive optimization
    // This must come before child optimization to catch the original MOVE_PTR patterns
    if (node->type == AST_LOOP && node->data.loop.body) {
        ast_node_t *curr = node->data.loop.body;

        // Pattern 1: [>+<-] -> MOVE_PTR(offset) -> ADD_VAL(1) -> MOVE_PTR(-offset) -> ADD_VAL(-1)
        if (curr->type == AST_MOVE_PTR &&
            curr->next && curr->next->type == AST_ADD_VAL && curr->next->data.basic.count == 1 &&
            curr->next->next && curr->next->next->type == AST_MOVE_PTR &&
            curr->next->next->data.basic.count == -curr->data.basic.count &&
            curr->next->next->next && curr->next->next->next->type == AST_ADD_VAL &&
            curr->next->next->next->data.basic.count == -1 &&
            !curr->next->next->next->next) {
            // This is [>+<-] -> [MOVE+MOVE_BACK-] which copies current cell to offset and clears current
            int offset = curr->data.basic.count;
            // Replace with copy cell operation followed by explicit clear
            ast_free(node->data.loop.body);

            // Create COPY_CELL (copy only, no clear)
            node->type = AST_COPY_CELL;
            node->data.copy.src_offset = 0;   // src is current position
            node->data.copy.dst_offset = offset;  // dst is at offset

            // Create SET_CONST(0) for explicit clearing and chain it
            ast_node_t *clear_node = ast_create_set_const(0);
            node->next = ast_create_sequence(clear_node, node->next);

            // Continue optimizing from current node
            return ast_optimize(node);
        }

        // Pattern 2: [-<+>] -> ADD_VAL(-1) -> MOVE_PTR(offset) -> ADD_VAL(1) -> MOVE_PTR(-offset)
        else if (curr->type == AST_ADD_VAL && curr->data.basic.count == -1 &&
            curr->next && curr->next->type == AST_MOVE_PTR &&
            curr->next->next && curr->next->next->type == AST_ADD_VAL && curr->next->next->data.basic.count == 1 &&
            curr->next->next->next && curr->next->next->next->type == AST_MOVE_PTR &&
            curr->next->next->next->data.basic.count == -curr->next->data.basic.count &&
            !curr->next->next->next->next) {
            // This is [-<+>] -> [-MOVE+MOVE_BACK] which copies current cell to offset and clears current
            int offset = curr->next->data.basic.count;
            // Replace with copy cell operation followed by explicit clear
            ast_free(node->data.loop.body);

            // Create COPY_CELL (copy only, no clear)
            node->type = AST_COPY_CELL;
            node->data.copy.src_offset = 0;   // src is current position
            node->data.copy.dst_offset = offset;  // dst is at offset

            // Create SET_CONST(0) for explicit clearing and chain it
            ast_node_t *clear_node = ast_create_set_const(0);
            node->next = ast_create_sequence(clear_node, node->next);

            // Continue optimizing from current node
            return ast_optimize(node);
        }
    }

    // Now optimize children recursively (after copy detection)
    if (node->type == AST_LOOP && node->data.loop.body) {
        node->data.loop.body = ast_optimize(node->data.loop.body);
    }
    if (node->next) {
        node->next = ast_optimize(node->next);
    }

    // Clear loop optimization: detect [-] pattern
    if (node->type == AST_LOOP && node->data.loop.body &&
        node->data.loop.body->type == AST_ADD_VAL && node->data.loop.body->data.basic.count == -1 &&
        !node->data.loop.body->next) {
        // Replace loop with set constant zero operation
        ast_free(node->data.loop.body);
        node->type = AST_SET_CONST;
        node->data.set_const.value = 0;
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

        // Create ADD_VAL node with offset
        ast_node_t *offset_add = ast_create_add(value, offset);
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

    // Set + Add coalescing: detect SET_CONST followed by ADD_VAL
    if (node->type == AST_SET_CONST && node->next &&
        node->next->type == AST_ADD_VAL) {

        int final_value = node->data.set_const.value + node->next->data.basic.count;
        ast_node_t *add_node = node->next;

        // Update SET_CONST with final value
        node->data.set_const.value = final_value;
        node->next = add_node->next;

        // Free the ADD_VAL node
        free(add_node);

        // Continue optimizing from current node
        return ast_optimize(node);
    }

    return node;
}
