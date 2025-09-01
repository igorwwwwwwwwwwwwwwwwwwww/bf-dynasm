#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdbool.h>

#include "dasm_proto.h"
#include "bf_ast.h"
#include "bf_prof.h"
#include "bf_debug.h"

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
    fprintf(stderr, "\nDumping %zu bytes of compiled machine code:\n", size);
    unsigned char *bytes = (unsigned char *)code;
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) fprintf(stderr, "%08zx: ", i);
        fprintf(stderr, "%02x ", bytes[i]);
        if (i % 16 == 15) fprintf(stderr, "\n");
    }
    if (size % 16 != 0) fprintf(stderr, "\n");
    fprintf(stderr, "\n");
}

static int ast_compile_direct(ast_node_t *node, dasm_State **Dst, int next_label, bf_debug_info_t *debug, int *debug_label) {
    if (!node) return next_label;

    if (debug && debug_label) {
        int current_debug_label = (*debug_label)++;
        bf_debug_add_mapping(debug, current_debug_label, node, node->line, node->column);
        compile_bf_debug_label(Dst, current_debug_label);
    }

    switch (node->type) {
        case AST_MOVE_PTR:
            compile_bf_move_ptr(Dst, node->data.basic.count);
            break;

        case AST_ADD_VAL:
            compile_bf_add_val(Dst, node->data.basic.count, node->data.basic.offset);
            break;

        case AST_OUTPUT:
            compile_bf_output(Dst, node->data.basic.offset);
            break;

        case AST_INPUT:
            compile_bf_input(Dst, node->data.basic.offset);
            break;

        case AST_LOOP: {
            int start_label = next_label++;
            int end_label = next_label++;
            compile_bf_loop_start(Dst, end_label);
            compile_bf_label(Dst, start_label);
            next_label = ast_compile_direct(node->data.loop.body, Dst, next_label, debug, debug_label);
            compile_bf_loop_end(Dst, start_label);
            compile_bf_label(Dst, end_label);
            break;
        }

        case AST_COPY_CELL:
            compile_bf_copy_cell(Dst, node->data.copy.src_offset, node->data.copy.dst_offset);
            break;

        case AST_SET_CONST:
            compile_bf_set_const(Dst, node->data.basic.count, node->data.basic.offset);
            break;

        case AST_MUL:
            compile_bf_mul(Dst, node->data.mul.multiplier, node->data.mul.src_offset, node->data.mul.dst_offset);
            break;
    }

    // Continue with next node
    if (node->next) {
        next_label = ast_compile_direct(node->next, Dst, next_label, debug, debug_label);
    }

    return next_label;
}

static bf_func compile_bf_ast(ast_node_t *ast, bool debug_mode, void **code_ptr, size_t *code_size, bf_debug_info_t *debug_info) {
    dasm_State *state = NULL;
    dasm_State **Dst = &state;
    dasm_init(Dst, 1);
    dasm_setup(Dst, actions);

    int debug_label_count = debug_info ? ast_count_nodes(ast) : 0;
    dasm_growpc(Dst, MAX_NESTING * 2 + debug_label_count);

    compile_bf_prologue(Dst);

    int debug_label_counter = MAX_NESTING * 2; // Start debug labels after loop labels
    ast_compile_direct(ast, Dst, 0, debug_info, debug_info ? &debug_label_counter : NULL);

    compile_bf_epilogue(Dst);

    size_t size;
    int ret = dasm_link(Dst, &size);
    if (ret != 0) {
        bf_error("DynASM linking failed");
    }

    void *code = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) {
        bf_error("Memory mapping failed");
    }

    // Resolve debug labels BEFORE encoding - after this dasm_getpclabel corrupts state
    if (debug_info) {
        for (int i = 0; i < debug_info->entry_count; i++) {
            debug_map_entry_t *entry = &debug_info->entries[i];
            int32_t ofs = dasm_getpclabel(Dst, entry->pc_label);
            if (ofs >= 0) {
                entry->pc_offset = (size_t)ofs;
            }
        }
    }

    ret = dasm_encode(Dst, code);
    if (ret != 0) {
        bf_error("DynASM encoding failed");
    }

    if (mprotect(code, size, PROT_READ | PROT_EXEC) != 0) {
        bf_error("Memory protection failed");
    }

    if (debug_mode) {
        dump_code_hex(code, size);
    }

    if (code_ptr) *code_ptr = code;
    if (code_size) *code_size = size;

    dasm_free(Dst);
    return (bf_func)code;
}


int main(int argc, char *argv[]) {
    bool debug_mode = false;
    bool optimize = true;
    bool show_help = false;
    bool profile_mode = false;
    const char *profile_output = NULL;
    int arg_offset = 1;

    for (int i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
            arg_offset++;
        } else if (strcmp(argv[i], "--no-optimize") == 0) {
            optimize = false;
            arg_offset++;
        } else if (strcmp(argv[i], "--profile") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --profile requires a filename\n");
                return 1;
            }
            profile_mode = true;
            profile_output = argv[i + 1];
            i++;
            arg_offset += 2;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            show_help = true;
            arg_offset++;
            break;
        } else {
            fprintf(stderr, "Unknown flag: %s\n", argv[i]);
            return 1;
        }
    }

    if (show_help || argc < arg_offset + 1) {
        FILE *stream = show_help ? stdout : stderr;
        fprintf(stream, "Usage: %s [options] <brainfuck_file>\n", argv[0]);
        fprintf(stream, "\nOptions:\n");
        fprintf(stream, "  --help, -h        Show this help message\n");
        fprintf(stream, "  --debug           Enable debug mode (dump AST and compiled code)\n");
        fprintf(stream, "  --no-optimize     Disable AST optimizations\n");
        fprintf(stream, "  --profile file    Enable profiling (output to file)\n");
        fprintf(stream, "\nExamples:\n");
        fprintf(stream, "  %s examples/hello.b\n", argv[0]);
        fprintf(stream, "  %s --debug examples/fizzbuzz.b\n", argv[0]);
        fprintf(stream, "  %s --no-optimize examples/mandelbrot.b\n", argv[0]);
        fprintf(stream, "  %s --profile profile.txt examples/mandelbrot.b\n", argv[0]);
        return show_help ? 0 : 1;
    }

    size_t program_size;
    char *program = read_file(argv[arg_offset], &program_size);

    bf_func compiled_program;
    ast_node_t *ast = NULL;

    ast = parse_bf_program(program);

    if (optimize) {
        ast = ast_rewrite_sequences(ast);
        ast = ast_optimize(ast);
    }

    if (debug_mode) {
        fprintf(stderr, "%s AST dump:\n", optimize ? "Optimized" : "Unoptimized");
        ast_print(ast, 0);
    }

    bf_debug_info_t debug_info;
    bf_debug_info_t *debug_ptr = NULL;
    if (profile_mode) {
        debug_ptr = &debug_info;
        if (bf_debug_init(debug_ptr, NULL, 0) != 0) {
            bf_error("Failed to initialize debug info");
        }
    }

    void *code_ptr = NULL;
    size_t code_size = 0;
    compiled_program = compile_bf_ast(ast, debug_mode, &code_ptr, &code_size, debug_ptr);

    if (debug_ptr) {
        debug_ptr->code_start = code_ptr;
        debug_ptr->code_size = code_size;
    }

    bf_profiler_t profiler;
    if (profile_mode) {
        if (bf_prof_init(&profiler, code_ptr, code_size, debug_ptr, ast) != 0) {
            bf_error("Failed to initialize profiler");
        }
        bf_prof_start(&profiler);
    }

    char *memory = calloc(BF_MEMORY_SIZE, 1);
    if (!memory) {
        bf_error("Memory allocation failed");
    }

    compiled_program(memory);

    if (profile_mode) {
        bf_prof_stop(&profiler);

        FILE *prof_out = fopen(profile_output, "w");
        if (!prof_out) {
            fprintf(stderr, "Error: Could not open profile output file '%s'\n", profile_output);
            bf_prof_cleanup(&profiler);
            if (debug_ptr) bf_debug_cleanup(debug_ptr);
            free(program);
            free(memory);
            if (ast) ast_free(ast);
            return 1;
        }

        bf_prof_dump_with_debug(&profiler, prof_out, debug_ptr);
        bf_prof_print_heat_ast(&profiler, debug_ptr, ast, prof_out);

        fclose(prof_out);
        fprintf(stderr, "Profile data written to: %s\n", profile_output);

        bf_prof_cleanup(&profiler);
    }

    if (debug_ptr) {
        bf_debug_cleanup(debug_ptr);
    }

    free(program);
    free(memory);
    if (ast) {
        ast_free(ast);
    }

    return 0;
}
