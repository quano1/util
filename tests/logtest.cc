#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <future>

#include "../libs/SimpleSignal.hpp"
#include "../libs/utils.h"
#include "../libs/logger.h"
// #include "../libs/exporterudp.h"

template <typename L>
struct A
{
    A(std::function<void(int, std::string const&)> logf)
    {
        sig_log.connect(logf);
        // sig_log.connect(log, L::*method);
        // sig_log.connect(Simple::slot (logger, &L::log));
        sig_log.emit(static_cast<int>(tll::LogType::kInfo), utils::stringFormat("%s\n", _LOG_HEADER));
    }

    ~A()
    {
        // logger_.log(tll::LogType::debug, utils::stringFormat("{%.6f}{%s}{%s}{%.6f(s)}\n", utils::timestamp(), utils::tid(), name, elapse()));
        sig_log.emit(static_cast<int>(tll::LogType::kInfo), utils::stringFormat("%s\n", _LOG_HEADER));
    }

    // L &logger_;
    // L sig_log_;
    Simple::Signal<void(int, std::string const&)> sig_log;
};

namespace {
    int const console = 1;
}

int main(int argc, char const *argv[])
{
    using Logger = tll::Logger<0x400, 0x1000, 10>;
    Logger lg
        (
            tll::LogFd{tll::lf_t | tll::lf_i, console},
            tll::LogFd{tll::lf_d | tll::lf_i | tll::lf_f | tll::lf_t, open("fd_t.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
            // ,tll::LogFd{tll::toFlag(tll::LogType::kInfo, tll::LogType::kDebug), open("fd_i.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
            // ,tll::LogFd{tll::toFlag(tll::LogType::kFatal), open("fd_f.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
        );
    // A<Logger > a(lg);
    A<Logger> a([&lg](int type, std::string const &log_msg){lg.log(type, "%s", log_msg);});

    TLL_LOGTF(lg);
    // if(argc > 1)
    {
        TLL_LOGT(lg, single);
        {
            TLL_LOGT(lg, single_inner);
            // #pragma omp parallel for
            for(int i=0; i < 100000; i++)
            {
                TLL_LOGD(lg, "%d %s", 10, "oi troi oi");
                TLL_LOGF(lg, "%d %s", 10, "oi troi oi");
            }
        }
        lg.join();
    }
    LOGD("%d", lg.write_count_);
    
    lg.write_count_=0;
    lg.batch_mode_=false;
    {
        TLL_LOGT(lg, single);
        {
            TLL_LOGT(lg, single_inner);
            // #pragma omp parallel for
            for(int i=0; i < 100000; i++)
            {
                TLL_LOGD(lg, "%d %s", 10, "oi troi oi");
                TLL_LOGF(lg, "%d %s", 10, "oi troi oi");
            }
        }
        lg.join();
    }
    LOGD("%d", lg.write_count_);

    // {
    //     TLL_LOGT(lg, multi);
    //     {
    //         TLL_LOGT(lg, multi_inner);
    //         // #pragma omp parallel for
    //         std::future<void> futs[2];
    //         for(auto &fut : futs)
    //             fut = std::async(std::launch::async, [&lg](){
    //                 for(int i=0; i < (1000 / 2); i++)
    //                 {
    //                     TLL_LOGD(lg, "%d %s", 10, "oi troi oi");
    //                     TLL_LOGF(lg, "%d %s", 10, "oi troi oi");
    //                 }
    //             });

    //         for(auto &fut : futs) fut.get();
    //     }
    //     lg.join();
    // }

    return 0;
}

