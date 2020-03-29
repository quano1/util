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
    kDebug=0,
    kInfo,
    kFatal,
    kTrace,
};

char const kLogTypeString[]="DIFT";

TLL_INLINE constexpr LogFlag toFlag(LogType type)
{
    return 1U << static_cast<LogFlag>(type);
}

template <typename... Args>
constexpr LogFlag toFlag(tll::LogType type, Args ...args)
{
    return toFlag(type) | toFlag(args...);
}

constexpr LogFlag lf_d = toFlag(LogType::kDebug);
constexpr LogFlag lf_t = toFlag(LogType::kTrace);
constexpr LogFlag lf_i = toFlag(LogType::kInfo);
constexpr LogFlag lf_f = toFlag(LogType::kFatal);

// namespace LogType { /// LogType
// static const LogType debug=(1U << 0);
// static const LogType trace=(1U << 1);
// static const LogType info=(1U << 2);
// static const LogType fatal=(1U << 3);
// }

typedef std::pair<LogType, std::string> LogInfo;
// typedef std::pair<LogFlag, int> LogFd;
typedef std::tuple<LogFlag, int, std::function<void(int, const void*, size_t)>> LogFd;

template <size_t kLogSize, uint32_t max_log_in_queue, uint32_t kDelayUs, size_t kChunkSize=0>
class Logger
{
public:
    template < typename ... LFds>
    Logger(LFds ...lfds) : write_count_(0), ring_queue_(max_log_in_queue), is_running_(false)
    {
        _addFd(lfds...);
        init();
    }

    ~Logger() 
    {
        this->join();
        is_running_.store(false, std::memory_order_relaxed);
        if(broadcast_.joinable()) broadcast_.join();
        this->flushAll();
        LOGD("%d", write_count_);
        for(auto lfd : lfds_)
        {
            const int fd = std::get<1>(lfd);
            if(fd > 0)
                close(fd);
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    template <typename... Args>
    void log(int type, const char *format, Args &&...args)
    {
        ring_queue_.push(
            [](){std::this_thread::sleep_for(std::chrono::microseconds(kDelayUs));},
            [](LogInfo &elem, uint32_t size, LogInfo &&log_msg)
            {
                elem = std::move(log_msg);
            }, LogInfo{static_cast<LogType>(type), utils::stringFormat(kLogSize, format, std::forward<Args>(args)...)}
        );

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

                LogInfo log_inf;
                ring_queue_.pop(
                    [](){std::this_thread::sleep_for(std::chrono::microseconds(kDelayUs));},
                    [&log_inf](LogInfo &elem, uint32_t)
                    {
                        log_inf = std::move(elem);
                    });

                // FIXME: parallel is 10 times slower???
                // #pragma omp parallel for
                for(int i=0; i<lfds_.size(); i++)
                {
                    LogFd &lfd = lfds_[i];
                    // if(!kChunkSize) 
                    //     { flush(lfd, log_inf); continue; }

                    LogType const kLogt = log_inf.first;
                    LogFlag flag = std::get<0>(lfd);
                    if(flag & tll::toFlag(kLogt))
                    {
                        // int const kFd = lfd.second;
                        // std::string &msg = log_inf.second;
                        std::string msg = utils::stringFormat(kLogSize, "{%c}%s", kLogTypeString[(uint32_t)(kLogt)], log_inf.second);
                        //     auto size = write(kFd, msg.data(), msg.size());
                        auto &buff = buffers_[i];
                        size_t const old_size = buff.size();
                        size_t const new_size = old_size + msg.size();

                        buff.resize(new_size);
                        memcpy(buff.data() + old_size, msg.data(), msg.size());

                        if(new_size >= kChunkSize)
                            this->flush(i);
                    }
                }
            }
        });
    }

    template < typename ... LFds>
    void addFd(LFds ...lfds)
    {
        if(is_running_.load(std::memory_order_relaxed)) return;
        _addFd(lfds...);
    }

    int write_count_;

    TLL_INLINE void join()
    {
        while(is_running_.load(std::memory_order_relaxed) && !ring_queue_.empty())
            std::this_thread::sleep_for(std::chrono::microseconds(kDelayUs));
    }

private:

    TLL_INLINE void init()
    {
        buffers_.resize(lfds_.size());
        if(kChunkSize == 0) return;
        for(auto &buff : buffers_) buff.reserve(kChunkSize*2);
    }

    TLL_INLINE void flush(int idx)
    {
        int fd = std::get<1>(lfds_[idx]);
        auto logf = std::get<2>(lfds_[idx]);
        auto &buff = buffers_[idx];
        logf(fd, buff.data(), buff.size());
        buff.resize(0);
        write_count_++;
    }

    // TLL_INLINE void flush(LogFd lfd, LogInfo const &linfo)
    // {
    //     LogType const kLogt = linfo.first;
    //     if(std::get<0>(lfd) & tll::toFlag(kLogt))
    //     {
    //         int const kFd = std::get<1>(lfd);
    //         std::string msg = utils::stringFormat(kLogSize, "{%c}%s", kLogTypeString[(uint32_t)(kLogt)], linfo.second);
    //         write_count_++;
    //         // auto size = write(kFd, msg.data(), msg.size());
    //         std::get<2>(lfd)(std::get<1>(lfd), msg.data(), msg.size());
    //     }
    // }

    TLL_INLINE void flushAll()
    {
        for(int i=0; i<lfds_.size(); i++) this->flush(i);
    }

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
    std::atomic<bool> is_flushing_;
    std::thread broadcast_;
    std::vector<LogFd> lfds_;
    std::vector<std::vector<char>> buffers_;
};

// template <typename T>
// struct Tracer
// {
//     Tracer()
// };

} // llt

#define _LOG_HEADER utils::stringFormat("{%.6f}{%s}{%s}{%s}{%d}", utils::timestamp<double>(), utils::tid(), __FILE__, __FUNCTION__, __LINE__)

#define TLL_LOGD(logger, format, ...) (logger).log(static_cast<int>(tll::LogType::kDebug), "%s{" format "}\n", _LOG_HEADER , ##__VA_ARGS__)

#define TLL_LOGTF(logger) utils::Timer timer_([&logger](std::string const &log_msg){logger.log(static_cast<int>(tll::LogType::kTrace), "%s", log_msg);}, _LOG_HEADER, (/*(logger).log(static_cast<int>(tll::LogType::kTrace), "%s\n", _LOG_HEADER),*/ __FUNCTION__))

#define TLL_LOGT(logger, ID) utils::Timer timer_##ID_([&logger](std::string const &log_msg){logger.log(static_cast<int>(tll::LogType::kTrace), "%s", log_msg);}, _LOG_HEADER, (/*(logger).log(static_cast<int>(tll::LogType::kTrace), "%s\n", _LOG_HEADER),*/ #ID))

#define TLL_LOGI(logger, format, ...) (logger).log(static_cast<int>(tll::LogType::kInfo), "%s{" format "}\n", _LOG_HEADER , ##__VA_ARGS__)

#define TLL_LOGF(logger, format, ...) (logger).log(static_cast<int>(tll::LogType::kFatal), "%s{" format "}\n", _LOG_HEADER , ##__VA_ARGS__)

#ifndef STATIC_LIB
#include "logger.cc"
#endif