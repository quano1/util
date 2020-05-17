#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <future>

#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<netdb.h> //hostent

// #include "../libs/SimpleSignal.hpp"
// #include "../libs/utils.h"
#include "../libs/logger.h"
// #include "../libs/exporterudp.h"

// template <typename L>
struct A
{
    A(std::function<void(int, std::string const&)> logf) : life_(std::bind(logf,static_cast<int>(tll::LogType::kInfo), std::placeholders::_1), _LOG_HEADER, __FUNCTION__)
    {
        sig_log.connect(logf);
        // sig_log.emit(static_cast<int>(tll::LogType::kInfo), utils::stringFormat("%s\n", _LOG_HEADER));
    }

    ~A()
    {
        // sig_log.emit(static_cast<int>(tll::LogType::kInfo), utils::stringFormat("%s\n", _LOG_HEADER));
    }

    // L &logger_;
    // L sig_log_;
    Simple::Signal<void(int, std::string const&)> sig_log;
    utils::Timer life_;
};

namespace {
    int const console = 1;
}

inline uint16_t myhtons(uint16_t port) {return htons(port);}

using Logger = tll::Logger<0x400, 0x1000, 10000>;
Logger *plogger;
        

int main(int argc, char const *argv[])
{
    LOGD("");
    int write_count = 0;
    plogger = new Logger(
            tll::LogEntity{tll::trace, nullptr, nullptr, std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2), 0x1000, nullptr},

            tll::LogEntity{tll::all, 
                [](){return static_cast<void*>(new std::ofstream("ofs_write.log", std::ios::out | std::ios::binary));}, 
                [](void *handle){delete static_cast<std::ofstream*>(handle);},
                [&](void *handle, const char *buff, size_t size){static_cast<std::ofstream*>(handle)->write((const char *)buff, size);
                    static_cast<std::ofstream*>(handle)->flush(); write_count++;}, 0x100000, nullptr}

            ,tll::LogEntity{tll::all,
                []()->void*{
                    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    struct hostent *host;
                    host = gethostbyname("localhost");
                    sockaddr_in svr_addr;
                    svr_addr.sin_family = AF_INET;
                    svr_addr.sin_port = myhtons(65501);
                    bcopy((char *)host->h_addr, (char *)&svr_addr.sin_addr.s_addr, host->h_length);

                    // set non-blocking io
                    if ( fcntl( sock_fd, F_SETFL, O_NONBLOCK, 1 ) == -1 )
                    {
                        LOGD( "failed to make socket non-blocking" );
                        return reinterpret_cast<void*>(-1);
                    }

                    if (connect(sock_fd, (const sockaddr*)&svr_addr, sizeof(svr_addr)) < 0)
                        LOGE("ERROR connecting");

                    return reinterpret_cast<void*>(sock_fd);
                },
                [](void *handle){
                    int64_t sock_fd = reinterpret_cast<int64_t>(handle);
                    if(sock_fd>-1) 
                        close(sock_fd);
                },
                [](void *handle, const char *buff, size_t size){
                    int64_t sock_fd = reinterpret_cast<int64_t>(handle);
                    if(sock_fd>-1)
                    {
                        (void)write(sock_fd, buff, size);
                    }
                }, 0x1000, nullptr}
        );
    auto &lg = *plogger;
    // lg.init();
    // lg.start();

    auto logf = [&lg](int type, std::string const &log_msg){lg.log(type, "%s", log_msg);};
    // auto myprinf = std::bind(std::printf, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
    // auto logf = std::bind(&Logger::log<Args...>, &lg, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    {
        TLL_LOGTF(&lg);

        // A a(logf);
        // if(argc > 1)
        #pragma omp parallel num_threads ( 4 )
        {
            TLL_LOGT(&lg, single);
            {
                TLL_LOGT(&lg, single_inner);
                // #pragma omp parallel for
                for(int i=0; i < std::stoi(argv[1]); i++)
                {
                    TLL_GLOGD("%s: %d", "log something for debuging", 1);
                    TLL_GLOGI("%s: %d", "a little bit of information", 2);
                    TLL_GLOGW("%s: %d", "this is a warning", 3);
                    TLL_GLOGF("%s: %d", "Oops, faltal!", 4);
                }
            }
        }
    }
    LOGD("");
    lg.join();
    lg.flushAll();
    LOGD("%d", write_count);
    // delete plg;
    // LOGD("");

    // lg.write_count_=0;
    // // lg.batch_mode_=false;
    // {
    //     TLL_LOGT(lg, single);
    //     {
    //         TLL_LOGT(lg, single_inner);
    //         // #pragma omp parallel for
    //         for(int i=0; i < 100000; i++)
    //         {
    //             TLL_LOGD(lg, "%d %s", 10, "oi troi oi");
    //             TLL_LOGF(lg, "%d %s", 10, "oi troi oi");
    //         }
    //     }
    // }
    // lg.join();
    // LOGD("%d", lg.write_count_);

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

