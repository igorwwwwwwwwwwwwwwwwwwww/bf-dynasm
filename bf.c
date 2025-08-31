#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#include "dasm_proto.h"
#include "bf_ast.h"

#include "bf_parser.h"

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
static int ast_compile_direct(ast_node_t *node, dasm_State **Dst, int next_label) {
    if (!node) return next_label;

    switch (node->type) {
        case AST_MOVE_PTR:
            compile_bf_move_ptr(Dst, node->data.basic.count);
            break;

        case AST_ADD_VAL:
            compile_bf_add_val(Dst, node->data.basic.count);
            break;

        case AST_OUTPUT:
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
            next_label = ast_compile_direct(node->data.loop.body, Dst, next_label);
            compile_bf_loop_end(Dst, start_label);
            compile_bf_label(Dst, end_label);
            break;
        }

        case AST_CLEAR_CELL:
            compile_bf_clear_cell(Dst);
            break;

        case AST_COPY_CELL:
            assert(node->data.copy.dst_offset == -1); // Only [-<+>] pattern is currently supported
            compile_bf_copy_current_to_left(Dst);
            break;

        case AST_MUL_CONST:
            compile_bf_mul_const(Dst, node->data.mul_const.multiplier, node->data.mul_const.dst_offset);
            break;
        case AST_SET_CONST:
            assert(0); // Not implemented
            break;
    }

    // Continue with next node
    if (node->next) {
        next_label = ast_compile_direct(node->next, Dst, next_label);
    }
    
    return next_label;
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

    // Compile the AST (starting with label 0)
    ast_compile_direct(ast, Dst, 0);

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


int main(int argc, char *argv[]) {
    int debug_mode = 0;
    int optimize = 1;
    int arg_offset = 1;

    // Parse flags
    for (int i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
            arg_offset++;
        } else if (strcmp(argv[i], "--no-optimize") == 0) {
            optimize = 0;
            arg_offset++;
        } else {
            fprintf(stderr, "Unknown flag: %s\n", argv[i]);
            return 1;
        }
    }

    if (argc < arg_offset + 1) {
        fprintf(stderr, "Usage: %s [options] <brainfuck_file>\n", argv[0]);
        fprintf(stderr, "  --debug: Enable debug mode (dump compiled code)\n");
        fprintf(stderr, "  --no-optimize: Disable optimizations\n");
        return 1;
    }

    size_t program_size;
    char *program = read_file(argv[arg_offset], &program_size);

    bf_func compiled_program;
    ast_node_t *ast = NULL;

    // Parse program into AST
    ast = parse_bf_program(program);

    // Optimize the AST if optimizations are enabled
    if (optimize) {
        ast = ast_optimize(ast);
    }

    if (debug_mode) {
        printf("%s AST dump:\n", optimize ? "Optimized" : "Unoptimized");
        ast_print(ast, 0);
    }

    compiled_program = compile_bf_ast(ast, debug_mode);

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