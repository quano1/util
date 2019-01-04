
#include <stdio.h>

struct LogMngr;
struct Exporter;

void logi_async(struct LogMngr *aLogger, const char *fmt, ...);
void log_init(struct LogMngr *aLogger);
void log_deinit(struct LogMngr *aLogger);

struct LogMngr *lp;

int main()
{
	log_init(&lp);
	logi_async(lp, "%s %s %d ", __FILE__, __FUNCTION__, __LINE__);
	log_deinit(lp);
	return 0;
}
