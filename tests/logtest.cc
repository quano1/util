#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <future>

#include "../libs/SimpleSignal.hpp"
#include "../libs/utils.h"
#include "../libs/logger.h"
// #include "../libs/exporterudp.h"

namespace {
    int const console = 1;
}

int main(int argc, char const *argv[])
{
    tll::Logger<0x400, 0x1000, 10> lg
        (
            tll::LogFd{tll::lf_t, console},
            tll::LogFd{tll::lf_d | tll::lf_i | tll::lf_f | tll::lf_t, open("fd_t.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
            // ,tll::LogFd{tll::toFlag(tll::LogType::kInfo, tll::LogType::kDebug), open("fd_i.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
            // ,tll::LogFd{tll::toFlag(tll::LogType::kFatal), open("fd_f.log", O_WRONLY | O_TRUNC | O_CREAT , 0644)}
        );

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
                TLL_LOGI(lg, "%d %s", 10, "oi troi oi");
                TLL_LOGF(lg, "%d %s", 10, "oi troi oi");
            }
        }
        lg.join();
    }

    {
        TLL_LOGT(lg, multi);
        {
            TLL_LOGT(lg, multi_inner);
            // #pragma omp parallel for
            std::future<void> futs[2];
            for(auto &fut : futs)
                fut = std::async(std::launch::async, [&lg](){
                    for(int i=0; i < (100000 / 2); i++)
                    {
                        TLL_LOGD(lg, "%d %s", 10, "oi troi oi");
                        TLL_LOGI(lg, "%d %s", 10, "oi troi oi");
                        TLL_LOGF(lg, "%d %s", 10, "oi troi oi");
                    }
                });

            for(auto &fut : futs) fut.get();
        }
        lg.join();
    }


    return 0;
}

