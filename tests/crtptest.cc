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
	int fdraw1 = open("crtptest1.log", O_WRONLY | O_CREAT , 0644);
	int fdraw2 = open("crtptest2.log", O_WRONLY | O_CREAT , 0644);
	int fdraw3 = open("crtptest3.log", O_WRONLY | O_CREAT , 0644);

	// LConsole console;
	// console.onInit();
	// console.onLog(LogInfo{});
	// std::string str = utils::Format("%d %s\n",10, "oi troi oi");
	// write(0, str.data(), str.size());

	if(argc == 2)
	{
		TIMER(logger);
		Logger<0x400, 0x1000, 0> lg;
		lg.add(1);
		lg.add(fdraw1);
		for(int i=0; i<std::stoi(argv[1]); i++)
			lg.log("[%d]%ld:%s:%s:%d[%s](%d %s)\n", (int)LogType::kInfo, utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid(), 10, "oi troi oi");
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

