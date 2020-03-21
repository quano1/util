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

typedef uint32_t LogFlag;

enum class LogType /// LogType
{
    debug=0,
    info,
    fatal,
    trace,
};

char const kLogTypeString[]="DIFT";

TLL_INLINE constexpr LogFlag toFlag(LogType type)
{
    return 1U << static_cast<LogFlag>(type);
}

constexpr LogFlag lf_d = toFlag(LogType::debug);
constexpr LogFlag lf_t = toFlag(LogType::trace);
constexpr LogFlag lf_i = toFlag(LogType::info);
constexpr LogFlag lf_f = toFlag(LogType::fatal);

template <typename... Args>
LogFlag toFlag(tll::LogType type, Args ...args)
{
    return toFlag(type) | toFlag(args...);
}

// namespace LogType { /// LogType
// static const LogType debug=(1U << 0);
// static const LogType trace=(1U << 1);
// static const LogType info=(1U << 2);
// static const LogType fatal=(1U << 3);
// }

typedef std::pair<LogType, std::string> LogInfo;
typedef std::pair<LogFlag, int> LogFd;

template <size_t kLogSize, uint32_t max_log_in_queue, uint32_t kDelayUs>
class Logger
{
public:
    template < typename ... LFds>
    Logger(LFds ...lfds) : ring_queue_(max_log_in_queue), is_running_(false)
    {
        _addFd(lfds...);
    }

    ~Logger() 
    {
        join();
        is_running_.store(false, std::memory_order_relaxed);
        if(broadcast_.joinable()) broadcast_.join();
        for(auto lfd : lfds_)
        {
            close(lfd.second);
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    template <LogType type, typename... Args>
    void log(const char *format, Args &&...args)
    {
        ring_queue_.push(
            [](){std::this_thread::sleep_for(std::chrono::microseconds(kDelayUs));},
            [](LogInfo &elem, uint32_t size, LogInfo &&log_msg)
            {
                elem = std::move(log_msg);
            }, LogInfo{type, utils::stringFormat(kLogSize, format, std::forward<Args>(args)...)});

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
                    std::this_thread::sleep_for(std::chrono::microseconds(kDelayUs));
                    continue;
                }

                LogInfo log_msg;
                ring_queue_.pop(
                    [](){std::this_thread::sleep_for(std::chrono::microseconds(kDelayUs));},
                    [&log_msg](LogInfo &elem, uint32_t)
                    {
                        log_msg = std::move(elem);
                    });

                // FIXME: parallel is 10 times slower???
                // #pragma omp parallel for
                for(int i=0; i<lfds_.size(); i++)
                {
                    LogFd &lfd = lfds_[i];
                    LogType const kLogt = log_msg.first;
                    if(lfd.first & tll::toFlag(kLogt))
                    {
                        int const kFd = lfd.second;
                        // std::string &msg = log_msg.second;
                        std::string msg = utils::stringFormat(kLogSize, "{%c}%s", kLogTypeString[(uint32_t)(kLogt)], log_msg.second);
                        auto size = write(kFd, msg.data(), msg.size());
                    }
                }
            }
        });
    }

    TLL_INLINE void join()
    {
        while(is_running_.load(std::memory_order_relaxed) && !ring_queue_.empty())
            std::this_thread::sleep_for(std::chrono::microseconds(kDelayUs));
    }

    template < typename ... LFds>
    void addFd(LFds ...lfds)
    {
        if(is_running_.load(std::memory_order_relaxed)) return;
        _addFd(lfds...);
    }

private:
    template <typename ... LFds>
    void _addFd(LogFd lfd, LFds ...lfds)
    {
        lfds_.push_back(lfd);
        _addFd(lfds...);
    }

    TLL_INLINE void _addFd(LogFd lfd)
    {
        lfds_.push_back(lfd);
    }

    utils::BSDLFQ<LogInfo> ring_queue_;
    std::atomic<bool> is_running_;
    std::thread broadcast_;
    std::vector<LogFd> lfds_;
};

} // llt

#define _LOG_HEADER utils::stringFormat("{%.6f}{%s}{%s}{%s}{%d}", utils::timestamp<double>(), utils::tid(), __FILE__, __FUNCTION__, __LINE__)

#define TLL_LOGD(logger, format, ...) (logger).log<tll::LogType::debug>("%s{" format "}\n", _LOG_HEADER , ##__VA_ARGS__)

#define TLL_LOGTF(logger) (logger).log<tll::LogType::trace>("%s\n", _LOG_HEADER); utils::Timer timer_([&logger](std::string const &log_msg){logger.log<tll::LogType::trace>("%s", log_msg);}, __FUNCTION__)

#define TLL_LOGT(logger, ID) (logger).log<tll::LogType::trace>("%s\n", _LOG_HEADER); utils::Timer timer_##ID_([&logger](std::string const &log_msg){logger.log<tll::LogType::trace>("%s", log_msg);}, #ID)

#define TLL_LOGI(logger, format, ...) (logger).log<tll::LogType::info>("%s{" format "}\n", _LOG_HEADER , ##__VA_ARGS__)

#define TLL_LOGF(logger, format, ...) (logger).log<tll::LogType::fatal>("%s{" format "}\n", _LOG_HEADER , ##__VA_ARGS__)

#ifndef STATIC_LIB
#include "logger.cc"
#endif