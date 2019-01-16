#pragma once

struct LogMngr;
struct Exporter;
struct Tracer;

void log_async(struct LogMngr *aLogger, int aLogType, const char *fmt, ...);
void log_init(struct LogMngr **aLogger);
void log_deinit(struct LogMngr *aLogger);

void start_trace(struct LogMngr *aLogger, struct Tracer **aTracer, char const *aBuf);
void stop_trace(struct Tracer *aTracer);

#define LOGD(format, ...) printf("[Dbg] %s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define TRACES(logger, FUNC) struct Tracer *__##FUNC; log_async(logger, 3, #FUNC " %s %d", __FUNCTION__, __LINE__); start_trace(logger, &__##FUNC, #FUNC)
#define TRACEE(FUNC) stop_trace(__##FUNC)

#define LOGI(logger, fmt, ...) log_async(logger, 0, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(logger, fmt, ...) log_async(logger, 1, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(logger, fmt, ...) log_async(logger, 2, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

