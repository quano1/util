#include "../log.hpp"
#include "../Counter.hpp"

int main()
{
	LOGP(MAIN);

	// Log l;
	// l.log(1, "hello world oi troi oi\n");
	// llt_log(l, 1, "hello world oi troi oi\n");

	// LogImpl::instance().add_fd(1);
	// Log::ins().logi("hello\n");

	// Log *l = Log::ins();
	// llt_log(Log::ins(), 0, "%d hello\n", __LINE__);

	LOGI("hello");
	CLOGI("hello");
	LOGD("hello");
	LOGW("hello");
	LOGF("hello");

	return 0;
}

