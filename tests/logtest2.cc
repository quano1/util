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

int write_cnt=0;

int main(int argc, char const *argv[])
{
    auto &logger = tll::log::Node::instance();
    // auto ptr = logger.connect(tll::log::Flag::kAll, std::bind(printf, "%.*s", std::placeholders::_2, std::placeholders::_1));
    // tll::time::List<> timer;
    TLL_GLOGTF();
    {
        TLL_GLOGT(MAIN);
        logger.remove("console");
        // tll::log::Entity console_ent{
        //                 .name = "console",
        //                 .flag = tll::log::Flag::kAll,
        //                 .on_log = std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)};
        tll::log::Entity file_ent{
                        .name = "file", .flag = tll::log::Flag::kAll,
                        .on_log= [](void *handle, const char *buff, size_t size)
                        {
                            if(handle == nullptr)
                            {
                                printf("%.*s", (int)size, buff);
                                return;
                            }
                            static_cast<std::ofstream*>(handle)->write((const char *)buff, size);
                            write_cnt++;
                        },
                        .on_start = []()
                        {
                            write_cnt=0;
                            return static_cast<void*>(new std::ofstream("file_ent.log", std::ios::out | std::ios::binary));
                        }, 
                        .on_stop = [](void *&handle)
                        {
                            static_cast<std::ofstream*>(handle)->flush();
                            static_cast<std::ofstream*>(handle)->close();
                            delete static_cast<std::ofstream*>(handle);
                            handle = nullptr;
                        }};
        logger.add(file_ent);
        {
            tll::util::Guard time_guard{tracer__("starting")};
            logger.start();
        }
        {
            TLL_GLOGT(log);
            #pragma omp parallel num_threads ( 16 )
            {
                TLL_GLOGT(omp_parallel);
                tll::util::Guard time_guard{tracer__("do logging")};
                for(int i=0; i < std::stoi(argv[1]); i++)
                {
                    TLL_GLOGD("this is debug logging");
                    TLL_GLOGI("Some information");
                    TLL_GLOGW("A warning!!!");
                    TLL_GLOGF("Ooops!!! fatal logging");
                    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(argv[2])));
                }
            }
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        {
            tll::util::Guard time_guard{tracer__("stopping")};
            logger.stop();
        }

        logger.remove("file_ent");
    }

    TLL_GLOGD("Write Count: %d", write_cnt);
    TLL_GLOGD("starting: %.6f", tracer__("starting").total().count());
    TLL_GLOGD("do logging: %.6f", tracer__("do logging").total().count());
    TLL_GLOGD("stopping: %.6f", tracer__("stopping").total().count());
    TLL_GLOGD("elapse: %.6f", tracer__().elapse().count());

    return 0;
}


