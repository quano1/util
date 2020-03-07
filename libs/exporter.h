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

union LogFlag {
    struct
    {
        uint32_t trace : 1;
        uint32_t info : 1;
        uint32_t fatal : 1;
        uint32_t reserve : 28;
        uint32_t debug : 1;
    };
    uint32_t value;
};

enum class LogType : uint32_t
{
    kTrace=(1U << 0),
    kInfo=(1U << 1),
    kFatal=(1U << 2),
    kDebug=(1U << 31)
};

typedef std::pair<LogType, std::string> LogInfo;

template <size_t const kLogSize, uint32_t max_log_queue, uint32_t const kDelay>
class Logger
{
public:
    Logger() : lf_queue_(max_log_queue), is_running_(false)
    {
        
    }

    ~Logger() 
    {
        is_running_ = false;
        if(broadcast_thread.joinable()) broadcast_thread.join();
    }

    UTIL_INLINE void start()
    {
        is_running_ = true;
        broadcast_thread = std::thread([this]()
        {
            while(is_running_)
            {
                using namespace std;
                if(lf_queue_.empty())
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(kDelay));
                    continue;
                }

                LogInfo log_message;
                lf_queue_.pop([](LogInfo &elem, uint32_t, LogInfo &log_msg)
                {
                    log_msg = std::move(elem);
                }, log_message);

                for(auto fd : fds_)
                {
                    if((uint32_t)fd.second.value & (uint32_t)log_message.first)
                    {
                        auto size = write(fd.first, log_message.second.data(), log_message.second.size());
                    }
                }
            }
        });
    }

    UTIL_INLINE void join()
    {
        while(is_running_ && !lf_queue_.empty())
            std::this_thread::sleep_for(std::chrono::microseconds(kDelay));
    }

    template <LogType type, typename... Args>
    void log(const char *format, Args &&...args)
    {
        lf_queue_.push([](LogInfo &elem, uint32_t, LogInfo &&log_msg)
        {
            elem = std::move(log_msg);
        }, LogInfo{type, utils::Format(kLogSize, format, std::forward<Args>(args)...)});
    }

    template <typename ... Fds>
    void addFd(std::pair<int, LogFlag> const &fd, Fds ...fds)
    {
        if(is_running_) return;
        fds_[fd.first] = fd.second;
        addFd(fds...);
    }

    UTIL_INLINE void addFd(std::pair<int, LogFlag> const &fd)
    {
        if(is_running_) return;
        fds_[fd.first] = fd.second;
    }

    utils::BSDLFQ<LogInfo> lf_queue_;
    bool is_running_;
    std::thread broadcast_thread;
    std::unordered_map<int, LogFlag> fds_;
};

} // llt


#ifndef STATIC_LIB
#include "exporter.cc"
#endif