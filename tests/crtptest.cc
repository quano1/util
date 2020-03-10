#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */

#include "../libs/SimpleSignal.hpp"
#include "../libs/utils.h"
#include "../libs/exporter.h"
// #include "../libs/exporterudp.h"



namespace {
    int const fd = 0;
}

int main(int argc, char const *argv[])
{
    // using namespace tll;
    // int fd_t = open("fd_t.log", O_WRONLY | O_TRUNC | O_CREAT , 0644);
    // LOGD("%d", fd_t);
    // int fd_i = open("fd_i.log", O_WRONLY | O_TRUNC | O_CREAT , 0644);
    // LOGD("%d", fd_i);
    // int fd_f = open("fd_f.log", O_WRONLY | O_TRUNC | O_CREAT , 0644);
    // LOGD("%d", fd_f);

    tll::Logger<0x400, 0x1000, 5> lg
    (
            tll::LogFd{tll::logtype::kDebug | tll::logtype::kInfo | tll::logtype::kFatal, 0},
            tll::LogFd{tll::logtype::kTrace | tll::logtype::kDebug, open("fd_t.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)},
            tll::LogFd{tll::logtype::kInfo, open("fd_i.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)},
            tll::LogFd{tll::logtype::kFatal, open("fd_f.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
            );

    TLL_LOGTF(lg);
    if(argc == 2)
    {
        TIMER(logger);
        // auto lf = LogFlag{.flag=0xFU};
        // LOGD("%d %d %d %d", lf.debug, lf.trace, lf.info, lf.fatal);

        // lg.addFd(tll::LogFd{0, LogFlag{.debug=1}}
        //          ,tll::LogFd{fd_t, {{.debug=0,.trace=1,.info=1,.fatal=1,}}}
        //          ,tll::LogFd{fd_i, {{.debug=0,.trace=1,.info=1,}}}
        //          ,tll::LogFd{fd_f, LogFlag{.flag=0xFU}}
        //          );

        // lg.start();

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

