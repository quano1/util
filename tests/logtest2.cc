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
    double counter[3];
    memset(counter, 0, sizeof(counter));
    tll::log::Timer timer;

    {
        auto &logger = tll::log::Node::instance();
        logger.remove("console");
        TLL_GLOGTF();
        tll::log::Entity console_ent{
                        .name = "console",
                        .flag = tll::log::Flag::kAll, .chunk_size = 0x1000,
                        .send = std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)};
        tll::log::Entity file_ent1{
                        .name = "file", .flag = tll::log::Flag::kAll, .chunk_size = 0x10000,
                        .send = [&](void *handle, const char *buff, size_t size)
                        {
                            if(handle == nullptr)
                            {
                                printf("%.*s", (int)size, buff);
                                return;
                            }
                            static_cast<std::ofstream*>(handle)->write((const char *)buff, size);
                            write_cnt++;
                        },
                        .open = []()
                        {
                            write_cnt=0;
                            return static_cast<void*>(new std::ofstream("file_ent1.log", std::ios::out | std::ios::binary));
                        }, 
                        .close = [](void *&handle)
                        {
                            static_cast<std::ofstream*>(handle)->flush();
                            delete static_cast<std::ofstream*>(handle);
                            handle = nullptr;
                        }};
        tll::log::Entity file_ent2{
                        .name = "file", .flag = tll::log::Flag::kAll, .chunk_size = 0x10000,
                        .send = [&](void *handle, const char *buff, size_t size)
                        {
                            if(handle == nullptr) return;
                            static_cast<std::ofstream*>(handle)->write((const char *)buff, size);
                            static_cast<std::ofstream*>(handle)->flush();
                            write_cnt++;
                        },
                        .open = []()
                        {
                            write_cnt=0;
                            return static_cast<void*>(new std::ofstream("file_ent2.log", std::ios::out | std::ios::binary));
                        }, 
                        .close = [](void *&handle)
                        {
                            delete static_cast<std::ofstream*>(handle);
                            handle = nullptr;
                        }};
        logger.add(file_ent1);

        timer.reset();
        logger.start();
        counter[0] = timer.elapse();
        {
            TLL_GLOGT(start);
            // #pragma omp parallel num_threads ( 16 )
            {
                TLL_GLOGT(omp_parallel);
                timer.reset();
                for(int i=0; i < std::stoi(argv[1]); i++)
                {
                    TLL_GLOGD("this is debug logging");
                    TLL_GLOGI("Some information");
                    TLL_GLOGW("A warning!!!");
                    TLL_GLOGF("Ooops!!! fatal logging");
                }
                counter[1] = timer.elapse();
            }
        }
        TLL_GLOGI("Write Count: %d", write_cnt);
        timer.reset();
        logger.stop();
        counter[2] = timer.elapse();
        // logger.remove("file");
        // logger.add(file_ent2);
        // logger.start();
        // {
        //     TLL_GLOGT(start);
        //     // #pragma omp parallel num_threads ( 16 )
        //     {
        //         TLL_GLOGT(omp_parallel);
        //         for(int i=0; i < std::stoi(argv[1]); i++)
        //         {
        //             TLL_GLOGD("this is debug logging");
        //             TLL_GLOGI("Some information");
        //             TLL_GLOGW("A warning!!!");
        //             TLL_GLOGF("Ooops!!! fatal logging");
        //         }
        //     }
        // }
        // logger.stop();
        // TLL_GLOGI("Write Count: %d", write_cnt);
    }
    for(auto val : counter)
        LOGD("%.3f", val);

    LOGD("%.3f", timer.abs_elapse());

    return 0;
}


