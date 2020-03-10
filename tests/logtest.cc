#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */

#include "../libs/SimpleSignal.hpp"
#include "../libs/utils.h"
#include "../libs/logger.h"
// #include "../libs/exporterudp.h"



namespace {
    int const fd_terminal = 0;
}

int main(int argc, char const *argv[])
{
    tll::Logger<0x400, 0x1000, 5> lg
        (
            tll::LogFd{tll::logtype::kDebug | tll::logtype::kInfo | tll::logtype::kFatal, fd_terminal},
            tll::LogFd{tll::logtype::kTrace | tll::logtype::kDebug, open("fd_t.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)},
            tll::LogFd{tll::logtype::kInfo, open("fd_i.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)},
            tll::LogFd{tll::logtype::kFatal, open("fd_f.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
        );

    TLL_LOGTF(lg);
    if(argc == 2)
    {
        TIMER(logger);

        for(int i=0; i<std::stoi(argv[1]); i++)
        {
            TLL_LOGD(lg, "%d %s", 10, "oi troi oi");
            TLL_LOGT(lg, loop);
            TLL_LOGI(lg, "%d %s", 10, "oi troi oi");
            TLL_LOGF(lg, "%d %s", 10, "oi troi oi");
        }
        lg.join();
    }
    else
    {
        TIMER(rawlog);
        for(int i=0; i<std::stoi(argv[1]); i++)
            printf("[%d]%ld:%s:%s:%d[%s](%d %s)\n", (int)tll::logtype::kInfo, utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid().data(), 10, "oi troi oi");
    }

    return 0;
}

