#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint32_t lvl_msk; // bitmask variable
    int fd;
} llt_L;

void llt_init();
void llt_log_set_file(FILE *file);
void llt_log_set_fd(int fd);

// char *__llt_log(int lvl_msk, char const *format, va_list args);
size_t __llt_log(int lvl_msk, char **buf, char const *format, va_list args);
void llt_log(int lvl_msk, char const * format, ...);

#endif
