#include "../log.hpp"

int main()
{
	LOGI("hello");
	CLOGI("hello");
	// llt_log_global(static_cast<int>(LogType::Info), "%d %d %d hello\n", 1, 2, 3);

	LOGD("hello");
	LOGW("hello");
	LOGF("hello");

	return 0;
}

