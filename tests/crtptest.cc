#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */

#include "../libs/SimpleSignal.hpp"
#include "../libs/utils.h"
#include "../libs/exporter.h"
// #include "../libs/exporterudp.h"

#define LOG_HEADER utils::Format("(%.6f)%s:%s:%d[%s]", utils::timestamp<double>(), __FILE__, __FUNCTION__, __LINE__, utils::tid())

#define TLL_LOGD(logger, format, ...) (logger).log<tll::LogType::D>("[D]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

#define TLL_LOGTF(logger) (logger).log<tll::LogType::T>("[T]%s", LOG_HEADER); utils::Timer timer__([&logger](std::string const &str){logger.log<tll::LogType::T>("%s", str.data());}, __FUNCTION__)

#define TLL_LOGT(logger, ID) (logger).log<tll::LogType::T>("[T]%s", LOG_HEADER); utils::Timer timer_##ID__([&logger](std::string const &str){logger.log<tll::LogType::T>("%s", str.data());}, #ID)

#define TLL_LOGI(logger, format, ...) (logger).log<tll::LogType::I>("[I]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

#define TLL_LOGF(logger, format, ...) (logger).log<tll::LogType::F>("[F]%s(" format ")\n", LOG_HEADER , ##__VA_ARGS__)

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
    
    tll::Logger<0x400, 0x1000, 5> lg;
    lg.addFd(
            tll::LogFd{tll::LogType::D | tll::LogType::I | tll::LogType::F, 0},
            tll::LogFd{tll::LogType::T | tll::LogType::D, open("fd_t.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)},
            tll::LogFd{tll::LogType::I, open("fd_i.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)},
            tll::LogFd{tll::LogType::F, open("fd_f.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
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
            printf("[%d]%ld:%s:%s:%d[%s](%d %s)\n", (int)tll::LogType::I, utils::timestamp(), __FILE__, __FUNCTION__, __LINE__, utils::tid().data(), 10, "oi troi oi");
    }

    return 0;
}

