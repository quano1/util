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
    (plog)->log2<Mode::kAsync>((tll::log::Severity)severity, \
                                __FILE__, __FUNCTION__, __LINE__, \
                                ##__VA_ARGS__)

#define TLL_GLOG(severity, ...) TLL_LOG(&tll::log::Manager::instance(), severity, ##__VA_ARGS__)


#define TLL_GLOGD2(...) TLL_GLOG((tll::log::Severity::kDebug), ##__VA_ARGS__)
#define TLL_GLOGI2(...) TLL_GLOG((tll::log::Severity::kInfo), ##__VA_ARGS__)
#define TLL_GLOGW2(...) TLL_GLOG((tll::log::Severity::kWarn), ##__VA_ARGS__)
#define TLL_GLOGF2(...) TLL_GLOG((tll::log::Severity::kFatal), ##__VA_ARGS__)
#define TLL_GLOGT2(ID) \
    tll::log::Tracer COMBINE(tracer, __LINE__)(ID, \
    std::bind(&tll::log::Manager::log2<Mode::kAsync, Tracer *, const std::string &, double>, &tll::log::Manager::instance(), (tll::log::Severity::kTrace), __FILE__, __FUNCTION__, __LINE__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))

#define TLL_GLOGTF2()   TLL_GLOGT2(__FUNCTION__)

    // tll::log::Tracer COMBINE(tracer__, __LINE__)(__FUNCTION__, \
    // std::bind(&tll::log::Manager::log2<Mode::kAsync, Tracer *, const std::string &, double>, &tll::log::Manager::instance(), (tll::log::Severity::kTrace), __FILE__, __FUNCTION__, __LINE__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))

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

struct StaticLogInfo
{
    int severity=0;
    std::string format;
    std::string file;
    std::string function;
    int line=0;
}; /// StaticLogInfo

class Manager
{
public:
    size_t total_size = 0, total_count = 0;
private:
    std::atomic<bool> is_running_{false}, flush_request_{false};
    // lf::CCFIFO<Message> ccq_{0x1000};
    lf::ring_queue_ds<std::function<std::string()>> task_queue_{1000000};
    lf::ring_buffer_mpsc<char> ring_buffer_{0x100000 * 16}; /// 16Mb

    std::unordered_map<std::string, Entity> ents_ = {{"file", Entity{
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
                        static_cast<std::ofstream*>(handle)->flush();
                        static_cast<std::ofstream*>(handle)->close();
                        delete static_cast<std::ofstream*>(handle);
                        handle = nullptr;
                    }
                }
            }}};

    std::thread broadcast_;

    std::condition_variable pop_wait_;
    std::mutex mtx_;

public:
    Manager()
    {
        // start_(0);
        start();
    }

    Manager(uint32_t queue_size) :
        ring_buffer_{queue_size}
    {}

    template < typename ... LogEnts>
    Manager(uint32_t queue_size,
           LogEnts ...ents) :
        ring_buffer_{queue_size}
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
    void log(Severity severity, const char *format, Args ...args)
    {
        std::string payload = util::stringFormat("{%c}%s", kLogSeverityString[(int)severity], util::stringFormat(format, std::forward<Args>(args)...));

        if(mode == Mode::kAsync && isRunning())
        {
            size_t ps = ring_buffer_.push_cb([](char *el, size_t sz, const std::string &msg) {
                memcpy(el, msg.data(), sz);
            }, payload.size(), payload);

            assert(ps > 0);
            pop_wait_.notify_all();
        }
        else
        {
            log_(payload);
        }
    }

    template <typename... Args>
    void log(Severity severity, const char *format, Args ...args)
    {
        // std::string payload = util::stringFormat(format, std::forward<Args>(args)...);
        log<Mode::kAsync>(severity, format, args...);
    }


    template <Mode mode, typename... Args>
    void log2(Severity severity, const char *file, const char *function, int line, const char *format, Args ...args)
    {
        const auto ts = std::chrono::steady_clock::now();
        const int tid = tll::util::tid_nice();
        const int level = tll::log::context::level;

        if(mode == Mode::kAsync && isRunning())
        {
            auto preprocess = [=]() -> std::string {
                    return util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
                            kLogSeverityString[(int)severity],
                            util::timestamp(ts), tid, level,
                            util::stringFormat("{%s}{%s}{%d}", file, function, line),
                            util::stringFormat(format, (args)...));
                };
            while(task_queue_.push( std::move(preprocess) ) == 0) pop_wait_.notify_all();
            // assert(ps > 0);
        }
        else
        {
            log_(util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
                            kLogSeverityString[(int)severity],
                            util::timestamp(ts), tid, level,
                            util::stringFormat("{%s}{%s}{%d}", file, function, line), 
                            util::stringFormat(format, (args)...)));
        }

        pop_wait_.notify_all();
    }

    // template <Mode mode, typename... Args>
    // void log3(const StaticLogInfo &info, Args ...args)
    // {
    //     const auto timestamp = std::chrono::steady_clock::now();
    //     const int tid = tll::util::tid_nice();
    //     const int level = tll::log::context::level;

    //     if(mode == Mode::kAsync && isRunning())
    //     {
    //         auto preprocess = [=, &info]() -> std::string {
    //                 return util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
    //                         kLogSeverityString[(int)info.severity],
    //                         util::timestamp(timestamp), tid, level,
    //                         util::stringFormat("{%s}{%s}{%d}", info.file, info.function, info.line),
    //                         util::stringFormat(info.format.data(), (args)...));
    //             };
    //         while(task_queue_.push( std::move(preprocess) ) == 0);
    //         // assert(ps > 0);
    //     }
    //     else
    //     {
    //         log_(util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
    //                         kLogSeverityString[(int)info.severity],
    //                         util::timestamp(timestamp), tid, level,
    //                         util::stringFormat("{%s}{%s}{%d}", info.file, info.function, info.line), 
    //                         util::stringFormat(info.format.data(), (args)...)));
    //     }
    // }

    inline void start(size_t chunk_size = 0x400 * 16, uint32_t period_us=(uint32_t)1e6)
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;

        for(auto &entry : ents_)
        {
            // entry.second.start();
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
            log_(first_log);
        }

        start2(period_us);
    }

    inline void start2(uint32_t period_us=(uint32_t)1e6)
    {
        // bool val = false;
        // if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
        //     return;

        broadcast_ = std::thread([this, period_us]()
        {
            util::Counter<1, std::chrono::duration<uint32_t, std::micro>> counter_us;
            uint32_t delta = 0;

            while(isRunning())
            {
                size_t rs = 0;
                counter_us.start();
                if(delta < period_us && task_queue_.empty())
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    /// wait timeout
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::microseconds(period_us - delta), 
                        [this, &delta, &counter_us]
                        {
                            delta += counter_us.elapse().count();
                            return !isRunning() || !task_queue_.empty();
                        });
                }

                while(!task_queue_.empty())
                {
                    rs = task_queue_.pop_cb([this](std::function<std::string()> *el, size_t sz){
                        std::string payload = (*el)();
                        this->log_(payload);
                    }, 0x100);
                    total_count += rs;
                    // LOGD("%ld", rs);
                }

                if(rs)
                {
                    delta = 0;
                    counter_us.start();
                }
                else if(delta > period_us)
                {
                    delta -= period_us;
                }

            } /// while(isRunning())

            if(!task_queue_.empty())
            {
                size_t rs = task_queue_.pop_cb([this](std::function<std::string()> *el, size_t sz){
                    total_count ++;
                    this->log_((*el)());
                }, -1);

                total_count += rs;
            };
        });
    }


    inline void start3(size_t chunk_size = 0x400 * 16, uint32_t period_us=(uint32_t)1e6)
    {
        // bool val = false;
        // if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
        //     return;

        broadcast_ = std::thread([this, chunk_size, period_us]()
        {
            util::Counter<1, std::chrono::duration<uint32_t, std::micro>> counter_us;
            uint32_t delta = 0;
            while(isRunning())
            {
                delta += counter_us.elapse().count();
                counter_us.start();
                if(delta < period_us && ring_buffer_.size() < chunk_size)
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    /// wait timeout
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::milliseconds(period_us - delta), [this, chunk_size, &delta, &counter_us] {
                        delta += counter_us.elapse().count();
                        return !isRunning() || (this->ring_buffer_.size() >= chunk_size);
                    });
                }
                if(delta >= period_us || ring_buffer_.size() >= chunk_size)
                {
                    delta -= period_us;
                    size_t rs = ring_buffer_.pop_cb([&](const char *el, size_t sz){
                        log_(el, sz);
                    }, -1);
                }
            } /// while(isRunning())

            if(!ring_buffer_.empty())
            {
                size_t rs = ring_buffer_.pop_cb([&](const char *el, size_t sz){ log_(el, sz); }, -1);
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
        for(auto &ent_entry : ents_)
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

    void remove(const std::string &name)
    {
        auto it = ents_.find(name);
        if(it != ents_.end())
            ents_.erase(it);
    }

    template < typename ... LogEnts>
    void add(LogEnts ...ents)
    {
        if(isRunning())
            return;

        add_(ents...);
    }

    static Manager &instance()
    {
        static std::atomic<Manager*> singleton{nullptr};
        static std::atomic<bool> init{false};
        bool tmp = false;
        auto obj = singleton.load(std::memory_order_acquire);

        if (obj)
            return *obj;

        if(!init.compare_exchange_strong(tmp, true))
        {
            while(!singleton.load(std::memory_order_relaxed)){}
        }
        else
        {
            singleton.store(new Manager{}, std::memory_order_release);
        }

        return *singleton.load(std::memory_order_acquire);
    }

    /// wait for ring_queue is empty
    inline bool flush()
    {
        // flush_request_.store(true, std::memory_order_relaxed);
        return flush_request_.exchange(true, std::memory_order_relaxed);
    }

private:

    inline void log_(const std::string &buff)
    {
        log_(buff.data(), buff.size());
    }

    inline void log_(const char *buff, size_t size)
    {
        total_size += size;
        bool flush_request = flush_request_.exchange(false, std::memory_order_relaxed);
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            ent.onLog(ent.handle, flush_request, buff, size);
        }
    }

    template <typename ... LogEnts>
    void add_(Entity ent, LogEnts ...ents)
    {
        ents_[ent.name] = ent;
        add_(ents...);
    }

    inline void add_(Entity ent)
    {
        ents_[ent.name] = ent;
    }
};

} /// tll::log
