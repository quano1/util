
#include <stdio.h>

struct LogMngr;
struct Exporter;
struct Tracer;

void log_async(struct LogMngr *aLogger, int aLogType, const char *fmt, ...);
void log_init(struct LogMngr *aLogger);
void log_deinit(struct LogMngr *aLogger);

void start_trace(struct LogMngr *aLogger, struct Tracer **aTracer, char const *aBuf);
void stop_trace(struct Tracer *aTracer);

#define LOGD(format, ...) printf("[Dbg] %s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define TRACE_S(logger, FUNC) struct Tracer *__##FUNC; log_async(logger, 3, #FUNC " %s %d", __FUNCTION__, __LINE__); start_trace(logger, &__##FUNC, #FUNC)
#define TRACE_E(FUNC) stop_trace(__##FUNC)

#define LOGI(logger, fmt, ...) log_async(logger, 0, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(logger, fmt, ...) log_async(logger, 1, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(logger, fmt, ...) log_async(logger, 2, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

struct LogMngr *lp;


int main()
{
	// struct Tracer *tr;
	log_init(&lp);
	// start_trace(lp, &tr, "MAIN");
	TRACE_S(lp, MAIN);

	LOGI(lp, "");
	TRACE_S(lp, MAIN2);
	LOGW(lp, "");
	TRACE_E(MAIN2);
	LOGE(lp, "");

	TRACE_E(MAIN);

	log_deinit(lp);
	return 0;
}
