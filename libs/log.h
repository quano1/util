#pragma once

/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#include <string>
#include <cassert>
#include <cstring>

#include <vector>
#include <unordered_map>

#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <omp.h>
#include <unistd.h>

#include "util.h"
#include "lffifo.h"

#ifndef TLL_DEFAULT_LOG_PATH
#define TLL_DEFAULT_LOG_PATH "."
#endif

extern char *__progname;

#define COMBINE_(X,Y) X ##_ ##Y // helper macro
#define COMBINE(X,Y) COMBINE_(X,Y)

#define TLL_LOG(plog, severity, ...) \
    (plog)->log<Mode::kAsync>((tll::log::Severity)severity, \
                                __FILE__, __FUNCTION__, __LINE__, \
                                ##__VA_ARGS__)

#define TLL_GLOG(severity, ...) TLL_LOG(&tll::log::Manager::instance(), severity, ##__VA_ARGS__)

#define TLL_GLOGD(...) TLL_GLOG((tll::log::Severity::kDebug), ##__VA_ARGS__)
#define TLL_GLOGI(...) TLL_GLOG((tll::log::Severity::kInfo), ##__VA_ARGS__)
#define TLL_GLOGW(...) TLL_GLOG((tll::log::Severity::kWarn), ##__VA_ARGS__)
#define TLL_GLOGF(...) TLL_GLOG((tll::log::Severity::kFatal), ##__VA_ARGS__)
#define TLL_GLOGT(ID) \
    tll::log::Tracer COMBINE(tracer, __LINE__)(ID, \
    std::bind(&tll::log::Manager::log<Mode::kAsync, Tracer *, const std::string &, double>, &tll::log::Manager::instance(), (tll::log::Severity::kTrace), __FILE__, __FUNCTION__, __LINE__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))

#define TLL_GLOGTF()   TLL_GLOGT(__FUNCTION__)

namespace tll::log {

char const kLogSeverityString[]="TDIWF";
enum class Severity
{
    kTrace = 0,
    kDebug,
    kInfo,
    kWarn,
    kFatal,
    kMax,
};

enum class Mode
{
    kSync = 0,
    kAsync,
};

struct Message
{
    Severity severity;
    std::string payload;
};

struct Entity
{
    std::string name;
    std::function<void(void*, bool, const char*, size_t)> onLog;
    std::function<void *()> onStart;
    std::function<void(void *&)> onStop;
    void *handle;
    // friend class Manager;
};

namespace context {
    static thread_local int level;
}

struct Tracer
{
    typedef std::function <void (const char *format, Tracer *, const std::string &, double)> LogCallback;
private:
    std::string name_;
    LogCallback doLog_;
public:
    util::Counter<> counter;

    Tracer(const std::string &name, LogCallback &&doLog) : name_(name), doLog_(std::move(doLog))
    {
        doLog_("+%p:%s", this, name_, 0);
        context::level++;
    }

    ~Tracer()
    {
        context::level--;
        doLog_("-%p:%s %.9f(s)", this, name_, counter.elapse().count());
    }
};

// struct StaticLogInfo
// {
//     const char *format;
//     const char *file;
//     const char *func;
//     const int severity;
//     const int line;
// }; /// StaticLogInfo

class Manager : public util::SingletonBase<Manager>
{
public:
    size_t total_size = 0;
private:
    std::atomic<bool> is_running_{false}, flush_request_{false};
    // lf::CCFIFO<Message> ccq_{0x1000};
    lf::ring_queue_ds<std::function<std::string()>> task_queue_{0x1000};
    // not supported
    lf::ring_buffer_ds<char> ring_buffer_{0x400 * 32}; /// 32Kb
    std::unordered_map<std::string, Entity> raw_ents_;/* = {{"file", Entity{
                .name = "file",
                [this](void *handle, bool flush_req, const char *buff, size_t size)
                {
                    if(handle == nullptr)
                    {
                        printf("%.*s", (int)size, buff);
                    }
                    else
                    {
                        auto ofs = static_cast<std::ofstream*>(handle);
                        ofs->write((const char *)buff, size);
                        if(flush_req) ofs->flush();
                    }
                },
                [this]()
                {
                    auto log_path =std::getenv("TLL_LOG_PATH");
                    auto const &file = util::stringFormat("%s/%s.%d.raw.log", log_path ? log_path : TLL_DEFAULT_LOG_PATH, __progname, getpid());
                    LOGD("%s", file.data());
                    return static_cast<void*>(new std::ofstream(file, std::ios::out | std::ios::binary));
                },
                [this](void *&handle)
                {
                    if(handle)
                    {
                        // static_cast<std::ofstream*>(handle)->flush();
                        // static_cast<std::ofstream*>(handle)->close();
                        delete static_cast<std::ofstream*>(handle);
                        handle = nullptr;
                    }
                }
            }}};*/

    std::unordered_map<std::string, Entity> pre_ents_ = {{"file", Entity{
                .name = "file",
                [this](void *handle, bool flush_req, const char *buff, size_t size)
                {
                    if(handle == nullptr)
                    {
                        printf("%.*s", (int)size, buff);
                    }
                    else
                    {
                        auto ofs = static_cast<std::ofstream*>(handle);
                        ofs->write((const char *)buff, size);
                        if(flush_req) ofs->flush();
                    }
                },
                [this]()
                {
                    auto log_path =std::getenv("TLL_LOG_PATH");
                    auto const &file = util::stringFormat("%s/%s.%d.log", log_path ? log_path : TLL_DEFAULT_LOG_PATH, __progname, getpid());
                    LOGD("%s", file.data());
                    return static_cast<void*>(new std::ofstream(file, std::ios::out | std::ios::binary));
                },
                [this](void *&handle)
                {
                    if(handle)
                    {
                        // static_cast<std::ofstream*>(handle)->flush();
                        // static_cast<std::ofstream*>(handle)->close();
                        delete static_cast<std::ofstream*>(handle);
                        handle = nullptr;
                    }
                }
            }}};

    std::thread broadcast_;

    std::condition_variable pop_wait_;
    std::mutex mtx_;
    inline static thread_local auto sSW_ = util::StreamWrapper<>{0x1000};
    // static thread_local std::vector<char> sLogBuffer;

public:
    Manager()
    {
        // start_(0);
        start<false>();
    }

    Manager(uint32_t queue_size) :
        task_queue_{queue_size}
    {}

    template < typename ... LogEnts>
    Manager(uint32_t queue_size,
           LogEnts ...ents) :
        task_queue_{queue_size}
    {
        add_(ents...);
    }

    ~Manager()
    {
        stop();
    }

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    template <Mode mode, typename... Args>
    void log(Severity severity, const char *file, const char *function, int line, const char *format, Args ...args)
    {
        const auto ts = std::chrono::steady_clock::now();
        const int tid = tll::util::tid_nice();
        const int level = tll::log::context::level;

        if(mode == Mode::kAsync && isRunning())
        {
            auto preformat = [=]() -> std::string {
                    return util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
                            kLogSeverityString[(int)severity],
                            util::timestamp(ts), tid, level,
                            util::stringFormat("{%s}{%s}{%d}", file, function, line),
                            util::stringFormat(format, (args)...));
                };
            while(task_queue_.push( std::move(preformat) ) == 0) pop_wait_.notify_all();
            // assert(ps > 0);
            pop_wait_.notify_all();
        }
        else
        {
            log_<false>(util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
                            kLogSeverityString[(int)severity],
                            util::timestamp(ts), tid, level,
                            util::stringFormat("{%s}{%s}{%d}", file, function, line),
                            util::stringFormat(format, (args)...)));
        }
    }

    template <bool is_raw>
    void start(uint32_t period_us=(uint32_t)1e6)
    {
        static_assert(!is_raw, "not supported raw mode yet.");
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;

        auto &ents = is_raw ? raw_ents_ : pre_ents_;
        for(auto &entry : ents)
        {
            auto &ent = entry.second;
            if(ent.handle == nullptr && ent.onStart)
            {
                ent.handle = ent.onStart();
            }
        }

        {
            /// send first log to notify time_since_epoch
            auto sys_now = std::chrono::system_clock::now();
            auto std_now = std::chrono::steady_clock::now();
            auto sys_time = std::chrono::system_clock::to_time_t(sys_now);
            auto first_log = util::stringFormat("%s{%.9f}{%.9f}\n",
                std::ctime(&sys_time),
                std::chrono::duration_cast< std::chrono::duration<double> >(std_now.time_since_epoch()).count(),
                std::chrono::duration_cast< std::chrono::duration<double> >(sys_now.time_since_epoch()).count());
            log_<is_raw>(first_log);
        }

        broadcast_ = std::thread([this, period_us]()
        {
            size_t rs = 0;
            auto do_pop = getPopCallback_<is_raw>();
            auto &fifo = getFifo_<is_raw>();

            auto wait_cb = [this, &fifo]
                {
                    return !fifo.empty()
                            || !isRunning();
                };

            while(isRunning())
            {
                if(fifo.empty())
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::microseconds(period_us), wait_cb);
                }

                while(!fifo.empty())
                {
                    rs = fifo.pop_cb(do_pop, is_raw ? -1 : 1);
                }

            } /// while(isRunning())

            if(!fifo.empty())
            {
                rs = fifo.pop_cb(do_pop, -1);
            }

        });
    }

    inline void stop()
    {
        bool running = true;
        if(!is_running_.compare_exchange_strong(running, false)) return;

        {
            std::lock_guard<std::mutex> lock(mtx_);
            pop_wait_.notify_all();
        }

        if(broadcast_.joinable()) broadcast_.join();
        for(auto &ent_entry : pre_ents_)
        {
            auto &ent = ent_entry.second;
            if(ent.onStop)
                ent.onStop(ent.handle);
        }

        for(auto &ent_entry : raw_ents_)
        {
            auto &ent = ent_entry.second;
            if(ent.onStop)
                ent.onStop(ent.handle);
        }
    }

    inline bool isRunning() const
    {
        return is_running_.load(std::memory_order_relaxed);
    }

    template <bool is_raw=false>
    void remove(const std::string &name)
    {
        auto &ents_ = is_raw ? raw_ents_ : pre_ents_;
        auto it = ents_.find(name);
        if(it != ents_.end())
            ents_.erase(it);
    }

    template <bool is_raw=false, typename ... LogEnts>
    void add(LogEnts ...ents)
    {
        if(isRunning())
            return;

        add_(is_raw, ents...);
    }

    /// wait for ring_queue is empty
    inline bool flush()
    {
        return flush_request_.exchange(true, std::memory_order_relaxed);
    }

private:

    template <bool is_raw> auto &getFifo_()
    {
        if constexpr (is_raw) return ring_buffer_;
        else return task_queue_;
    }

    template <bool is_raw> auto getPopCallback_()
    {
        if constexpr (is_raw) return [this](const char *el, size_t sz)
                {
                    this->log_<true>(el, sz);
                };
        else return [this](std::function<std::string()> *el, size_t sz)
                {
                    std::string payload = (*el)();
                    this->log_<false>(payload);
                };
    }

    template <bool is_raw, typename C>
    inline void log_(const C &buff)
    {
        log_<is_raw>(buff.data(), buff.size());
    }

    template <bool is_raw>
    inline void log_(const char *buff, size_t size)
    {
        total_size += size;
        bool flush_request = flush_request_.exchange(false, std::memory_order_relaxed);
        auto &ents_ = is_raw ? raw_ents_ : pre_ents_;
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            ent.onLog(ent.handle, flush_request, buff, size);
        }
    }

    template <bool is_raw, typename ... LogEnts>
    void add_(Entity ent, LogEnts ...ents)
    {
        auto &ents_ = is_raw ? raw_ents_ : pre_ents_;
        ents_[ent.name] = ent;
        add_<is_raw>(ents...);
    }

    template <bool is_raw>
    inline void add_(Entity ent)
    {
        auto &ents_ = is_raw ? raw_ents_ : pre_ents_;
        ents_[ent.name] = ent;
    }
};

} /// tll::log
