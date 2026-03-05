#include "platform_posix.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

char *platform_posix_read_file(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(*size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(content, 1, *size, file);
    fclose(file);
    if (bytes_read != *size) {
        free(content);
        return NULL;
    }

    content[*size] = '\0';
    return content;
}

char *platform_posix_allocate_guarded_memory(size_t size) {
    size_t page_size = (size_t)getpagesize();
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);
    size_t total_size = page_size + aligned_size + page_size;

    void *region = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        return NULL;
    }

    char *guard1 = (char *)region;
    char *data = guard1 + page_size;
    char *guard2 = data + aligned_size;

    if (mprotect(guard1, page_size, PROT_NONE) != 0 ||
        mprotect(guard2, page_size, PROT_NONE) != 0) {
        munmap(region, total_size);
        return NULL;
    }

    return data;
}

void platform_posix_free_guarded_memory(char *memory, size_t size) {
    if (!memory) return;

    size_t page_size = (size_t)getpagesize();
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);
    size_t total_size = page_size + aligned_size + page_size;

    char *region_start = memory - page_size;
    munmap(region_start, total_size);
}

double platform_posix_time_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void platform_posix_print_phase_time(const char *phase, double start_ms, double end_ms) {
    fprintf(stderr, "%-20s: %8.3f ms\n", phase, end_ms - start_ms);
}
