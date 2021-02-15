/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <string>
#include <cassert>
#include <cstring>

#include <vector>
#include <unordered_map>

#include <chrono>
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
#include "contiguouscircular.h"

#ifndef TLL_DEFAULT_LOG_PATH
#define TLL_DEFAULT_LOG_PATH "."
#endif

extern char *__progname;

#define TLL_LOGD(plog, format, ...) (plog)->log((tll::log::Type::kDebug), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define TLL_LOGI(plog, format, ...) (plog)->log((tll::log::Type::kInfo), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define TLL_LOGW(plog, format, ...) (plog)->log((tll::log::Type::kWarn), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define TLL_LOGF(plog, format, ...) (plog)->log((tll::log::Type::kFatal), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define LOG_HEADER_ (tll::util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}{%d}",\
  tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__))

#define TLL_LOGT(plog, ID) tll::log::Tracer<> tracer_##ID##__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::Context::instance().push(__FILE__), LOG_HEADER_), #ID)

#define TLL_LOGTF(plog) tll::log::Tracer<> tracer__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::Context::instance().push(__FILE__), LOG_HEADER_), __FUNCTION__)

#define TLL_GLOGD(...) TLL_LOGD(&tll::log::Manager::instance(), ##__VA_ARGS__)
#define TLL_GLOGI(...) TLL_LOGI(&tll::log::Manager::instance(), ##__VA_ARGS__)
#define TLL_GLOGW(...) TLL_LOGW(&tll::log::Manager::instance(), ##__VA_ARGS__)
#define TLL_GLOGF(...) TLL_LOGF(&tll::log::Manager::instance(), ##__VA_ARGS__)
#define TLL_GLOGT(ID) tll::log::Tracer<> tracer_##ID##__([](std::string const &log_msg){tll::log::Manager::instance().log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::Context::instance().push(__FILE__), LOG_HEADER_), #ID)

#define TLL_GLOGTF() tll::log::Tracer<> tracer__(std::bind(&tll::log::Manager::log<std::string const &>, &tll::log::Manager::instance(), (tll::log::Type::kTrace), "%s", std::placeholders::_1), (tll::log::Context::instance().push(__FILE__), LOG_HEADER_), __FUNCTION__)

namespace tll{ namespace log{

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

enum class Flag : uint32_t
{
    kTrace = (1U << (int)Type::kTrace),
    kDebug = (1U << (int)Type::kDebug),
    kInfo = (1U << (int)Type::kInfo),
    kWarn = (1U << (int)Type::kWarn),
    kFatal = (1U << (int)Type::kFatal),
    kAll = kTrace | kDebug | kInfo | kWarn | kFatal,
};

enum class Mode
{
    kSync = 0,
    kAsync,
};

inline constexpr Flag toFlag(Type type)
{
    return static_cast<Flag>(1U << static_cast<int>(type));
}

template <typename... Args>
constexpr Flag toFlag(Type type, Args ...args)
{
    return toFlag(type) | toFlag(args...);
}

struct Message
{
    Type type;
    std::string payload;
};

struct Entity
{
    std::string name;
    Flag flag;
    std::function<void(void*, const char*, size_t)> onLog;
    std::function<void *()> onStart;
    std::function<void(void *&)> onStop;
    void *handle;

private:
    void start()
    {
        if(handle == nullptr && onStart)
            handle = onStart();
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
    Tracer(std::string id) : name(std::move(id))
    {
        printf(" (%.9f)%s\n", util::timestamp<double, std::ratio<1,1>, clock>(), name.data());
    }

    Tracer(std::function<void(std::string const&)> logf, std::string header, std::string id="") : name(std::move(id))
    {
        // sig_log.connect(logf);
        doLog = std::move(logf);
        doLog(util::stringFormat("%s{%s}\n", header, name));
    }

    ~Tracer()
    {
        const auto period = counter.elapse().count();
        if(doLog)
        {
            doLog(util::stringFormat("%s{%s}{%.6f(s)}\n",
                                      LOG_HEADER_,
                                      name,
                                      period));
            log::Context::instance().pop();
        }
        else if(!name.empty())
            printf(" (%.9f)~%s: %.6f (s)\n", util::timestamp<double, std::ratio<1,1>, clock>(), name.data(), period);
    }

    std::string name = "";
    time::Counter<> counter;

    std::function<void(std::string const&)> doLog;
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
        ccq_{queue_size}
    {}

    template < typename ... LogEnts>
    Manager(uint32_t queue_size,
           LogEnts ...ents) :
        ccq_{queue_size}
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

    inline void start(size_t chunk_size = 0x400, uint32_t period_ms=1000) /// 1 Kb, 1000 ms
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;
        broadcast_ = std::thread([this, chunk_size, period_ms]()
        {
            time::Counter<> counter;
            auto buff_list = init_(chunk_size);
            uint32_t total_delta = 0;
            std::vector<char> buff (chunk_size);

            cc::Callback onPopBatch{[this, &buff_list](uint32_t index, uint32_t size)
            {
                for(uint32_t i=0; i<size; i++)
                {
                    Message &log_msg = *ccq_.elemAt(index + i);
                    for(auto &entry : buff_list)
                    {
                        auto &flag = entry.first;
                        auto &ccb = entry.second;
                        if((uint32_t)flag & (uint32_t)toFlag(log_msg.type))
                        {
                            std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(uint32_t)(log_msg.type)], log_msg.payload);
                            ccb.push([&payload, &ccb](size_t i, size_t s){
                                std::memcpy(ccb.elemAt(i), payload.data(), s);
                            }, payload.size());
                        }
                    }
                }
            }};
            while(isRunning())
            {
                uint32_t delta = counter.elapse().count();
                counter.start();
                total_delta += delta;

                if(total_delta >= period_ms)
                {
                    total_delta -= period_ms;
                    /// flushing the buffer list
                    for(auto &buff_entry : buff_list)
                    {
                        auto &flag = buff_entry.first;
                        auto &ccb = buff_entry.second;
                        size_t cs = chunk_size;
                        if(ccb.pop([&buff, &ccb](size_t i, size_t s)
                                   { std::memcpy(buff.data(), ccb.elemAt(i), s); }, cs))
                        {
                            buff.resize(cs);
                            log_(flag, buff);
                            // buff.resize(0);
                        }
                    }
                }

                if(ccq_.empty())
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    /// wait timeout
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::milliseconds(period_ms - total_delta), [this] {
                        return !isRunning() || !this->ccq_.empty();
                    });
                    /// wait timeout or running == false
                    if( !wait_status || !isRunning())
                    {
                        continue;
                    }
                }
                ccq_.pop(onPopBatch, ccq_.capacity());
                for(auto &entry : buff_list)
                {
                    auto &flag = entry.first;
                    auto &ccb = entry.second;
                    while(ccb.size() >= chunk_size)
                    {
                        buff.resize(chunk_size);
                        size_t ps = ccb.pop([&ccb, &buff](size_t i, size_t s){std::memcpy(buff.data(), ccb.elemAt(i), s);}, chunk_size);
                        if(ps > 0)
                        {
                            buff.resize(ps);
                            log_(flag, buff);
                        }
                    }
                }
            }

            ccq_.pop(onPopBatch, ccq_.capacity());

            for(auto &entry : buff_list)
            {
                auto &flag = entry.first;
                auto &ccb = entry.second;
                while(!ccb.empty())
                {
                    buff.resize(chunk_size);
                    size_t ps = ccb.pop([&ccb, &buff](size_t i, size_t s){std::memcpy(buff.data(), ccb.elemAt(i), s);}, chunk_size);
                    if(ps)
                    {
                        buff.resize(ps);
                        log_(flag, buff);
                    }
                }
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
            return !isRunning() || ccq_.empty();
        });
    }

private:

    inline void log_(Flag flag, const std::vector<char> &buff) const
    {
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            if(ent.flag == flag)
                ent.log(buff.data(), buff.size());
        }
    }

    inline void log_(Flag flag, const char *buff, size_t size) const
    {
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            if(ent.flag == flag)
                ent.log(buff, size);
        }
    }

    inline void log_(Type type, const std::string &payload) const
    {
        for( auto &entry : ents_)
        {
            auto &ent = entry.second;
            if((uint32_t)ent.flag & (uint32_t)toFlag(type))
            {
                ent.log(payload.data(), payload.size());
            }
        }
    }

    // inline std::unordered_map<Flag, std::vector<char>> init_(uint32_t chunk_size)
    inline std::unordered_map<Flag, lf::CCFIFO<char>> init_(uint32_t chunk_size)
    {
        std::unordered_map<Flag, lf::CCFIFO<char>> buff_list;
        for(auto &entry : ents_)
        {
            auto &ent = entry.second;
            auto flag = ent.flag;
            ent.start();
            if(chunk_size > 0)
            {
                buff_list[flag].reserve(chunk_size * 0x400);
            }
        }
        return buff_list;
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
    lf::CCFIFO<Message> ccq_{0x1000};
    std::unordered_map<std::string, Entity> ents_ = {{"file", Entity{
                .name = "file", .flag = tll::log::Flag::kAll,
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

    log_(msg.type, payload);
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
        bool rs = ccq_.push([this, &msg](size_t index, size_t) {
            *(ccq_.elemAt(index)) = std::move(msg);
        });
        assert(rs);
        pop_wait_.notify_one();
    }
}

}} /// tll::log
