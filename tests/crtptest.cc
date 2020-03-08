#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */

#include "../libs/utils.h"
#include "../libs/exporter.h"
// #include "../libs/exporterudp.h"

#define LOG_HEADER utils::Format("%.6f:%s:%s:%d[%s]", utils::timestamp<double>(), __FILE__, __FUNCTION__, __LINE__, utils::tid())

#define TLL_LOGD(logger, format, ...) (logger).log<LogType::D>("[D]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

#define TLL_LOGT2(logger) (logger).log<LogType::T>("[T]%s\n", LOG_HEADER ); utils::Timer __tracer()

#define TLL_LOGT(logger) (logger).log<LogType::T>("[T]%s\n", LOG_HEADER )

#define TLL_LOGI(logger, format, ...) (logger).log<LogType::I>("[I]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

#define TLL_LOGF(logger, format, ...) (logger).log<LogType::F>("[F]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

namespace {
    int const fd = 0;
}

int main(int argc, char const *argv[])
{
    using namespace tll;
    int fd_t = open("fd_t.log", O_WRONLY | O_TRUNC | O_CREAT , 0644);
    LOGD("%d", fd_t);
    int fd_i = open("fd_i.log", O_WRONLY | O_TRUNC | O_CREAT , 0644);
    LOGD("%d", fd_i);
    int fd_f = open("fd_f.log", O_WRONLY | O_TRUNC | O_CREAT , 0644);
    LOGD("%d", fd_f);

    if(argc == 2)
    {
        TIMER(logger);
        Logger<0x400, 0x1000, 5> lg;

        // auto lf = LogFlag{.flag=0xFU};
        // LOGD("%d %d %d %d", lf.debug, lf.trace, lf.info, lf.fatal);

        // lg.addFd(LogFd{0, LogFlag{.debug=1}}
        //          ,LogFd{fd_t, {{.debug=0,.trace=1,.info=1,.fatal=1,}}}
        //          ,LogFd{fd_i, {{.debug=0,.trace=1,.info=1,}}}
        //          ,LogFd{fd_f, LogFlag{.flag=0xFU}}
        //          );

        lg.addFd(
                 // LogFd{LogType::D | LogType::T | LogType::I | LogType::F, 0},
                LogFd{LogType::T | LogType::D, fd_t},
                LogFd{LogType::I, fd_i},
                LogFd{LogType::F, fd_f}
                );
        // lg.start();

        for(int i=0; i<std::stoi(argv[1]); i++)
        {
            TLL_LOGD(lg, "%d %s", 10, "oi troi oi");
            TLL_LOGT(lg);
            TLL_LOGI(lg, "%d %s", 10, "oi troi oi");
            TLL_LOGF(lg, "%d %s", 10, "oi troi oi");
        }

        lg.join();
    }
    else
    {
        TIMER(rawlog);
        for(int i=0; i<std::stoi(argv[1]); i++)
            printf("[%d]%ld:%s:%s:%d[%s](%d %s)\n", (int)LogType::I, utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid().data(), 10, "oi troi oi");
    }

    return 0;
}

