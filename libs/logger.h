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

#include <omp.h>

#ifndef STATIC_LIB
#define TLL_INLINE
#else
#define TLL_INLINE inline
#endif


namespace tll {

typedef uint32_t LogType;

namespace logtype { /// logtype
static const LogType kDebug=(1U << 0);
static const LogType kTrace=(1U << 1);
static const LogType kInfo=(1U << 2);
static const LogType kFatal=(1U << 3);
}

typedef std::pair<LogType, std::string> LogInfo;
typedef std::pair<LogType, int> LogFd;

template <size_t const kLogSize, uint32_t max_log_in_queue, uint32_t const kDelayMicro>
class Logger
{
public:
    template < typename ... LFds>
    Logger(LFds ...lfds) : ring_queue_(max_log_in_queue), is_running_(false)
    {
        addFd__(lfds...);
    }

    ~Logger() 
    {
        is_running_.store(false, std::memory_order_relaxed);
        if(broadcast_.joinable()) broadcast_.join();
        for(auto lfd : lfds_)
        {
            close(lfd.second);
        }
    }

    template <LogType type, typename... Args>
    void log(const char *format, Args &&...args)
    {
        ring_queue_.push([](LogInfo &elem, uint32_t size, LogInfo &&log_msg)
        {
            elem = std::move(log_msg);
        }, LogInfo{type, utils::Format(kLogSize, format, std::forward<Args>(args)...)});

        if(!is_running_.load(std::memory_order_relaxed)) start();
    }

    TLL_INLINE void start()
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed)) return;

        broadcast_ = std::thread([this]()
        {
            while(is_running_.load(std::memory_order_relaxed))
            {
                if(ring_queue_.empty())
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(kDelayMicro));
                    continue;
                }

                LogInfo log_message;
                ring_queue_.pop([&log_message](LogInfo &elem, uint32_t)
                {
                    log_message = std::move(elem);
                });

                // #pragma omp parallel for
                for(int i=0; i<lfds_.size(); i++)
                {
                    LogFd &lfd = lfds_[i];
                    if(lfd.first & log_message.first)
                    {
                        auto size = write(lfd.second, log_message.second.data(), log_message.second.size());
                    }
                }
            }
        });
    }

    TLL_INLINE void join()
    {
        while(is_running_.load(std::memory_order_relaxed) && !ring_queue_.empty())
            std::this_thread::sleep_for(std::chrono::microseconds(kDelayMicro));
    }

    template < typename ... LFds>
    void addFd(LFds ...lfds)
    {
        if(is_running_.load(std::memory_order_relaxed)) return;
        addFd__(lfds...);
    }

private:
    template <typename ... LFds>
    void addFd__(LogFd lfd, LFds ...lfds)
    {
        lfds_.push_back(lfd);
        addFd__(lfds...);
    }

    TLL_INLINE void addFd__(LogFd lfd)
    {
        lfds_.push_back(lfd);
    }

    utils::BSDLFQ<LogInfo> ring_queue_;
    std::atomic<bool> is_running_;
    std::thread broadcast_;
    std::vector<LogFd> lfds_;
};

} // llt

#define LOG_HEADER__ utils::Format("(%.6f)%s:%s:%d[%s]", utils::timestamp<double>(), __FILE__, __FUNCTION__, __LINE__, utils::tid())

#define TLL_LOGD(logger, format, ...) (logger).log<tll::logtype::kDebug>("[D]%s(" format ")\n", LOG_HEADER__ , ##__VA_ARGS__)

#define TLL_LOGTF(logger) (logger).log<tll::logtype::kTrace>("[T]%s", LOG_HEADER__); utils::Timer timer__([&logger](std::string const &str){logger.log<tll::logtype::kTrace>("%s", str.data());}, __FUNCTION__)

#define TLL_LOGT(logger, ID) (logger).log<tll::logtype::kTrace>("[T]%s", LOG_HEADER__); utils::Timer timer_##ID__([&logger](std::string const &str){logger.log<tll::logtype::kTrace>("%s", str.data());}, #ID)

#define TLL_LOGI(logger, format, ...) (logger).log<tll::logtype::kInfo>("[I]%s(" format ")\n", LOG_HEADER__ , ##__VA_ARGS__)

#define TLL_LOGF(logger, format, ...) (logger).log<tll::logtype::kFatal>("[F]%s(" format ")\n", LOG_HEADER__ , ##__VA_ARGS__)

#ifndef STATIC_LIB
#include "logger.cc"
#endif