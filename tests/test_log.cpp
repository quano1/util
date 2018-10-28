#include "../log.hpp"

int main()
{
	// Log l;
	// l.log(1, "hello world oi troi oi\n");
	// llt_log(l, 1, "hello world oi troi oi\n");

	// LogImpl::instance().add_fd(1);
	// Log::ins().logi("hello\n");

	// Log *l = Log::ins();
	// llt_log(Log::ins(), 0, "%d hello\n", __LINE__);

	LOGI("hello");
	CLOGI("hello");
	// llt_log_global(static_cast<int>(LogType::Info), "%d %d %d hello\n", 1, 2, 3);

	LOGD("hello");
	LOGW("hello");
	LOGF("hello");

	return 0;
}

