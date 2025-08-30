#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "dasm_proto.h"
#include "bf_ast.h"
#include "bf_parser.h"

// Flex types and functions
#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

#define BF_MEMORY_SIZE 30000
#define MAX_NESTING 1000

typedef int (*bf_func)(char *memory);

// Include architecture-specific generated C files based on target architecture
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
#include "dasm_x86.h"
#include "bf_amd64.c"
#elif defined(__aarch64__) || defined(__arm64__)
#include "dasm_arm64.h"
#include "bf_arm64.c"
#else
#error "Unsupported architecture"
#endif

static void bf_error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

static char *read_file(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        bf_error("Could not open file");
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(*size + 1);
    if (!content) {
        bf_error("Memory allocation failed");
    }

    fread(content, 1, *size, file);
    content[*size] = '\0';
    fclose(file);

    return content;
}

// Global variable to receive parse result
extern ast_node_t *parse_result;

static ast_node_t* parse_bf_program(const char *program) {
    // Create buffer for parser input
    YY_BUFFER_STATE buffer = yy_scan_string(program);

    parse_result = NULL;
    if (yyparse() != 0) {
        yy_delete_buffer(buffer);
        bf_error("Parser error");
    }

    ast_node_t *result = parse_result;
    yy_delete_buffer(buffer);
    return result;
}

static void dump_code_hex(void *code, size_t size) {
    printf("\nDumping %zu bytes of compiled machine code:\n", size);
    unsigned char *bytes = (unsigned char *)code;
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) printf("%08zx: ", i);
        printf("%02x ", bytes[i]);
        if (i % 16 == 15) printf("\n");
    }
    if (size % 16 != 0) printf("\n");
    printf("\n");
}

// Direct AST compilation with access to static DynASM functions
static void ast_compile_direct(ast_node_t *node, dasm_State **Dst) {
    static int next_label = 0;

    if (!node) return;

    switch (node->type) {
        case AST_MOVE_PTR:
            compile_bf_move_ptr(Dst, node->value);
            break;

        case AST_ADD_VAL:
            compile_bf_add_val(Dst, node->value);
            break;

        case AST_OUTPUT:
            // Use the same call as traditional compiler
            compile_bf_arch(Dst, '.');
            break;

        case AST_INPUT:
            compile_bf_input(Dst);
            break;

        case AST_LOOP: {
            int start_label = next_label++;
            int end_label = next_label++;
            compile_bf_loop_start(Dst, end_label);
            compile_bf_label(Dst, start_label);
            ast_compile_direct(node->body, Dst);
            compile_bf_loop_end(Dst, start_label);
            compile_bf_label(Dst, end_label);
            break;
        }

        case AST_CLEAR_CELL:
            compile_bf_clear_cell(Dst);
            break;

        case AST_COPY_CELL:
            // For now, only support copying current to left ([-<+>] pattern)
            if (node->value == -1) {
                compile_bf_copy_current_to_left(Dst);
            } else {
                // Other copy directions not implemented yet
                // Fall back to original loop
            }
            break;

        case AST_SEQUENCE:
        case AST_MUL_CONST:
        case AST_SET_CONST:
            // These should be handled by optimization passes
            break;
    }

    // Continue with next node
    if (node->next) {
        ast_compile_direct(node->next, Dst);
    }
}

static bf_func compile_bf_ast(ast_node_t *ast, int debug_mode) {
    dasm_State *state = NULL;
    dasm_State **Dst = &state;
    dasm_init(Dst, 1);
    dasm_setup(Dst, actions);

    // Allocate enough PC labels for nested loops
    dasm_growpc(Dst, MAX_NESTING * 2);

    // Architecture-specific prologue
    compile_bf_prologue(Dst);

    // Reset static variables and compile the AST
    ast_compile_direct(ast, Dst);

    // Architecture-specific epilogue
    compile_bf_epilogue(Dst);

    // Link and encode
    size_t size;
    int ret = dasm_link(Dst, &size);
    if (ret != 0) {
        bf_error("DynASM linking failed");
    }

    void *code = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) {
        bf_error("Memory mapping failed");
    }

    ret = dasm_encode(Dst, code);
    if (ret != 0) {
        bf_error("DynASM encoding failed");
    }

    // Make code executable
    if (mprotect(code, size, PROT_READ | PROT_EXEC) != 0) {
        bf_error("Memory protection failed");
    }

    if (debug_mode) {
        dump_code_hex(code, size);
    }

    dasm_free(Dst);
    return (bf_func)code;
}

static bf_func compile_bf(const char *program, int debug_mode) {
    dasm_State *state = NULL;
    dasm_State **Dst = &state;
    dasm_init(Dst, 1);
    dasm_setup(Dst, actions);

    // Allocate enough PC labels for nested loops
    dasm_growpc(Dst, MAX_NESTING * 2);

    int loop_stack[MAX_NESTING];
    int loop_sp = 0;
    int next_label = 0;

    // Architecture-specific prologue
    compile_bf_prologue(Dst);

    for (const char *pc = program; *pc; pc++) {
        switch (*pc) {
            case '>':
                // Check for copy loop optimization >[-<+>]<
                if (pc[1] == '[' && pc[2] == '-' && pc[3] == '<' &&
                    pc[4] == '+' && pc[5] == '>' && pc[6] == ']' && pc[7] == '<') {
                    // Copy right to left pattern detected
                    compile_bf_copy_right_to_left(Dst);
                    pc += 7; // Skip the entire pattern
                    break;
                }
                // Fall through to run-length encoding
            case '<':
            case '+':
            case '-': {
                // Run-length encoding optimization for consecutive operations
                char op = *pc;
                int count = 1;

                // Count consecutive identical operations
                while (pc[1] == op) {
                    count++;
                    pc++;
                }

                // Generate optimized instruction with count
                compile_bf_arch_optimized(Dst, op, count);
                break;
            }
            case '.':
            case ',':
                compile_bf_arch(Dst, *pc);
                break;
            case '[':
                // Check for clear loop optimization [-]
                if (pc[1] == '-' && pc[2] == ']') {
                    // Clear loop detected: [-] -> mov byte [ptr], #0
                    compile_bf_clear_cell(Dst);
                    pc += 2; // Skip the '-]'
                    break;
                }

                if (loop_sp >= MAX_NESTING) {
                    bf_error("Too many nested loops");
                }
                int loop_start = next_label++;
                int loop_end = next_label++;
                loop_stack[loop_sp] = loop_start;
                loop_sp++;
                compile_bf_loop_start(Dst, loop_end);
                compile_bf_label(Dst, loop_start);
                break;
            case ']':
                if (loop_sp == 0) {
                    bf_error("Unmatched ']'");
                }
                loop_sp--;
                int back_to_start = loop_stack[loop_sp];
                int loop_exit = back_to_start + 1;
                compile_bf_loop_end(Dst, back_to_start);
                compile_bf_label(Dst, loop_exit);
                break;
        }
    }

    if (loop_sp != 0) {
        bf_error("Unmatched '['");
    }

    // Architecture-specific epilogue
    compile_bf_epilogue(Dst);

    void *code = NULL;
    size_t size;
    dasm_link(Dst, &size);

    code = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (code == MAP_FAILED) {
        bf_error("mmap failed");
    }

    dasm_encode(Dst, code);
    dasm_free(Dst);

    if (debug_mode) {
        dump_code_hex(code, size);
    }

    if (mprotect(code, size, PROT_READ | PROT_EXEC) != 0) {
        bf_error("mprotect failed");
    }

    return (bf_func)code;
}

int main(int argc, char *argv[]) {
    int debug_mode = 0;
    int use_ast = 1; // Default to AST compiler
    int arg_offset = 1;

    // Parse flags
    for (int i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            debug_mode = 1;
            arg_offset++;
        } else if (strcmp(argv[i], "--trad") == 0) {
            use_ast = 0;
            arg_offset++;
        } else {
            fprintf(stderr, "Unknown flag: %s\n", argv[i]);
            return 1;
        }
    }

    if (argc < arg_offset + 1) {
        fprintf(stderr, "Usage: %s [-d] [--trad] <brainfuck_file>\n", argv[0]);
        fprintf(stderr, "  -d: Enable debug mode (dump compiled code)\n");
        fprintf(stderr, "  --trad: Use traditional string-based compiler instead of AST\n");
        return 1;
    }

    size_t program_size;
    char *program = read_file(argv[arg_offset], &program_size);

    bf_func compiled_program;
    ast_node_t *ast = NULL;

    if (use_ast) {
        // Parse program into AST
        ast = parse_bf_program(program);

        if (debug_mode) {
            printf("Original AST dump:\n");
            ast_print(ast, 0);
            printf("\n");
        }

        // Optimize the AST
        ast = ast_optimize(ast);

        if (debug_mode) {
            printf("Optimized AST dump:\n");
            ast_print(ast, 0);
            printf("\n");
        }

        compiled_program = compile_bf_ast(ast, debug_mode);
    } else {
        // Use traditional string-based compiler
        compiled_program = compile_bf(program, debug_mode);
    }

    char *memory = calloc(BF_MEMORY_SIZE, 1);
    if (!memory) {
        bf_error("Memory allocation failed");
    }
    compiled_program(memory);

    free(program);
    free(memory);
    if (ast) {
        ast_free(ast);
    }

    return 0;
}