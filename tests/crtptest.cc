#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */

#include "../libs/utils.h"
#include "../libs/exporter.h"
// #include "../libs/exporterudp.h"

namespace {
	int const fd = 0;
}

int main(int argc, char const *argv[])
{
	using namespace llt;
	int fdraw = open("crtptest1.log", O_WRONLY | O_CREAT);

	// LConsole console;
	// console.onInit();
	// console.onLog(LogInfo{});
	// std::string str = utils::Format("%d %s\n",10, "oi troi oi");
	// write(0, str.data(), str.size());

	Logger<10> lg;
	// LOGD("");
	lg.add(0);
	// LOGD("");
	lg.log<LogType::kInfo>("%ld:%s:%s:%d[%s](%d %s)\n", utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid(), 10, "oi troi oi");
	// LOGD("");
	lg.rem(0);
	// LOGD("");
	lg.log<LogType::kInfo>("%ld:%s:%s:%d[%s](%d %s)\n", utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid(), 10, "oi troi oi");

	// LOGD("");
	lg.join();
	// LOGD("");
	// lg.log<LogType::kInfo>(fdraw, "%ld:%s:%s:%d[%s](%d %s)\n", utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid(), 10, "oi troi oi");

	// lg.log<LogType::kInfo>(fd, fdraw, "%ld:%s:%s:%d[%s](%d %s)\n", utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid(), 10, "HEHEHE");

	return 0;
}

