#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

#include "dasm_proto.h"
#include "bf_ast.h"
#include "bf_prof.h"
#include "bf_debug.h"

#include "bf_parser.h"

#define BF_DEFAULT_MEMORY_SIZE 65536  // 64KB - nice power of 2
#define MAX_NESTING 1000

// Global flag to control unsafe mode (accessible by DynASM templates)
static bool g_unsafe_mode = false;

// High-resolution timing helpers
static double get_time_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void print_phase_time(const char *phase, double start_ms, double end_ms) {
    fprintf(stderr, "%-20s: %8.3f ms\n", phase, end_ms - start_ms);
}

// Memory allocation with guard pages
static char* allocate_guarded_memory(size_t size) {
    // Get page size for alignment
    size_t page_size = getpagesize();

    // Round up size to page boundary
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);

    // Allocate 3 regions: guard page + data + guard page
    size_t total_size = page_size + aligned_size + page_size;

    void *region = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        return NULL;
    }

    char *guard1 = (char*)region;
    char *data = guard1 + page_size;
    char *guard2 = data + aligned_size;

    // Make guard pages inaccessible (no read/write/execute)
    if (mprotect(guard1, page_size, PROT_NONE) != 0 ||
        mprotect(guard2, page_size, PROT_NONE) != 0) {
        munmap(region, total_size);
        return NULL;
    }

    return data;
}

static void free_guarded_memory(char *memory, size_t size) {
    if (!memory) return;

    size_t page_size = getpagesize();
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);
    size_t total_size = page_size + aligned_size + page_size;

    // Find start of the full region (guard page before data)
    char *region_start = memory - page_size;
    munmap(region_start, total_size);
}

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

static bf_func compile_bf_ast(ast_node_t *ast, bool debug_mode, bool unsafe_mode, void **code_ptr, size_t *code_size, bf_debug_info_t *debug_info, size_t memory_size) {
    g_unsafe_mode = unsafe_mode;  // Set global flag for DynASM templates
    
    dasm_State *state = NULL;
    dasm_State **Dst = &state;
    dasm_init(Dst, 1);
    dasm_setup(Dst, actions);

    int debug_label_count = debug_info ? ast_count_nodes(ast) : 0;
    dasm_growpc(Dst, MAX_NESTING * 2 + debug_label_count);

    compile_bf_prologue(Dst, memory_size);

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
    bool timing_mode = false;
    bool unsafe_mode = false;  // Disable memory safety for performance
    const char *profile_output = NULL;
    size_t memory_size = BF_DEFAULT_MEMORY_SIZE;
    size_t memory_offset = 4096;  // Default 4KB offset for negative access
    int arg_offset = 1;

    for (int i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
            arg_offset++;
        } else if (strcmp(argv[i], "--timing") == 0) {
            timing_mode = true;
            arg_offset++;
        } else if (strcmp(argv[i], "--no-optimize") == 0) {
            optimize = false;
            arg_offset++;
        } else if (strcmp(argv[i], "--unsafe") == 0) {
            unsafe_mode = true;
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
        } else if (strcmp(argv[i], "--memory") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --memory requires a size in bytes\n");
                return 1;
            }
            char *endptr;
            memory_size = strtoul(argv[i + 1], &endptr, 10);
            if (*endptr != '\0' || memory_size == 0) {
                fprintf(stderr, "Error: Invalid memory size '%s'\n", argv[i + 1]);
                return 1;
            }
            i++;
            arg_offset += 2;
        } else if (strcmp(argv[i], "--memory-offset") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --memory-offset requires a size in bytes\n");
                return 1;
            }
            char *endptr;
            memory_offset = strtoul(argv[i + 1], &endptr, 10);
            if (*endptr != '\0') {
                fprintf(stderr, "Error: Invalid memory offset '%s'\n", argv[i + 1]);
                return 1;
            }
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
        fprintf(stream, "  --timing          Show execution phase timing\n");
        fprintf(stream, "  --no-optimize     Disable AST optimizations\n");
        fprintf(stream, "  --unsafe          Disable memory safety checks for performance\n");
        fprintf(stream, "  --profile file    Enable profiling (folded stack format)\n");
        fprintf(stream, "  --memory size     Set memory size in bytes (default: %zu)\n", (size_t)BF_DEFAULT_MEMORY_SIZE);
        fprintf(stream, "  --memory-offset n Set initial pointer offset in bytes (default: 4096)\n");
        fprintf(stream, "\nExamples:\n");
        fprintf(stream, "  %s examples/hello.b\n", argv[0]);
        fprintf(stream, "  %s --debug examples/fizzbuzz.b\n", argv[0]);
        fprintf(stream, "  %s --no-optimize examples/mandelbrot.b\n", argv[0]);
        fprintf(stream, "  %s --profile profile.txt examples/mandelbrot.b\n", argv[0]);
        fprintf(stream, "  %s --memory 32768 examples/hello.b\n", argv[0]);
        fprintf(stream, "  %s --memory-offset 8192 examples/program.b\n", argv[0]);
        return show_help ? 0 : 1;
    }

    // Validate memory offset
    if (memory_offset >= memory_size) {
        fprintf(stderr, "Error: Memory offset (%zu) must be less than memory size (%zu)\n",
                memory_offset, memory_size);
        return 1;
    }

    // Start timing
    double total_start = timing_mode ? get_time_ms() : 0.0;
    double phase_start = total_start;

    size_t program_size;
    char *program = read_file(argv[arg_offset], &program_size);

    if (timing_mode) {
        double phase_end = get_time_ms();
        print_phase_time("File I/O", phase_start, phase_end);
        phase_start = phase_end;
    }

    bf_func compiled_program;
    ast_node_t *ast = NULL;

    ast = parse_bf_program(program);

    if (timing_mode) {
        double phase_end = get_time_ms();
        print_phase_time("Parsing", phase_start, phase_end);
        phase_start = phase_end;
    }

    if (optimize) {
        ast = ast_rewrite_sequences(ast);
        ast = ast_optimize(ast);

        if (timing_mode) {
            double phase_end = get_time_ms();
            print_phase_time("AST Optimization", phase_start, phase_end);
            phase_start = phase_end;
        }
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
    // Adjust memory size for JIT compilation to account for offset
    size_t effective_memory_size = memory_size - memory_offset;
    compiled_program = compile_bf_ast(ast, debug_mode, unsafe_mode, &code_ptr, &code_size, debug_ptr, effective_memory_size);

    if (timing_mode) {
        double phase_end = get_time_ms();
        print_phase_time("JIT Compilation", phase_start, phase_end);
        phase_start = phase_end;
    }

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

    char *memory = allocate_guarded_memory(memory_size);
    if (!memory) {
        bf_error("Memory allocation failed");
    }

    if (timing_mode) {
        double phase_end = get_time_ms();
        print_phase_time("Memory Allocation", phase_start, phase_end);
        phase_start = phase_end;
    }

    compiled_program(memory + memory_offset);

    if (timing_mode) {
        double phase_end = get_time_ms();
        print_phase_time("Program Execution", phase_start, phase_end);
        phase_start = phase_end;
    }

    if (profile_mode) {
        bf_prof_stop(&profiler);

        FILE *prof_out = fopen(profile_output, "w");
        if (!prof_out) {
            fprintf(stderr, "Error: Could not open profile output file '%s'\n", profile_output);
            bf_prof_cleanup(&profiler);
            if (debug_ptr) bf_debug_cleanup(debug_ptr);
            free(program);
            free_guarded_memory(memory, memory_size);
            if (ast) ast_free(ast);
            return 1;
        }

        bf_prof_dump_folded(&profiler, prof_out);

        fclose(prof_out);
        fprintf(stderr, "Profile data written to: %s\n", profile_output);

        bf_prof_cleanup(&profiler);
    }

    if (debug_ptr) {
        bf_debug_cleanup(debug_ptr);
    }

    free(program);
    free_guarded_memory(memory, memory_size);
    if (timing_mode) {
        double total_end = get_time_ms();
        fprintf(stderr, "%-20s  --------\n", "");
        print_phase_time("Total Time", total_start, total_end);
    }

    if (ast) {
        ast_free(ast);
    }

    return 0;
}
