#pragma once

#include <stdbool.h>

enum LogType;
struct LogMngr;
struct Exporter;
struct Tracer;

void llt_log(struct LogMngr *aLogger, int aLogType, const char *fmt, ...);
void llt_set_async(struct LogMngr *aLogger, bool aAsync);
// void llt_log_async(struct LogMngr *aLogger, int aLogType, const char *fmt, ...);
void llt_log_init(struct LogMngr **aLogger, size_t aWorkerNum, const char *aFileName);
void llt_log_deinit(struct LogMngr *aLogger);

void llt_start_trace(struct LogMngr *aLogger, struct Tracer **aTracer, char const *aBuf);
void llt_stop_trace(struct Tracer *aTracer);


#undef LOG_INIT
#undef LOG_DEINIT
#undef LOGD
#undef TRACE
#undef LOGI
#undef LOGI_A
#undef LOGW
#undef LOGW_A
#undef LOGE
#undef LOGE_A

#define LOG_INIT(logger, workerNum, async, fileName) llt_log_init(&logger, workerNum, fileName); llt_set_async(logger, async)
#define LOG_DEINIT(logger) llt_log_deinit(logger)

#define LOGD(format, ...) printf("[Dbg] %s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define TRACES(logger, FUNC) struct Tracer *__##FUNC; llt_log(logger, 3, __FILE__, __FUNCTION__, __LINE__, #FUNC); llt_start_trace(logger, &__##FUNC, #FUNC)
#define TRACEE(FUNC) llt_stop_trace(__##FUNC)

#define LOGI(logger, fmt, ...) llt_log(logger, 0, __FILE__, __FUNCTION__, __LINE__, fmt "", ##__VA_ARGS__)
#define LOGW(logger, fmt, ...) llt_log(logger, 1, __FILE__, __FUNCTION__, __LINE__, fmt "", ##__VA_ARGS__)
#define LOGE(logger, fmt, ...) llt_log(logger, 2, __FILE__, __FUNCTION__, __LINE__, fmt "", ##__VA_ARGS__)
