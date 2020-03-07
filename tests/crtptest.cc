#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */

#include "../libs/utils.h"
#include "../libs/exporter.h"
// #include "../libs/exporterudp.h"

#define LOG_HEADER utils::Format("%ld:%s:%s:%d[%s]", utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid())

#define LLT_LOGT(logger, format, ...) (logger).log<LogType::kTrace>("[T]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

#define LLT_LOGI(logger, format, ...) (logger).log<LogType::kInfo>("[I]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)


#define LLT_LOGF(logger, format, ...) (logger).log<LogType::kFatal>("[F]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

#define LLT_LOGD(logger, format, ...) (logger).log<LogType::kDebug>("[D]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

namespace {
	int const fd = 0;
}

int main(int argc, char const *argv[])
{
	using namespace llt;
	int fdraw1 = open("crtptest1.log", O_WRONLY | O_TRUNC /*| O_CREAT*/ , 0644);
	int fdraw2 = open("crtptest2.log", O_WRONLY | O_TRUNC /*| O_CREAT*/ , 0644);
	int fdraw3 = open("crtptest3.log", O_WRONLY | O_TRUNC /*| O_CREAT*/ , 0644);

	// LConsole console;
	// console.onInit();
	// console.onLog(LogInfo{});
	// std::string str = utils::Format("%d %s\n",10, "oi troi oi");
	// write(0, str.data(), str.size());

	// LOGD("%ld", sizeof(LogFlag));
	// LogFlag lt; lt.value = 0;
	// lt.fatal = 1;
	// LOGD("%x", lt.value);
	// LOGD("%x", lt.trace);
	// LOGD("%x", lt.info);
	// LOGD("%x", lt.fatal);
	// LOGD("%x", lt.debug);
	// LOGD("%x", lt.reserve);

	// return 0;

	if(argc == 2)
	{
		TIMER(logger);
		Logger<0x400, 0x1000, 0> lg;
		lg.addFd({1, LogFlag{.debug=(1U)}});
		lg.addFd({fdraw1, LogFlag{.trace=(1U)}});
		lg.addFd({fdraw2, LogFlag{.info=(1U)}});
		lg.addFd({fdraw3, LogFlag{.fatal=(1U)}});
		lg.start();
		for(int i=0; i<std::stoi(argv[1]); i++)
		{
			LLT_LOGT(lg, "%d %s", 10, "oi troi oi");
			LLT_LOGI(lg, "%d %s", 10, "oi troi oi");
			LLT_LOGF(lg, "%d %s", 10, "oi troi oi");
			LLT_LOGD(lg, "%d %s", 10, "oi troi oi");
		}

		lg.join();
	}
	else
	{
		TIMER(rawlog);
		for(int i=0; i<std::stoi(argv[1]); i++)
			printf("[%d]%ld:%s:%s:%d[%s](%d %s)\n", (int)LogType::kInfo, utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid().data(), 10, "oi troi oi");
	}

	return 0;
}

