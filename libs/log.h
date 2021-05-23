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

#define TLL_LOGD(plog, format, ...) (plog)->log((tll::log::Type::kDebug), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define TLL_LOGI(plog, format, ...) (plog)->log((tll::log::Type::kInfo), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define TLL_LOGW(plog, format, ...) (plog)->log((tll::log::Type::kWarn), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define TLL_LOGF(plog, format, ...) (plog)->log((tll::log::Type::kFatal), "{%.9f}{%s}{%d}{%s}{%s}{%d}{" format "}\n", tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
  tll::log::Context::instance().level(),\
  tll::log::Context::instance().get(),\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)

#define LOG_HEADER_ (tll::util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}{%d}",\
  tll::util::timestamp<>(),\
  tll::util::str_tid_nice(),\
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

#define TLL_GLOGTF() tll::log::Tracer<> tracer__(std::bind(&tll::log::Manager::log<std::string const &>, &tll::log::Manager::instance(), (tll::log::Type::kTrace), "%s", std::placeholders::_1), (tll::log::Context::instance().push(__FILE__), __FUNCTION__), __LINE__, __FUNCTION__)


#define TLL_LOGD2(plog, ...) (plog)->log2<Mode::kAsync>((tll::log::Type::kDebug), \
  __FILE__,\
  __FUNCTION__, \
  __LINE__, \
  ##__VA_ARGS__)
#define TLL_GLOGD2(...) TLL_LOGD2(&tll::log::Manager::instance(), ##__VA_ARGS__)

namespace tll::log {

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
    // friend class Manager;
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

    inline int level() const
    {
        return (int)ctxs_.size();
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
                                    tll::log::Context::instance().level(),
                                    tll::log::Context::instance().get(),
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
                                    tll::log::Context::instance().level(),
                                    tll::log::Context::instance().get(),
                                    func_,
                                    line_,
                                    name_,
                                    period));
            log::Context::instance().pop();
        }
        else if(!name_.empty())
            printf(" (%.9f)~%s: %.6f (s)\n", util::timestamp<std::chrono::duration<double, std::ratio<1>>, clock>(), name_.data(), period);
    }

    const char *func_ = "";
    int line_ = 0;
    std::string name_ = "";
    util::Counter<> counter_;

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
    void log2(Type type, const char *file, const char *function, int line, const char *format, Args ...args)
    {
        const auto timestamp = std::chrono::steady_clock::now();
        // const std::chrono::steady_clock::time_point timestamp;
        const int tid = tll::util::tid_nice();
        // const int tid = 0;
        const int level = tll::log::Context::instance().level();
        // const int level = 1;

        static const Type stype = type;
        static const std::string sffl = util::stringFormat("{%s}{%s}{%d}", file, function, line);
        static const char *sformat = format;

        // static auto preprocess = [timestamp, tid, level, args...]() -> std::string {
        //             return util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
        //                     kLogTypeString[(int)stype],
        //                     util::timestamp(timestamp), tid, level,
        //                     sffl,
        //                     util::stringFormat(sformat, (args)...));
        //         };

        // static auto preprocess = [timestamp, tid, level]() -> std::string {
        //             return util::stringFormat("{%c}{%.9f}{%d}{%d}%s\n",
        //                     kLogTypeString[(int)stype],
        //                     util::timestamp(timestamp), tid, level,
        //                     sffl);
        //         };

        static auto preprocess = [](static DynamicLogInfo &info) -> std::string {
                    return util::stringFormat("{%c}{%.9f}{%d}{%d}%s\n",
                            kLogTypeString[(int)stype],
                            util::timestamp(info.ts), info.tid, info.level,
                            sffl);
                };

        // static const char *sfile = file;
        // static const char *sfunction = function;
        // static const int sline = line;

        // std::string payload = util::stringFormat(format, std::forward<Args>(args)...);
        if(mode == Mode::kAsync)
        {
            // tll::util::timestamp<>();
            
            // const auto context = tll::log::Context::instance().get();
            task_queue_.push( preprocess );
            // assert(ps > 0);
        }
        else
        {
            log_(util::stringFormat("{%c}{%.9f}{%d}{%d}%s{%s}\n",
                            kLogTypeString[(int)stype],
                            util::timestamp(timestamp), tid, level,
                            sffl, 
                            util::stringFormat(sformat, (args)...)));
        }
    }
            

    template <Mode mode, typename... Args>
    void log(Type type, const char *format, Args ...args)
    {
        std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(int)type], util::stringFormat(format, std::forward<Args>(args)...));

        if(mode == Mode::kAsync && isRunning())
        {
            size_t ps = rb_.push([](char *el, size_t sz, const std::string &msg) {
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
    void log(Type type, const char *format, Args ...args)
    {
        // std::string payload = util::stringFormat(format, std::forward<Args>(args)...);
        log<Mode::kAsync>(type, format, args...);
    }



// LOG("%d %x", arg, arg);
// log({file,function,line,format}, arg...)

// std::unordered_map<styd::string, StaticLogInfo> s_info;

// file, function, line, format, ...

// uintptr_t getStaticLogId(const StaticLogInfo &info)
// {
//     s_info[{info.file, line}] = info;
//     return &s_info[{info.file, line}]
// }
// static StaticLogInfo local_info{file, function, format, line};
// static StaticLogInfo *local_log_obj = &local_info;
// s_info[{info.file, line}] = info;
// static size_t log_id = (size_t)&s_info[{info.file, line}];

// static uintptr_t log_id = getStaticLogId({file, function, format, line});

// struct StreamBuffer {
//     StreamBuffer(size_t reserve=0x400) {buff_.reserve(0x400);}
//     ~StreamBuffer()=default;

//     template <typename T>
//     StreamBuffer &operator<< (T val)
//     {
//         uint8_t *crr_pos = buff_.data() + buff_.size();
//         buff_.resize(sizeof(T) + 2);
//         *(uint16_t*)crr_pos = 1; /// primitive id
//         crr_pos += 2;
//         *(val*)crr_pos = val;
//     }

//     template <>
//     StreamBuffer &operator<< <const char *>(const char *val)
//     {
//         uint8_t *crr_pos = buff_.data() + buff_.size();
//         auto str_len = strlen(val);
//         uff_.resize(str_len + 4);

//         *(uint16_t*)crr_pos = 2; /// string id
//         crr_pos += 2;

//         *(uint16_t*)crr_pos = str_len;
//         crr_pos +=2;

//         memcpy(crr_pos, val, str_len);
//         crr_pos += str_len;
//     }

//     auto giveUp()
//     {
//         return std::move(buff_);
//     }

//     std::vector<uint8_t> buff_;
// };

//     template <typename First, typename... Args>
//     auto getArgs_(StreamBuffer &sb, First first, Args ...args)
//     {
//         sb << first;
//         getArgs_(buff, args...);
//     }

//     template <typename First, typename... Args>
//     auto getArgs(First first, Args ...args)
//     {
//         StreamBuffer sb;
//         getArgs_(sb, first, args...);
//         return sb.giveUp();
//     }

    // /// log pack/params
    // template <Mode mode, size_t N, typename... Args>
    // size_t logP(Type type, const char *file, const char *function, int line,
    //             const char (&format)[N], Args &&...args)
    // {
    //     /// {id|len|payload}
    //     /// int|uint32_t|
    //     /// prod:extract params -> pack -> push
    //     /// cons:pop -> compress -> doLog
    //     // static log_id = s_info[info];
    //     static StaticLogInfo s_info = {
    //         .file = file, 
    //         .function = function, 
    //         .format = format,
    //         .line = line, 
    //         .severity = (int)type
    //     };

    //     return 0;
    // }

    // template <Mode mode>
    // void log(Message);

    inline void start(size_t chunk_size = 0x400 * 16, uint32_t period_ms=1000) /// 16 Kb, 1000 ms
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;
        for(auto &entry : ents_)
        {
            // entry.second.start();
            auto &ent = entry.second;
            if(ent.handle == nullptr && ent.onStart)
                ent.handle = ent.onStart();
            /// send first log to notify time_since_epoch
            auto std_now = std::chrono::steady_clock::now();
            auto sys_now = std::chrono::system_clock::now();
            auto sys_time = std::chrono::system_clock::to_time_t(sys_now);
            auto buff = util::stringFormat("%s{%.9f}{%.9f}\n", 
                std::ctime(&sys_time),
                std::chrono::duration_cast< std::chrono::duration<double> >(std_now.time_since_epoch()).count(), 
                std::chrono::duration_cast< std::chrono::duration<double> >(sys_now.time_since_epoch()).count());
            ent.onLog(ent.handle, buff.data(), buff.size());
        }

        broadcast_ = std::thread([this, chunk_size, period_ms]()
        {
            util::Counter<std::chrono::duration<uint32_t, std::ratio<1, 1000>>> counter;
            uint32_t delta = 0;
            // while(isRunning())
            // {
            //     delta += counter.elapse().count();
            //     counter.start();
            //     if(delta < period_ms && rb_.size() < chunk_size)
            //     {
            //         std::unique_lock<std::mutex> lock(mtx_);
            //         /// wait timeout
            //         bool wait_status = pop_wait_.wait_for(lock, std::chrono::milliseconds(period_ms - delta), [this, chunk_size, &delta, &counter] {
            //             delta += counter.elapse().count();
            //             return !isRunning() || (this->rb_.size() >= chunk_size);
            //         });
            //     }
            //     if(delta >= period_ms || rb_.size() >= chunk_size)
            //     {
            //         delta -= period_ms;
            //         size_t rs = rb_.pop([&](const char *el, size_t sz){
            //             log_(el, sz);
            //         }, -1);
            //     }
            // } /// while(isRunning())

            // if(!rb_.empty())
            // {
            //     size_t rs = rb_.pop([&](const char *el, size_t sz){ log_(el, sz); }, -1);
            // }

            while(isRunning())
            {
                delta += counter.elapse().count();
                counter.start();
                if(delta < period_ms && task_queue_.empty())
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    /// wait timeout
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::milliseconds(period_ms - delta), [this, chunk_size, &delta, &counter] {
                        delta += counter.elapse().count();
                        return !isRunning() || !task_queue_.empty();
                    });
                }

                if(!task_queue_.empty())
                {
                    size_t rs = task_queue_.pop([this](std::function<std::string()> *el, size_t sz){
                        this->log_((*el)());
                    }, -1);
                    if(rs) delta = 0;
                }

                if(delta >= period_ms)
                {
                    delta -= period_ms;
                }
            } /// while(isRunning())

            if(!task_queue_.empty())
            {
                size_t rs = task_queue_.pop([this](std::function<std::string()> *el, size_t sz){
                        this->log_((*el)());
                    }, -1);
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
            pop_wait_.notify_all();
            return !isRunning() || rb_.empty();
        });
    }

private:

    inline void log_(const std::string &buff) const
    {
        log_(buff.data(), buff.size());
    }

    inline void log_(const char *buff, size_t size) const
    {
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            ent.onLog(ent.handle, buff, size);
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
    lf::ring_queue_ds<std::function<std::string()>> task_queue_{1000000};

    lf::ring_buffer_mpsc<char> rb_{0x100000 * 16}; /// 16Mb
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

    std::condition_variable pop_wait_, join_wait_;
    std::mutex mtx_;
};

} /// tll::log
