/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

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
#include "counter.h"
#include "lffifo.h"

#ifndef TLL_DEFAULT_LOG_PATH
#define TLL_DEFAULT_LOG_PATH "."
#endif

extern char *__progname;

#define TLL_LOGD(plog, format, ...) (plog)->log((tll::log2::Type::kDebug), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log2::Context::instance().level(),\
  tll::log2::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

// #define TLL_LOGD2(plog, format, ...) (plog)->log((tll::log2::Type::kDebug), "{%.9f}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
//   tll::util::str_tid_nice(),\
//   __FUNCTION__, \
//   __LINE__, \
//   ##__VA_ARGS__)

#define TLL_LOGI(plog, format, ...) (plog)->log((tll::log2::Type::kInfo), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log2::Context::instance().level(),\
  tll::log2::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define TLL_LOGW(plog, format, ...) (plog)->log((tll::log2::Type::kWarn), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log2::Context::instance().level(),\
  tll::log2::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define TLL_LOGF(plog, format, ...) (plog)->log((tll::log2::Type::kFatal), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log2::Context::instance().level(),\
  tll::log2::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

// #define LOG_HEADER_ tll::util::stringFormat("{%s}{%d}", __FUNCTION__, __LINE__)

// #define LOG_HEADER_ (tll::util::stringFormat("{%s}{%d}{%s}{%s}{%d}",\
//   tll::util::str_tid_nice(),\
//   tll::log2::Context::instance().level(),\
//   tll::log2::Context::instance().get(),\
//   __FUNCTION__, \
//   __LINE__))

#define LOG_HEADER_ (tll::util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}{%d}",\
  tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log2::Context::instance().level(),\
  tll::log2::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__))

#define TLL_LOGT(plog, ID) tll::log2::Tracer<> tracer_##ID##__([plog](std::string const &log_msg){(plog)->log((tll::log2::Type::kTrace), "%s", log_msg);}, (tll::log2::Context::instance().push(__FILE__), LOG_HEADER_), #ID)

#define TLL_LOGTF(plog) tll::log2::Tracer<> tracer__([plog](std::string const &log_msg){(plog)->log((tll::log2::Type::kTrace), "%s", log_msg);}, (tll::log2::Context::instance().push(__FILE__), LOG_HEADER_), __FUNCTION__)

#define TLL_GLOGD(...) TLL_LOGD(&tll::log2::Manager::instance(), ##__VA_ARGS__)
// #define TLL_GLOGD2(...) TLL_LOGD2(&tll::log2::Manager::instance(), ##__VA_ARGS__)
#define TLL_GLOGI(...) TLL_LOGI(&tll::log2::Manager::instance(), ##__VA_ARGS__)
#define TLL_GLOGW(...) TLL_LOGW(&tll::log2::Manager::instance(), ##__VA_ARGS__)
#define TLL_GLOGF(...) TLL_LOGF(&tll::log2::Manager::instance(), ##__VA_ARGS__)
#define TLL_GLOGT(ID) tll::log2::Tracer<> tracer_##ID##__([](std::string const &log_msg){tll::log2::Manager::instance().log((tll::log2::Type::kTrace), "%s", log_msg);}, (tll::log2::Context::instance().push(__FILE__), LOG_HEADER_), #ID)

#define TLL_GLOGTF() tll::log2::Tracer<> tracer__(std::bind(&tll::log2::Manager::log<std::string const &>, &tll::log2::Manager::instance(), (tll::log2::Type::kTrace), "%s", std::placeholders::_1), (tll::log2::Context::instance().push(__FILE__), __FUNCTION__), __LINE__, __FUNCTION__)

namespace tll::log2 {

char const kLogTypeString[]="TDIWF";
enum class Type
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
    Type type;
    std::string payload;
};

struct Entity
{
    std::string name;
    std::function<void(void*, const char*, size_t)> onLog;
    std::function<void *()> onStart;
    std::function<void(void *&)> onStop;
    void *handle;

private:
    void start()
    {
        if(handle == nullptr && onStart)
            handle = onStart();
        /// send first log to notify time_since_epoch
        auto std_now = std::chrono::steady_clock::now();
        auto sys_now = std::chrono::system_clock::now();
        auto sys_time = std::chrono::system_clock::to_time_t(sys_now);
        auto buff = util::stringFormat("%s{%.9f}{%.9f}\n", 
                std::ctime(&sys_time),
                std::chrono::duration_cast< std::chrono::duration<double> >(std_now.time_since_epoch()).count(), 
                std::chrono::duration_cast< std::chrono::duration<double> >(sys_now.time_since_epoch()).count());
        onLog(handle, buff.data(), buff.size());
    }

    void log(const char *buff, size_t size) const
    {
        onLog(handle, buff, size);
    }

    void stop()
    {
        if(onStop)
            onStop(handle);
    }

    friend class Manager;
};

struct Context
{
    inline void push(std::string ctx)
    {
        ctxs_.push_back(ctx);
    }

    inline void pop()
    {
        ctxs_.pop_back();
    }

    inline size_t level() const
    {
        return ctxs_.size();
    }

    inline std::string get() const
    {
        std::string ret;

        for(int i=ctxs_.size()-1; i>=0; i--)
        {
            ret += util::stringFormat("%s,", ctxs_[i]);
        }

        return ret;
    }

    inline static Context &instance()
    {
        static thread_local Context instance;
        return instance;
    }

private:
    std::vector<std::string> ctxs_;
}; /// Context

template <typename clock=std::chrono::steady_clock>
struct Tracer
{
    Tracer() = default;
    Tracer(std::string id) : name_(std::move(id))
    {
        printf(" (%.9f)%s\n", util::timestamp<std::chrono::duration<double, std::ratio<1>>, clock>(), name_.data());
    }

    Tracer(std::function<void(std::string const&)> logf, 
           const char *func, int line, std::string id="") : func_(func), line_(line), name_(std::move(id))
    {
        // sig_log.connect(logf);
        doLog_ = std::move(logf);
        doLog_(util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}{%d}{%s}\n", 
                                    tll::util::timestamp<>(),
                                    tll::util::str_tid_nice(),
                                    tll::log2::Context::instance().level(),
                                    tll::log2::Context::instance().get(),
                                    func_,
                                    line_, name_));
    }

    ~Tracer()
    {
        const auto period = counter_.elapse().count();
        if(doLog_)
        {
            doLog_(util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}{%d}{%s}{%.6f(s)}\n",
                                    tll::util::timestamp<>(),
                                    tll::util::str_tid_nice(),
                                    tll::log2::Context::instance().level(),
                                    tll::log2::Context::instance().get(),
                                    func_,
                                    line_,
                                    name_,
                                    period));
            log2::Context::instance().pop();
        }
        else if(!name_.empty())
            printf(" (%.9f)~%s: %.6f (s)\n", util::timestamp<std::chrono::duration<double, std::ratio<1>>, clock>(), name_.data(), period);
    }

    const char *func_ = "";
    int line_ = 0;
    std::string name_ = "";
    time::Counter<> counter_;

    std::function<void(std::string const&)> doLog_;
}; /// Tracer

class Manager
{
public:
    Manager()
    {
        // start_(0);
        start();
    }

    Manager(uint32_t queue_size) :
        rb_{queue_size}
    {}

    template < typename ... LogEnts>
    Manager(uint32_t queue_size,
           LogEnts ...ents) :
        rb_{queue_size}
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
    void log(Type type, const char *format, Args &&...args)
    {
        std::string payload = util::stringFormat(format, std::forward<Args>(args)...);
        log<mode>(Message{type, payload});
    }

    template <typename... Args>
    void log(Type type, const char *format, Args &&...args)
    {
        std::string payload = util::stringFormat(format, std::forward<Args>(args)...);
        log<Mode::kAsync>(Message{type, payload});
    }

    template <Mode mode>
    void log(Message);

    inline void start(size_t chunk_size = 0x400 * 16, uint32_t period_ms=1000) /// 16 Kb, 1000 ms
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;
        for(auto &entry : ents_) entry.second.start();
        broadcast_ = std::thread([this, chunk_size, period_ms]()
        {
            time::Counter<std::chrono::duration<uint32_t, std::ratio<1, 1000>>> counter;
            uint32_t delta = 0;
            while(isRunning())
            {
                delta += counter.elapse().count();
                counter.start();
                if(delta < period_ms && rb_.size() < chunk_size)
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    /// wait timeout
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::milliseconds(period_ms - delta), [this, chunk_size, &delta, &counter] {
                        delta += counter.elapse().count();
                        return !isRunning() || (this->rb_.size() >= chunk_size);
                    });
                }
                if(delta >= period_ms || rb_.size() >= chunk_size)
                {
                    delta -= period_ms;
                    size_t rs = rb_.pop([&](const char *el, size_t sz){
                        log_(el, sz);
                    }, -1);
                }
            } /// while(isRunning())
            if(!rb_.empty())
            {
                size_t rs = rb_.pop([&](const char *el, size_t sz){ log_(el, sz); }, -1);
            }
        });
    }

    inline void stop()
    {
        if(!isRunning()) return;
        
        is_running_.store(false); // write release
        {
            std::lock_guard<std::mutex> lock(mtx_);
            pop_wait_.notify_one();
        }
        if(broadcast_.joinable()) broadcast_.join();
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            ent.stop();
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

        if (singleton.load(std::memory_order_relaxed))
            return *singleton.load(std::memory_order_acquire);

        if(!init.compare_exchange_strong(tmp, true))
        {
            while(!singleton.load(std::memory_order_relaxed)){}
        }
        else
        {
            singleton.store(new Manager{}, std::memory_order_release);
            // singleton->init_(0);
        }

        return *singleton.load(std::memory_order_acquire);
    }

    /// wait for ring_queue is empty
    inline void wait()
    {
        std::mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        join_wait_.wait(lock, [this] 
        {
            pop_wait_.notify_one();
            return !isRunning() || rb_.empty();
        });
    }

private:

    // inline void log_(Flag flag, const std::vector<char> &buff) const
    // {
    //     for(auto &ent_entry : ents_)
    //     {
    //         auto &ent = ent_entry.second;
    //         if(ent.flag == flag)
    //             ent.log(buff.data(), buff.size());
    //     }
    // }

    // inline void log_(Flag flag, const char *buff, size_t size) const
    // {
    //     for(auto &ent_entry : ents_)
    //     {
    //         auto &ent = ent_entry.second;
    //         if(ent.flag == flag)
    //             ent.log(buff, size);
    //     }
    // }

    // inline void log_(Type type, const std::string &payload) const
    // {
    //     for( auto &entry : ents_)
    //     {
    //         auto &ent = entry.second;
    //         if((uint32_t)ent.flag & (uint32_t)toFlag(type))
    //         {
    //             ent.log(payload.data(), payload.size());
    //         }
    //     }
    // }

    inline void log_(const std::string &buff) const
    {
        log_(buff.data(), buff.size());
    }

    inline void log_(const char *buff, size_t size) const
    {
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            ent.log(buff, size);
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

    std::atomic<bool> is_running_{false};
    // lf::CCFIFO<Message> ccq_{0x1000};
    lf::ring_buffer_mpmc<char> rb_{0x100000 * 16}; /// 16Mb
    std::unordered_map<std::string, Entity> ents_ = {{"file", Entity{
                .name = "file",
                [this](void *handle, const char *buff, size_t size)
                {
                    if(handle == nullptr)
                    {
                        printf("%.*s", (int)size, buff);
                        return;
                    }
                    static_cast<std::ofstream*>(handle)->write((const char *)buff, size);
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
                    static_cast<std::ofstream*>(handle)->flush();
                    static_cast<std::ofstream*>(handle)->close();
                    delete static_cast<std::ofstream*>(handle);
                    handle = nullptr;
                }
            }}};

    std::thread broadcast_;

    std::condition_variable pop_wait_, join_wait_;
    std::mutex mtx_;
};

template <>
inline void Manager::log<Mode::kSync>(Message msg)
{
    /// TODO add pre-format
    std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(int)msg.type], msg.payload);

    log_(payload);
}

template <>
inline void Manager::log<Mode::kAsync>(Message msg)
{
    if(!isRunning())
    {
        log<Mode::kSync>(msg);
        /// TODO: force push?
    }
    else
    {
        std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(int)msg.type], msg.payload);
        size_t ps = rb_.push([](char *el, size_t sz, const std::string &msg) {
            memcpy(el, msg.data(), sz);
        }, payload.size(), payload);
        assert(ps > 0);
        pop_wait_.notify_one();
    }
}

} /// tll::log2
