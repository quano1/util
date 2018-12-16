#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>

// typedef struct {
//     uint32_t lvl_msk; // bitmask variable
//     int fd;
// } llt_L;

// void llt_init();
// void llt_log_set_file(FILE *file);
// void llt_log_set_fd(int fd);

// char *__llt_log(int lvl_msk, char const *format, va_list args);
// size_t __llt_log(int lvl_msk, char **buf, char const *format, va_list args);
void llt_log(int lvl_msk, char const * format, ...);

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOGD(format, ...) printf("[Dbg] %s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define CLOGI(format, ...) llt_clog(static_cast<int>(LogType::Info), true, "%s %s %d " format "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
// #define CLOGD(format, ...) llt_clog(static_cast<int>(LogType::Debug), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define CLOGW(format, ...) llt_clog(static_cast<int>(LogType::Warn), true, "%s %s %d " format "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define CLOGF(format, ...) llt_clog(static_cast<int>(LogType::Fatal), true, "%s %s %d " format "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

#endif
