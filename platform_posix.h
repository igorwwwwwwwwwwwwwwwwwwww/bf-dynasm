#ifndef PLATFORM_POSIX_H
#define PLATFORM_POSIX_H

#include <stddef.h>

char *platform_posix_read_file(const char *filename, size_t *size);
char *platform_posix_allocate_guarded_memory(size_t size);
void platform_posix_free_guarded_memory(char *memory, size_t size);
double platform_posix_time_ms(void);
void platform_posix_print_phase_time(const char *phase, double start_ms, double end_ms);

#endif
