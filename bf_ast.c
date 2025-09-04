#include "bf_ast.h"
#include <string.h>
#include <stdbool.h>

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

    ast_node_t *current = first;
    while (current->next) {
        current = current->next;
    }
    current->next = second;
    return first;
}


ast_node_t* ast_create_copy_cell(int src_offset, int dst_offset) {
    ast_node_t *node = ast_create_node(AST_COPY_CELL);
    node->data.copy.src_offset = src_offset;
    node->data.copy.dst_offset = dst_offset;
    return node;
}

ast_node_t* ast_create_set_const(int value, int offset) {
    ast_node_t *node = ast_create_node(AST_SET_CONST);
    node->data.basic.count = value;
    node->data.basic.offset = offset;
    return node;
}

ast_node_t* ast_create_mul(int multiplier, int src_offset, int dst_offset) {
    ast_node_t *node = ast_create_node(AST_MUL);
    node->data.mul.multiplier = multiplier;
    node->data.mul.src_offset = src_offset;
    node->data.mul.dst_offset = dst_offset;
    return node;
}

void ast_free(ast_node_t *node) {
    if (!node) return;

    if (node->type == AST_LOOP) {
        ast_free(node->data.loop.body);
    }
    ast_free(node->next);
    free(node);
}

static const char* ast_type_name(ast_node_type_t type) {
    switch (type) {
        case AST_MOVE_PTR: return "MOVE_PTR";
        case AST_ADD_VAL: return "ADD_VAL";
        case AST_OUTPUT: return "OUTPUT";
        case AST_INPUT: return "INPUT";
        case AST_LOOP: return "LOOP";
        case AST_COPY_CELL: return "COPY_CELL";
        case AST_SET_CONST: return "SET_CONST";
        case AST_MUL: return "MUL";
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
        case AST_COPY_CELL:
            fprintf(stderr, " (src: %d, dst: %d)",
                   node->data.copy.src_offset,
                   node->data.copy.dst_offset);
            break;
        case AST_SET_CONST:
            if (node->data.basic.offset != 0) {
                fprintf(stderr, " (value: %d, offset: %d)", node->data.basic.count, node->data.basic.offset);
            } else {
                fprintf(stderr, " (value: %d)", node->data.basic.count);
            }
            break;
        case AST_MUL:
            fprintf(stderr, " (%d*[%d] -> [%d])",
                   node->data.mul.multiplier,
                   node->data.mul.src_offset,
                   node->data.mul.dst_offset);
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
    if (node->line > 0 || node->column > 0) {
        fprintf(stderr, " \033[90m@%d:%d\033[0m", node->line, node->column);
    }
    fprintf(stderr, "\n");

    if (node->type == AST_LOOP && node->data.loop.body) {
        ast_print(node->data.loop.body, indent + 1);
    }

    if (node->next) {
        ast_print(node->next, indent);
    }
}

static bool is_multiplication_loop(ast_node_t *loop) {
    if (loop->type != AST_LOOP || !loop->data.loop.body) return false;

    ast_node_t *body = loop->data.loop.body;
    bool has_counter_decrement = false;

    for (ast_node_t *op = body; op; op = op->next) {
        if (op->type == AST_ADD_VAL) {
            if (op->data.basic.offset == 0) {
                if (op->data.basic.count == -1 && !has_counter_decrement) {
                    has_counter_decrement = true;
                } else {
                    return false; // Multiple or wrong counter modifications
                }
            }
        } else if (op->type == AST_MOVE_PTR) {
            continue;
        } else {
            return false;
        }
    }

    return has_counter_decrement;
}

ast_node_t* ast_optimize(ast_node_t *node) {
    if (!node) return NULL;

    // Run-length encoding: combine consecutive ADD_VAL or MOVE_PTR operations FIRST
    if (node->next && node->type == node->next->type) {
        if (node->type == AST_MOVE_PTR) {
            node->data.basic.count += node->next->data.basic.count;
            ast_node_t *old_next = node->next;
            node->next = node->next->next;
            free(old_next);

            return ast_optimize(node);
        } else if (node->type == AST_ADD_VAL &&
                   node->data.basic.offset == node->next->data.basic.offset) {
            node->data.basic.count += node->next->data.basic.count;
            ast_node_t *old_next = node->next;
            node->next = node->next->next;
            free(old_next);

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
        // Replace loop with set constant zero operation - preserve location
        ast_free(node->data.loop.body);
        node->type = AST_SET_CONST;
        node->data.basic.count = 0;
        node->data.basic.offset = 0;
        // Location is already preserved in node
    }

    // Multiplication loop optimization: detect loops with only ADD_VAL operations
    if (node->type == AST_LOOP && node->data.loop.body && is_multiplication_loop(node)) {
        ast_node_t *loop_body = node->data.loop.body;

        // CRITICAL: Capture the next node BEFORE we free loop_body
        ast_node_t *original_next = node->next;

        // Create individual MUL nodes for each ADD_VAL operation (except counter decrement)
        ast_node_t *first_mul = NULL;
        ast_node_t *last_mul = NULL;

        for (ast_node_t *op = loop_body; op; op = op->next) {
            if (op->type == AST_ADD_VAL && op->data.basic.offset != 0) {
                ast_node_t *new_node;

                if (op->data.basic.count == 1) {
                    // Use COPY_CELL for multiplier = 1 (copy from source to destination)
                    new_node = ast_create_copy_cell(0, op->data.basic.offset);
                } else {
                    // Use MUL for other multipliers
                    new_node = ast_create_mul(op->data.basic.count, 0, op->data.basic.offset);
                }

                // Preserve location from original loop
                ast_copy_location(new_node, node);

                if (!first_mul) {
                    first_mul = last_mul = new_node;
                } else {
                    last_mul->next = new_node;
                    last_mul = new_node;
                }
            }
        }

        // Add SET_CONST(0) to clear the counter
        ast_node_t *clear_counter = ast_create_set_const(0, 0);
        ast_copy_location(clear_counter, node); // Preserve location from original loop
        if (last_mul) {
            last_mul->next = clear_counter;
        } else {
            first_mul = clear_counter;
        }
        clear_counter->next = original_next;

        // Free the original loop
        ast_free(loop_body);

        // Replace current node with first MUL node
        if (first_mul) {
            // Directly copy the node data (removing memset that might cause issues)
            node->type = first_mul->type;
            node->data = first_mul->data;
            node->next = first_mul->next;
            node->line = first_mul->line;
            node->column = first_mul->column;

            // Free only the first_mul node structure, not its contents
            free(first_mul);
        }

        return ast_optimize(node);
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
        ast_copy_location(offset_add, node); // Preserve location from first node
        offset_add->next = third->next;

        // Free the three nodes we're replacing
        free(node->next);
        free(third);

        // Replace current node
        node->type = offset_add->type;
        node->data = offset_add->data;
        node->next = offset_add->next;
        node->line = offset_add->line;
        node->column = offset_add->column;
        free(offset_add);

        // Continue optimizing from current node
        return ast_optimize(node);
    }

    // Set + Add coalescing: detect SET_CONST followed by ADD_VAL at same offset
    if (node->type == AST_SET_CONST && node->next &&
        node->next->type == AST_ADD_VAL && node->next->data.basic.offset == 0) {

        int final_value = node->data.basic.count + node->next->data.basic.count;
        ast_node_t *add_node = node->next;

        // Update SET_CONST with final value
        node->data.basic.count = final_value;
        node->next = add_node->next;

        // Free the ADD_VAL node
        free(add_node);

        // Continue optimizing from current node
        return ast_optimize(node);
    }

    return node;
}

// Sequence rewriting optimization: coalesce pointer movements and use offsets
ast_node_t* ast_rewrite_sequences(ast_node_t *node) {
    if (!node) return NULL;

    // Process this level - find sequences of operations between control flow
    ast_node_t *current = node;

    while (current) {
        // Find the start of a basic block (sequence without control flow)
        ast_node_t *block_start = current;
        ast_node_t *block_end = current;
        int total_ptr_movement = 0;
        int current_offset = 0;

        // Scan forward to find the end of this basic block
        bool has_ptr_movements = false;
        while (block_end && block_end->type != AST_LOOP) {
            if (block_end->type == AST_MOVE_PTR) {
                total_ptr_movement += block_end->data.basic.count;
                current_offset += block_end->data.basic.count;
                has_ptr_movements = true;
            }

            // If this is the last node or next node is a loop, end the block
            if (!block_end->next || block_end->next->type == AST_LOOP) {
                break;
            }
            block_end = block_end->next;
        }

        // Capture the next node before we modify the structure
        ast_node_t *original_next = block_end ? block_end->next : NULL;

        // If we have a block with pointer movements, rewrite it
        if (block_start != block_end && has_ptr_movements) {
            ast_node_t *rewrite_current = block_start;
            ast_node_t *first_move_node = NULL; // Track first MOVE_PTR for location
            current_offset = 0;

            while (rewrite_current && rewrite_current != original_next) {
                if (rewrite_current->type == AST_MOVE_PTR) {
                    current_offset += rewrite_current->data.basic.count;

                    // Remember the first MOVE_PTR node for source location
                    if (!first_move_node) {
                        first_move_node = rewrite_current;
                    }

                    // Mark this MOVE_PTR for removal by setting count to 0
                    rewrite_current->data.basic.count = 0;

                } else if (rewrite_current->type == AST_ADD_VAL) {
                    // Update ADD_VAL to use current offset
                    rewrite_current->data.basic.offset += current_offset;

                } else if (rewrite_current->type == AST_INPUT || rewrite_current->type == AST_OUTPUT) {
                    // Update INPUT/OUTPUT to use current offset
                    rewrite_current->data.basic.offset += current_offset;

                } else if (rewrite_current->type == AST_SET_CONST) {
                    // Update SET_CONST to use current offset
                    rewrite_current->data.basic.offset += current_offset;

                } else if (rewrite_current->type == AST_COPY_CELL) {
                    // Update COPY_CELL offsets
                    rewrite_current->data.copy.src_offset += current_offset;
                    rewrite_current->data.copy.dst_offset += current_offset;
                }

                rewrite_current = rewrite_current->next;
            }

            // Add a single MOVE_PTR at the end if needed
            if (total_ptr_movement != 0) {
                ast_node_t *final_move = ast_create_move(total_ptr_movement);
                if (first_move_node) {
                    ast_copy_location(final_move, first_move_node);
                }
                final_move->next = original_next;
                block_end->next = final_move;
            }
        }

        // Move to next block
        if (block_end && block_end->type == AST_LOOP) {
            // Recursively process the loop body
            if (block_end->data.loop.body) {
                block_end->data.loop.body = ast_rewrite_sequences(block_end->data.loop.body);
            }
            current = original_next;
        } else {
            current = original_next;
        }
    }

    // Remove MOVE_PTR nodes with count=0 (marked for removal)
    ast_node_t *prev = NULL;
    current = node;

    while (current) {
        if (current->type == AST_MOVE_PTR && current->data.basic.count == 0) {
            // Remove this node
            if (prev) {
                prev->next = current->next;
                free(current);
                current = prev->next;
            } else {
                // Removing the first node
                node = current->next;
                free(current);
                current = node;
            }
        } else {
            prev = current;
            current = current->next;
        }
    }

    return node;
}

int ast_count_nodes(ast_node_t *node) {
    if (!node) return 0;

    int count = 1; // Count current node

    // Count nodes in loop body if this is a loop
    if (node->type == AST_LOOP && node->data.loop.body) {
        count += ast_count_nodes(node->data.loop.body);
    }

    // Count next node in sequence
    if (node->next) {
        count += ast_count_nodes(node->next);
    }

    return count;
}

void ast_set_location(ast_node_t *node, int line, int column) {
    if (node) {
        node->line = line;
        node->column = column;
    }
}

void ast_copy_location(ast_node_t *dst, ast_node_t *src) {
    if (dst && src) {
        dst->line = src->line;
        dst->column = src->column;
    }
}
