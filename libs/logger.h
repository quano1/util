#pragma once

#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <sstream>
#include <string>
#include <unordered_set>
#include <cassert>
#include <cstdarg>
#include <functional>
#include <array>
#include <unordered_map>
#include <atomic>
#include <cstring>
#include <memory>
#include <list>
#include <algorithm>
#include <omp.h>
#include <set>
#include <condition_variable>
#include "utils.h"
#include "timer.h"

#define LOG_HEADER_ tll::util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}",\
  tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::ContextMap::instance().level(),\
  tll::log::ContextMap::instance().get<tll::log::ContextMap::Prev>(),\
  tll::log::ContextMap::instance().get<tll::log::ContextMap::Curr>())

#define LOG_DBG_ tll::util::stringFormat("{%s}{%d}",\
  __FUNCTION__,\
  __LINE__)

#define TLL_LOGD(plog, format, ...) (plog)->log((tll::log::Type::kDebug), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGI(plog, format, ...) (plog)->log((tll::log::Type::kInfo), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGW(plog, format, ...) (plog)->log((tll::log::Type::kWarn), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGF(plog, format, ...) (plog)->log((tll::log::Type::kFatal), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGT(plog, ID) tll::log::Tracer<> tracer_##ID##__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (LOG_HEADER_ + LOG_DBG_), (/*(Node).log((tll::log::Type::kTrace), "%s\n", LOG_HEADER_ + LOG_DBG_),*/ tll::log::ContextMap::instance()().push_back(tll::util::fileName(__FILE__)), #ID))

#define TLL_LOGTF(plog) tll::log::Tracer<> tracer__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (LOG_HEADER_ + LOG_DBG_), (/*(Node).log((tll::log::Type::kTrace), "%s\n", LOG_HEADER_ + LOG_DBG_),*/ tll::log::ContextMap::instance()().push_back(tll::util::fileName(__FILE__)), __FUNCTION__))

#define TLL_GLOGD(...) TLL_LOGD(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGI(...) TLL_LOGI(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGW(...) TLL_LOGW(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGF(...) TLL_LOGF(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGT(ID) tll::log::Tracer<> tracer_##ID##__([](std::string const &log_msg){tll::log::Node::instance().log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::ContextMap::instance()().push_back(tll::util::fileName(__FILE__)), LOG_HEADER_ + LOG_DBG_), (#ID))

// std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)
#define TLL_GLOGTF() tll::log::Tracer<> tracer__(std::bind(&tll::log::Node::log<std::string const &>, &tll::log::Node::instance(), (tll::log::Type::kTrace), "%s", std::placeholders::_1), (tll::log::ContextMap::instance()().push_back(tll::util::fileName(__FILE__)), LOG_HEADER_ + LOG_DBG_), (__FUNCTION__))

namespace tll::log
{

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

typedef std::function<void *()> OnStart;
typedef std::function<void(void *&)> OnStop;
typedef std::function<void(void*, const char*, size_t)> OnLog;

struct Message
{
    Type type;
    std::string payload;
};

struct Entity
{
    std::string name;
    Flag flag;
    OnLog on_log;
    OnStart on_start;
    OnStop on_stop;
    void *handle;

// private:
    void start()
    {
        if(handle == nullptr && on_start)
            handle = on_start();
    }

    void log(const char *buff, size_t size) const
    {
        on_log(handle, buff, size);
    }

    void stop()
    {
        if(on_stop)
            on_stop(handle);
    }
};

struct ContextMap
{
    enum
    {
        Prev = 0,
        Curr,
    };

    static ContextMap &instance()
    {
        static std::atomic<ContextMap*> singleton;
        for(ContextMap *sin = singleton.load(std::memory_order_relaxed); 
            !sin && !singleton.compare_exchange_weak(sin, new ContextMap(), std::memory_order_release, std::memory_order_acquire);) { }
        return *singleton.load(std::memory_order_relaxed);
    }

    std::vector<std::string> &operator()(const std::thread::id tid = tll::util::tid())
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return map_[tid];
    }

    size_t level(const std::thread::id tid = tll::util::tid())
    {
        std::lock_guard<std::mutex> lock(mtx_);
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
            return ctx->back();
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
        printf(" (%.9f)%s\n", util::timestamp(), name.data());
    }

    Tracer(std::function<void(std::string const&)> logf, std::string header, std::string id="") : name(std::move(id))
    {
        // sig_log.connect(logf);
        on_log = std::move(logf);
        on_log(util::stringFormat("%s{%s}\n", header, name));
    }

    ~Tracer()
    {
        const auto duration = timer().stop();
        if(on_log)
        {
            on_log(util::stringFormat("%s{%s}{%.6f(s)}\n",
                                      LOG_HEADER_,
                                      name,
                                      duration.count()));
            log::ContextMap::instance()().pop_back();
        }
        else if(!name.empty())
            printf(" (%.9f)~%s: %.6f (s)\n", util::timestamp(), name.data(), duration.count());
    }

    std::string name = "";
    time::List<> timer;

    std::function<void(std::string const&)> on_log;
}; /// Tracer

class Node
{
public:
    Node() = default;

    template < typename ... LogEnts>
    Node(uint32_t max_log_in_queue,
           LogEnts ...ents) :
        ents_{},
        ring_queue_{max_log_in_queue}
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
            time::List<std::chrono::duration<uint32_t, std::ratio<1, 1000>>> timer;
            auto buff_list = start_(chunk_size);
            uint32_t total_delta = 0;

            while(isRunning())
            {
                uint32_t delta = timer().restart().count();
                total_delta += delta;

                if(total_delta >= period_ms)
                {
                    total_delta -= period_ms;
                    /// flushing the buffer list
                    for(auto &buff_entry : buff_list)
                    {
                        auto &flag = buff_entry.first;
                        auto &buff = buff_entry.second;
                        onLog(flag, buff);
                        buff.resize(0);
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

                Message log_msg;
                ring_queue_.pop([this, chunk_size, &buff_list, &log_msg](Message &elem, uint32_t) {
                    log_msg = std::move(elem);
                    // buff_list[msg.flag]
                });

                for(auto &buff_entry : buff_list)
                {
                    auto &flag = buff_entry.first;
                    auto &buff = buff_entry.second;
                    if((uint32_t)flag & (uint32_t)toFlag(log_msg.type))
                    {
                        std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(uint32_t)(log_msg.type)], log_msg.payload);
                        size_t const old_size = buff.size();
                        size_t const new_size = old_size + payload.size();
                        buff.resize(new_size);
                        memcpy(buff.data() + old_size, payload.data(), payload.size());
                        /// TODO log exactly chunk_size
                        if(new_size >= chunk_size)
                        {
                            onLog(flag, buff);
                            buff.resize(0); /// avoid elem's destructor
                        }
                    }
                }

                // #pragma omp parallel num_threads(2)
                // {
                //     // FIXME: parallel is 10 times slower???
                //     // #pragma omp parallel for
                //     // #pragma omp for
                //     for(auto &entry : ents_)
                //     {
                //     }
                // }
            }
            /// TODO: popBatch
            ring_queue_.popBatch(~0u, [this, &buff_list](uint32_t index, uint32_t elem_num, uint32_t) {
                for(auto &entry : buff_list)
                {
                    for(int i=0; i<elem_num; i++)
                    {
                        Message &log_msg = ring_queue_.elemAt(index + i);
                        auto &flag = entry.first;
                        auto &buff = entry.second;
                        if((uint32_t)flag & (uint32_t)toFlag(log_msg.type))
                        {
                            std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(uint32_t)(log_msg.type)], log_msg.payload);
                            size_t const old_size = buff.size();
                            size_t const new_size = old_size + payload.size();
                            buff.resize(new_size);
                            memcpy(buff.data() + old_size, payload.data(), payload.size());
                        }
                    }
                }
            });

            for(auto &buff_entry : buff_list)
            {
                auto &flag = buff_entry.first;
                auto &buff = buff_entry.second;
                onLog(flag, buff);
            }
        });
    }

    TLL_INLINE void stop(bool wait=true)
    {
        if(!isRunning()) return;
        
        is_running_.store(false); // write release
        {
            std::lock_guard<std::mutex> lock(mtx_);
            pop_wait_.notify_one();
        }
        if(broadcast_.joinable()) broadcast_.join();
        // stop_signal_.emit();
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            ent.stop();
        }
    }

    /// TODO remove duplicated code
    // TLL_INLINE uintptr_t connectStart(util::Signal<void()>::Slot start)
    // {
    //     return isRunning() ? 0 : start_signal_.connect(start);
    // }

    // TLL_INLINE bool disconnectStart(uintptr_t id)
    // {
    //     return isRunning() ? false : start_signal_.disconnect(id);
    // }

    // TLL_INLINE uintptr_t connectStop(util::Signal<void()>::Slot stop)
    // {
    //     return isRunning() ? 0 : stop_signal_.connect(stop);
    // }

    // TLL_INLINE bool disconnectStop(uintptr_t id)
    // {
    //     return isRunning() ? false : stop_signal_.disconnect(id);
    // }

    TLL_INLINE uintptr_t connect(Flag flag, util::Signal<void(const char*, size_t)>::Slot log)
    {
        return isRunning() ? 0 : log_signal_[flag].connect(log);
    }

    TLL_INLINE bool disconnect(Flag flag, uintptr_t id)
    {
        return isRunning() ? false : log_signal_[flag].disconnect(id);
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

    static Node &instance()
    {
        static std::atomic<Node*> singleton;
        Node *sin = singleton.load(std::memory_order_relaxed);
        while(!sin && !singleton.compare_exchange_weak(sin, new Node(),std::memory_order_release, std::memory_order_acquire)) { }
        return *singleton.load(std::memory_order_relaxed);
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

    TLL_INLINE void onLog(Flag flag, const std::vector<char> buff) const
    {
        this->log_signal_.at(flag).emit(buff.data(), buff.size());
        for(auto &ent_entry : ents_)
        {
            auto &ent = ent_entry.second;
            if(ent.flag == flag)
                ent.log(buff.data(), buff.size());
        }
    }

    TLL_INLINE void onLog(Type type, const std::string payload) const
    {
        for( auto &entry : log_signal_)
        {
            Flag flag = entry.first;
            auto &log_sig = entry.second;

            if((uint32_t)flag & (uint32_t)toFlag(type))
            {
                log_sig.emit(payload.data(), payload.size());
            }
        }

        for( auto &entry : ents_)
        {
            auto &ent = entry.second;
            if((uint32_t)ent.flag & (uint32_t)toFlag(type))
            {
                ent.log(payload.data(), payload.size());
            }
        }
    }

    TLL_INLINE std::unordered_map<Flag, std::vector<char>> start_(uint32_t chunk_size)
    {
        // start_signal_.emit();
        std::unordered_map<Flag, std::vector<char>> buff_list;
        for(const auto &entry : log_signal_)
        {
            auto flag = entry.first;
            buff_list[flag].reserve(chunk_size);
        }

        for(auto &entry : ents_)
        {
            auto &ent = entry.second;
            auto flag = ent.flag;
            log_signal_[flag];
            ent.start();
            buff_list[flag].reserve(chunk_size);
        }
        return buff_list;
    }

    template <typename ... LogEnts>
    void add_(Entity ent, LogEnts ...ents)
    {
        ents_[ent.name] = ent;
        auto &entity = ents_[ent.name];
        /// connect all signals
        // connectStart(std::bind(&Entity::start, &entity));
        // connectStop(std::bind(&Entity::stop, &entity));
        // connectLog(ent.flag, std::bind(&Entity::log, &entity, std::placeholders::_1, std::placeholders::_2));

        add_(ents...);
    }

    TLL_INLINE void add_(Entity ent)
    {
        ents_[ent.name] = ent;
        auto &entity = ents_[ent.name];
        /// connect all signals
        // connectStart(std::bind(&Entity::start, &entity));
        // connectStop(std::bind(&Entity::stop, &entity));
        // connectLog(ent.flag, std::bind(&Entity::log, &entity, std::placeholders::_1, std::placeholders::_2));
    }

    std::atomic<bool> is_running_{false};
    util::LFQueue<Message> ring_queue_{0x1000};
    std::unordered_map<std::string, Entity> ents_ = {{"console", Entity{
                "console",
                Flag::kAll, 
                std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)}}};
    // uint32_t wait_ms_{100};
    std::thread broadcast_;

    std::unordered_map<Flag, util::Signal<void (const char*, size_t)>> log_signal_;
    // util::Signal<void()> start_signal_;
    // util::Signal<void()> stop_signal_;

    std::condition_variable pop_wait_, join_wait_;
    std::mutex mtx_;
};

template <>
TLL_INLINE void Node::log<Mode::kSync>(Message msg)
{
    /// TODO add pre-format
    std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(int)msg.type], msg.payload);

    onLog(msg.type, payload);
}

template <>
TLL_INLINE void Node::log<Mode::kAsync>(Message msg)
{
    if(!isRunning())
    {
        log<Mode::kSync>(msg);
        /// FIXME: force push?
    }
    else
    {
        ring_queue_.push([](Message &elem, uint32_t size, Message msg) {
            elem = std::move(msg);
        }, msg);

        pop_wait_.notify_one();
    }
}

} /// tll::log

#ifdef STATIC_LIB
#include "Node.cc"
#endif