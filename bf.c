#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "dasm_proto.h"

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
    int arg_offset = 1;

    if (argc >= 2 && strcmp(argv[1], "-d") == 0) {
        debug_mode = 1;
        arg_offset = 2;
    }

    if (argc < arg_offset + 1) {
        fprintf(stderr, "Usage: %s [-d] <brainfuck_file>\n", argv[0]);
        fprintf(stderr, "  -d: Enable debug mode (dump compiled code)\n");
        return 1;
    }

    size_t program_size;
    char *program = read_file(argv[arg_offset], &program_size);

    char *memory = calloc(BF_MEMORY_SIZE, 1);
    if (!memory) {
        bf_error("Memory allocation failed");
    }

    bf_func compiled_program = compile_bf(program, debug_mode);
    compiled_program(memory);

    free(program);
    free(memory);

    return 0;
}