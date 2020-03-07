#pragma once

#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <sstream>
#include <string>
#include <unordered_set>

#include <cstdarg>
#include <iomanip>

#include <netinet/in.h>
#include <unistd.h> // close
#include <sys/socket.h>
#include <sys/un.h>
#include <functional>
#include "utils.h"

#ifndef STATIC_LIB
#define UTIL_INLINE
#else
#define UTIL_INLINE inline
#endif


namespace llt {

enum class LogType : uint8_t
{
    kTrace=0,
    kDebug,
    kInfo,
    kFatal,
};

template <uint32_t elem_size, uint32_t num_of_elem, uint32_t delay>
class Logger
{
public:
    Logger() : lf_queue_(num_of_elem), is_running_(true), is_dirty_(false)
    {
        writer_ = std::thread([this]()
        {
            while(is_running_)
            {
                using namespace std;
                // uint32_t head;
                // while(!lf_queue_.tryPop(head) && is_running_)
                //     std::this_thread::sleep_for(std::chrono::microseconds(delay));

                // if(!is_running_) break;

                // std::string log_message(lf_queue_.data(head));
                // lf_queue_.completePop(head);

                std::string log_message;
                lf_queue_.pop([&log_message](char *, char const *buff, uint32_t)
                {
                    log_message = std::string(buff);
                });
                if(is_dirty_)
                {
                    std::lock_guard<std::mutex> lock(fd_lock_);
                    is_dirty_ = false;
                    for(auto fd : add_reqs_) fds_.insert(fd);
                    for(auto fd : rem_reqs_) fds_.erase(fd);

                    add_reqs_.clear();
                    rem_reqs_.clear();
                }
                for(auto fd : fds_)
                {
                    auto size = write(fd, log_message.data(), log_message.size());
                }
            }
        });
    }

    ~Logger() 
    {
        is_running_ = false;
        writer_.join();
    }

    inline void join()
    {
        while(lf_queue_.available())
            std::this_thread::sleep_for(std::chrono::microseconds(delay));
    }

    template <typename... Args>
    void log(const char *format, Args &&...args)
    {
        std::string log_msg = utils::Format(elem_size, format, std::forward<Args>(args)...);
        // lf_queue_.push(log_msg.data());
        lf_queue_.push([](char const *elem, char *buff, uint32_t size)
        {
            memcpy(buff, elem, size);
        }, log_msg.data());
    }

    // template <typename ... Fds>
    // void add(int fd, Fds ...fds)
    // {
    //     add_reqs_.push_back(fd);
    //     add(fds...);
    // }

    inline void add(int fd)
    {
        std::lock_guard<std::mutex> lock(fd_lock_);
        add_reqs_.push_back(fd);
        is_dirty_ = true;
    }

    // template <typename ... Fds>
    // void rem(int fd, Fds ...fds)
    // {
    //     rem_reqs_.push_back(fd);
    //     rem(fds...);
    // }
    
    inline void rem(int fd)
    {
        std::lock_guard<std::mutex> lock(fd_lock_);
        rem_reqs_.push_back(fd);
        is_dirty_ = true;
    }

    utils::BSDLFQ<elem_size, char> lf_queue_;
    bool is_dirty_;
    bool is_running_;
    std::thread writer_;
    std::unordered_set<int> fds_;
    std::mutex fd_lock_;
    std::vector<int> add_reqs_;
    std::vector<int> rem_reqs_;
};

} // llt


#ifndef STATIC_LIB
#include "exporter.cc"
#endif