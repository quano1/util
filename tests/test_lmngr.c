
#include <stdio.h>
#include <log.h>

struct LogMngr *lp;

int main()
{
	// struct Tracer *tr;
	log_init(&lp);
	// start_trace(lp, &tr, "MAIN");
	TRACES(lp, MAIN);

	LOGI(lp, "");
	TRACES(lp, MAIN2);
	LOGW(lp, "");
	TRACEE(MAIN2);
	LOGE(lp, "");

	TRACEE(MAIN);

	log_deinit(lp);
	return 0;
}
