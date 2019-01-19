
#include <stdio.h>
#include <log.h>

struct LogMngr *lp;

int main()
{
	// struct Tracer *tr;
	LOG_INIT(lp, 1, true);
	// start_trace(lp, &tr, "MAIN");
	TRACES(lp, MAIN);

	LOGI(lp, "");
	TRACES(lp, MAIN2);
	LOGW(lp, "");
	TRACEE(MAIN2);
	LOGE(lp, "");

	TRACEE(MAIN);

	LOG_DEINIT(lp);
	return 0;
}
