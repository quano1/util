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
#include "timer.h"

#ifndef TLL_DEFAULT_LOG_PATH
#define TLL_DEFAULT_LOG_PATH "."
#endif

extern char *__progname;

#ifdef PROF_LOG
namespace prof {
int asyncCnt = 0;
int hdCnt = 0;
int dologCnt = 0;
}
inline std::string getLogHeader();
#define LOG_HEADER_ getLogHeader()
#else
#define LOG_HEADER_ (tll::log::ContextMap::instance()(), tll::util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}",\
  tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::ContextMap::instance().level(),\
  tll::log::ContextMap::instance().get<tll::log::ContextMap::Prev>(),\
  tll::log::ContextMap::instance().get<tll::log::ContextMap::Curr>()))
#endif

#define LOG_DBG_ tll::util::stringFormat("{%s}{%d}",\
  __FUNCTION__,\
  __LINE__)

#define TLL_LOGD(plog, format, ...) (plog)->log((tll::log::Type::kDebug), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGI(plog, format, ...) (plog)->log((tll::log::Type::kInfo), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGW(plog, format, ...) (plog)->log((tll::log::Type::kWarn), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGF(plog, format, ...) (plog)->log((tll::log::Type::kFatal), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGT(plog, ID) tll::log::Tracer<> tracer_##ID##__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::ContextMap::instance()().push_back(tll::util::fileName(__FILE__)), LOG_HEADER_ + LOG_DBG_), #ID)

#define TLL_LOGTF(plog) tll::log::Tracer<> tracer__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::ContextMap::instance()().push_back(tll::util::fileName(__FILE__)), LOG_HEADER_ + LOG_DBG_), __FUNCTION__)

#define TLL_GLOGD(...) TLL_LOGD(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGI(...) TLL_LOGI(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGW(...) TLL_LOGW(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGF(...) TLL_LOGF(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGT(ID) tll::log::Tracer<> tracer_##ID##__([](std::string const &log_msg){tll::log::Node::instance().log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::ContextMap::instance()().push_back(tll::util::fileName(__FILE__)), LOG_HEADER_ + LOG_DBG_), #ID)

// std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)
#define TLL_GLOGTF() tll::log::Tracer<> tracer__(std::bind(&tll::log::Node::log<std::string const &>, &tll::log::Node::instance(), (tll::log::Type::kTrace), "%s", std::placeholders::_1), (tll::log::ContextMap::instance()().push_back(tll::util::fileName(__FILE__)), LOG_HEADER_ + LOG_DBG_), __FUNCTION__)

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

TLL_INLINE constexpr Flag toFlag(Type type)
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

    friend class Node;
};

struct ContextMap : public util::SingletonT<ContextMap>
{
    enum
    {
        Prev = 0,
        Curr,
    };

    std::vector<std::string> &operator()(const std::thread::id tid = tll::util::tid())
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return map_[tid];
    }

    size_t level(const std::thread::id tid = tll::util::tid())
    {
        std::lock_guard<std::mutex> lock(mtx_);
        assert(map_.find(tid) != map_.end());
        return map_.at(tid).size();
    }

    template <int pos>
    std::string get(const std::thread::id tid = tll::util::tid())
    {
        assert(map_.find(tid) != map_.end());
        std::vector<std::string> *ctx;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            ctx = &map_[tid];
        }

        if(pos == ContextMap::Prev)
        {
            return (ctx->size() < 2) ? "" : ctx->operator[](ctx->size() - 2);
        }
        else 
        {
            return (ctx->size() == 0) ? "" : ctx->back();
        }
    }

private:
    std::mutex mtx_;
    std::unordered_map<std::thread::id, std::vector<std::string>> map_;
}; /// ContextMap

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
        const auto duration = timer().elapse().count();
        if(doLog)
        {
            doLog(util::stringFormat("%s{%s}{%.6f(s)}\n",
                                      LOG_HEADER_,
                                      name,
                                      duration));
            log::ContextMap::instance()().pop_back();
        }
        else if(!name.empty())
            printf(" (%.9f)~%s: %.6f (s)\n", util::timestamp<double, std::ratio<1,1>, clock>(), name.data(), duration);
    }

    time::Counter<> &operator()(const std::string cnt_id="")
    {
        return timer(cnt_id);
    }

    const time::Counter<> &operator()(const std::string cnt_id="") const
    {
        return timer(cnt_id);
    }

    std::string name = "";
    time::Map<> timer;

    std::function<void(std::string const&)> doLog;
}; /// Tracer

class Node
{
public:
    Node()
    {
        // start_(0);
        start();
    }

    Node(uint32_t queue_size) :
        ring_queue_{queue_size}
    {}

    template < typename ... LogEnts>
    Node(uint32_t queue_size,
           LogEnts ...ents) :
        ring_queue_{queue_size}
    {
        add_(ents...);
    }

    ~Node()
    {
        stop();
    }

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;

    template <Mode mode, typename... Args>
    TLL_INLINE void log(Type type, const char *format, Args &&...args)
    {
        std::string payload = util::stringFormat(format, std::forward<Args>(args)...);
        log<mode>(Message{type, payload});
    }

    template <typename... Args>
    TLL_INLINE void log(Type type, const char *format, Args &&...args)
    {
        std::string payload = util::stringFormat(format, std::forward<Args>(args)...);
        log<Mode::kAsync>(Message{type, payload});
    }

    template <Mode mode>
    TLL_INLINE void log(Message);

    TLL_INLINE void start(size_t chunk_size = 0x400, uint32_t period_ms=1000) /// 1 Kb, 1000 ms
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;

        broadcast_ = std::thread([this, chunk_size, period_ms]()
        {
            time::Map<> timer;
            auto buff_list = init_(chunk_size);
            uint32_t total_delta = 0;
            std::vector<char> buff (chunk_size);

            std::function<void(uint32_t, uint32_t, uint32_t)> onPopBatch{[this, &buff_list](uint32_t index, uint32_t elem_num, uint32_t)
            {
                for(uint32_t i=0; i<elem_num; i++)
                {
                    Message &log_msg = ring_queue_.elemAt(index + i);
                    for(auto &entry : buff_list)
                    {
                        auto &flag = entry.first;
                        auto &rb = entry.second;
                        if((uint32_t)flag & (uint32_t)toFlag(log_msg.type))
                        {
                            std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(uint32_t)(log_msg.type)], log_msg.payload);
                            rb.push(payload.data(), payload.size());
                        }
                    }
                }
            }};

            while(isRunning())
            {
                uint32_t delta = timer().restart().duration().count();
                total_delta += delta;

                if(total_delta >= period_ms)
                {
                    total_delta -= period_ms;
                    /// flushing the buffer list
                    for(auto &buff_entry : buff_list)
                    {
                        auto &flag = buff_entry.first;
                        auto &rb = buff_entry.second;
                        size_t s = rb.pop(buff.data(), chunk_size);
                        buff.resize(s);
                        log_(flag, buff);
                        // buff.resize(0);
                    }
                }

                if(ring_queue_.isEmpty())
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    /// wait timeout
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::milliseconds(period_ms - total_delta), [this] {
                        return !isRunning() || !this->ring_queue_.isEmpty();
                    });
                    /// wait timeout or running == false
                    if( !wait_status || !isRunning())
                    {
                        continue;
                    }
                }

                ring_queue_.popBatch(~0u, onPopBatch);

                for(auto &entry : buff_list)
                {
                    auto &flag = entry.first;
                    auto &rb = entry.second;
                    while(rb.size() >= chunk_size)
                    {
                        buff.resize(chunk_size);
                        size_t s = rb.pop(buff.data(), chunk_size);
                        buff.resize(s);
                        log_(flag, buff);
                    }
                }
            }

            ring_queue_.popBatch(~0u, onPopBatch);

            for(auto &entry : buff_list)
            {
                auto &flag = entry.first;
                auto &rb = entry.second;
                while(rb.size() > 0)
                {
                    buff.resize(chunk_size);
                    size_t s = rb.pop(buff.data(), chunk_size);
                    buff.resize(s);
                    log_(flag, buff);
                }
            }
        });
    }

    TLL_INLINE void stop()
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
        LOGD("");
    }

    TLL_INLINE bool isRunning() const 
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

    template <uint32_t queue_size=0x1000>
    static Node &instance()
    {
        static std::atomic<Node*> singleton{nullptr};
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
            singleton.store(new Node{}, std::memory_order_release);
            // singleton->init_(0);
        }

        return *singleton.load(std::memory_order_acquire);
    }

    /// wait for ring_queue is empty
    TLL_INLINE void wait()
    {
        std::mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        join_wait_.wait(lock, [this] 
        {
            pop_wait_.notify_one();
            return !isRunning() || ring_queue_.isEmpty();
        });
    }

private:

    TLL_INLINE void log_(Flag flag, const std::vector<char> &buff) const
    {
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            if(ent.flag == flag)
                ent.log(buff.data(), buff.size());
        }
    }

    TLL_INLINE void log_(Flag flag, const char *buff, size_t size) const
    {
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            if(ent.flag == flag)
                ent.log(buff, size);
        }
    }

    TLL_INLINE void log_(Type type, const std::string &payload) const
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

    // TLL_INLINE std::unordered_map<Flag, std::vector<char>> init_(uint32_t chunk_size)
    TLL_INLINE std::unordered_map<Flag, util::ContiRB> init_(uint32_t chunk_size)
    {
        std::unordered_map<Flag, util::ContiRB> buff_list;
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

    TLL_INLINE void add_(Entity ent)
    {
        ents_[ent.name] = ent;
    }

    std::atomic<bool> is_running_{false};
    util::LFQueue<Message> ring_queue_{0x10000};
    std::unordered_map<std::string, Entity> ents_ = {{"file", Entity{
                .name = "file", .flag = tll::log::Flag::kAll,
                [this](void *handle, const char *buff, size_t size)
                {
#ifdef PROF_LOG
                    tll::util::Guard<tll::time::Map<>::CNT> guard(prof::timer("dolog"));
                    prof::dologCnt++;
#endif
                    if(handle == nullptr)
                    {
                        printf("%.*s", (int)size, buff);
                        return;
                    }
                    static_cast<std::ofstream*>(handle)->write((const char *)buff, size);
                },
                [this]()
                {
#ifdef PROF_LOG
                    tll::util::Guard<tll::time::Map<>::CNT> guard(prof::timer("open"));
#endif
                    auto log_path =std::getenv("TLL_LOG_PATH");
                    auto const &file = util::stringFormat("%s/%s.%d.log", log_path ? log_path : TLL_DEFAULT_LOG_PATH, __progname, getpid());
                    LOGD("%s", file.data());
                    return static_cast<void*>(new std::ofstream(file, std::ios::out | std::ios::binary));
                }, 
                [this](void *&handle)
                {
#ifdef PROF_LOG
                    tll::util::Guard<tll::time::Map<>::CNT> guard(prof::timer("close"));
#endif
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
TLL_INLINE void Node::log<Mode::kSync>(Message msg)
{
    /// TODO add pre-format
    std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(int)msg.type], msg.payload);

    log_(msg.type, payload);
}

template <>
TLL_INLINE void Node::log<Mode::kAsync>(Message msg)
{
    if(!isRunning())
    {
        log<Mode::kSync>(msg);
        /// TODO: force push?
    }
    else
    {
#ifdef PROF_LOG
        tll::util::Guard<tll::time::Map<>::CNT> guard(prof::timer("node::log::async"));
        prof::asyncCnt++;
#endif
        bool rs = ring_queue_.push([](Message &elem, uint32_t, Message msg) {
            elem = std::move(msg);
        }, msg);
        assert(rs);
        pop_wait_.notify_one();
    }
}

}} /// tll::log


#ifdef PROF_LOG
std::string getLogHeader() {
    tll::util::Guard<tll::time::Map<>::CNT> guard(prof::timer("header"));
    prof::hdCnt++;
    tll::log::ContextMap::instance()();
    return tll::util::stringFormat(""
                                   "{%.9f}"
                                   "{%s}"
                                   "{%d}"
                                   "{%s}"
                                   "{%s}"
            ,tll::util::timestamp<>()
            ,tll::util::to_string(tll::util::tid())
            ,tll::log::ContextMap::instance().level()
            ,tll::log::ContextMap::instance().get<tll::log::ContextMap::Prev>()
            ,tll::log::ContextMap::instance().get<tll::log::ContextMap::Curr>()
            );
}
#endif

// #ifdef STATIC_LIB
// #include "Node.cc"
// #endif