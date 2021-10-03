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

#define TLL_LOG2(plog, severity, ...) \
    (plog)->log2<Mode::kAsync>((tll::log::Severity)severity, \
                                __FILE__, __FUNCTION__, __LINE__, \
                                ##__VA_ARGS__)

#define TLL_GLOG2(severity, ...) TLL_LOG2(&tll::log::Manager::instance(), severity, ##__VA_ARGS__)

#define TLL_GLOGD(...) \
    (&tll::log::Manager::instance())->log<Mode::kAsync>(tll::log::Severity::kDebug, \
        __FILE__, __FUNCTION__, __LINE__, \
        ##__VA_ARGS__)

#define TLL_GLOGD2(...) TLL_GLOG2((tll::log::Severity::kDebug), ##__VA_ARGS__)
#define TLL_GLOGI2(...) TLL_GLOG2((tll::log::Severity::kInfo), ##__VA_ARGS__)
#define TLL_GLOGW2(...) TLL_GLOG2((tll::log::Severity::kWarn), ##__VA_ARGS__)
#define TLL_GLOGF2(...) TLL_GLOG2((tll::log::Severity::kFatal), ##__VA_ARGS__)
#define TLL_GLOGT2(ID) \
    tll::log::Tracer COMBINE(tracer, __LINE__)(ID, \
    std::bind(&tll::log::Manager::log2<Mode::kAsync, Tracer *, const std::string &, double>, &tll::log::Manager::instance(), (tll::log::Severity::kTrace), __FILE__, __FUNCTION__, __LINE__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))

#define TLL_GLOGTF2()   TLL_GLOGT2(__FUNCTION__)


#define TLL_LOG3(plog, severity, fmt, ...) \
    do \
    { \
        static const StaticLogInfo sInfo { \
            fmt, \
            __FILE__, \
            __FUNCTION__, \
            (int)severity, \
            __LINE__ \
        }; \
        (plog)->log3<Mode::kAsync>(sInfo, \
                                ##__VA_ARGS__); \
    } while(0)

#define TLL_GLOG3(severity, ...) TLL_LOG3(&tll::log::Manager::instance(), severity, ##__VA_ARGS__)

#define TLL_GLOGD(...) \
    (&tll::log::Manager::instance())->log<Mode::kAsync>(tll::log::Severity::kDebug, \
        __FILE__, __FUNCTION__, __LINE__, \
        ##__VA_ARGS__)

#define TLL_GLOGD3(...) TLL_GLOG3((tll::log::Severity::kDebug), ##__VA_ARGS__)

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
    const char *format;
    const char *file;
    const char *func;
    const int severity;
    const int line;
}; /// StaticLogInfo

class Manager : public util::SingletonBase<Manager>
{
public:
    size_t total_size = 0, total_count = 0;
private:
    std::atomic<bool> is_running_{false}, flush_request_{false};
    // lf::CCFIFO<Message> ccq_{0x1000};
    lf::ring_queue_ds<std::function<std::string()>> task_queue_{100000};
    lf::ring_buffer_ds<char> ring_buffer_{0x100000 * 1}; /// 1Mb

    std::unordered_map<std::string, Entity> raw_ents_ = {{"file", Entity{
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
            }}};

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
        start2();
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


    template <Mode mode, size_t N, typename... Args>
    void log(Severity severity, const char *file, const char *function, int line, const char (&format)[N], Args ...args)
    {
        // LOGD("%s", format);
        log(0, args...);
    }

    template <typename... Args>
    void log(int id, Args ...args)
    {
        // static std::vector<char> payload(4096);
        // static util::StreamWrapper stream(nullptr);

        // stream.reset(payload.data());
        sSW_.reset();
        sSW_.writeArg(args...);
        if(isRunning())
        {

            while(ring_buffer_.push(sSW_.buffer_, sSW_.size_) == 0) 
            {
                pop_wait_.notify_all();
            }

            pop_wait_.notify_all();
            // LOGD("%ld", stream.size);
        }
        else
        {
            // std::string payload = util::stringFormat("{%c}%s", kLogSeverityString[(int)severity], util::stringFormat(format, std::forward<Args>(args)...));
            log_<true>(sSW_.buffer_, sSW_.size_);
        }
    }

    // template <typename... Args>
    // void log(Severity severity, const char *format, Args ...args)
    // {
    //     // std::string payload = util::stringFormat(format, std::forward<Args>(args)...);
    //     log<Mode::kAsync>(severity, format, args...);
    // }


    template <Mode mode, typename... Args>
    void log2(Severity severity, const char *file, const char *function, int line, const char *format, Args ...args)
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

        // pop_wait_.notify_all();
    }

    template <Mode mode, typename... Args>
    void log3(const StaticLogInfo &info_data, Args ...args)
    {
        const auto ts = util::timestamp();
        const int tid = tll::util::tid_nice();
        const int level = tll::log::context::level;
        sSW_.reset();
        sSW_ << 1
            << ts << tid << level << (... << args);
        if(mode == Mode::kAsync && isRunning())
        {
            // (sSW_ << args)...;
            // std::string payload = util::stringFormat("{%c}%s", kLogSeverityString[(int)severity], util::stringFormat(format, std::forward<Args>(args)...));
            // util::StreamBuffer sSW_;
            // size_t ps = ring_buffer_.push_cb([](char *el, size_t sz, const std::string &msg) {
                // memcpy(el, msg.data(), sz);
            // }, sSW_.size(), sSW_);
            // size_t ps = ring_buffer_.push(sSW_.buffer_, sSW_.size_);
            while(ring_buffer_.push(sSW_.buffer_, sSW_.size_) == 0) 
            {
                pop_wait_.notify_all();
            }

            // assert(ps > 0);
            pop_wait_.notify_all();
        }
        else
        {
            // log_(util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
            //                 kLogSeverityString[(int)info.severity],
            //                 ts, tid, level,
            //                 util::stringFormat("{%s}{%s}{%d}", info.file, info.func, info.line), 
            //                 util::stringFormat(info.format, (args)...)));
            log_<true>(sSW_.buffer_, sSW_.size_);
        }
    }

    inline void start(uint32_t period_us=(uint32_t)1e6)
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;

        for(auto &entry : raw_ents_)
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
            log_<true>(first_log);
        }

        start_(period_us);
    }

    inline void start_(uint32_t period_us=(uint32_t)1e6)
    {
        broadcast_ = std::thread([this, period_us]()
        {
            size_t rs = 0;

            while(isRunning())
            {
                if(ring_buffer_.empty())
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::microseconds(period_us), 
                        [this]
                        {
                            return !ring_buffer_.empty()
                                    || !isRunning();
                        });
                }

                while(!ring_buffer_.empty())
                {
                    rs = ring_buffer_.pop_cb([&](const char *el, size_t sz){ log_<true>(el, sz); }, -1);
                }

            } /// while(isRunning())

            if(!ring_buffer_.empty())
            {
                rs = ring_buffer_.pop_cb([&](const char *el, size_t sz){ log_<true>(el, sz); }, -1);
            };
        });
    }


    inline void start2(uint32_t period_us=(uint32_t)1e6)
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;

        for(auto &entry : pre_ents_)
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
            log_<false>(first_log);
        }

        start2_(period_us);
    }

    inline void start2_(uint32_t period_us=(uint32_t)1e6)
    {
        broadcast_ = std::thread([this, period_us]()
        {
            size_t rs = 0;

            while(isRunning())
            {
                if(task_queue_.empty())
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::microseconds(period_us), 
                        [this]
                        {
                            return !task_queue_.empty()
                                    || !isRunning();
                        });
                }

                while(!task_queue_.empty())
                {
                    rs = task_queue_.pop_cb([this](std::function<std::string()> *el, size_t sz){
                        std::string payload = (*el)();
                        this->log_<false>(payload);
                    }, 0x100);
                    total_count += rs;
                }

            } /// while(isRunning())

            if(!task_queue_.empty())
            {
                rs = task_queue_.pop_cb([this](std::function<std::string()> *el, size_t sz){
                    this->log_<false>((*el)());
                }, -1);
                total_count += rs;
            };
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
        // flush_request_.store(true, std::memory_order_relaxed);
        return flush_request_.exchange(true, std::memory_order_relaxed);
    }

private:

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
