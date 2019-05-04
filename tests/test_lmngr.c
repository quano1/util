
#include <stdio.h>
#include <log.h>

struct LogMngr *lp;

const int BUF_SIZE = 0x100;

int main(int argc, char **argv)
{
	// struct Tracer *tr;
	char buffer[BUF_SIZE];
	snprintf(buffer, BUF_SIZE, "%s.log", argv[0]);
	LOG_INIT(lp, 1, true, buffer);
	// start_trace(lp, &tr, "MAIN");
	TRACES(lp, MAIN);

	LOGI(lp, "Test log Info");
	TRACES(lp, MAIN2);
	LOGW(lp, "Test log Warning");
	TRACEE(MAIN2);
	LOGE(lp, "Test log Error");

	TRACEE(MAIN);

	LOG_DEINIT(lp);
	return 0;
}
