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
#include "utils.h"
#include "timer.h"

#define LOG_HEADER_ tll::util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}",\
  tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::ContextMap::instance().level(tll::util::tid()),\
  tll::log::ContextMap::instance().get<tll::log::ContextMap::Prev>(tll::util::tid()),\
  tll::log::ContextMap::instance().get<tll::log::ContextMap::Curr>(tll::util::tid()))

#define LOG_DBG_ tll::util::stringFormat("{%s}{%d}",\
  __FUNCTION__,\
  __LINE__)

#define TLL_LOGD(plog, format, ...) (plog)->log((tll::log::Type::kDebug), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGI(plog, format, ...) (plog)->log((tll::log::Type::kInfo), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGW(plog, format, ...) (plog)->log((tll::log::Type::kWarn), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGF(plog, format, ...) (plog)->log((tll::log::Type::kFatal), "%s{" format "}\n", LOG_HEADER_ + LOG_DBG_ , ##__VA_ARGS__)

#define TLL_LOGT(plog, ID) tll::log::Tracer<> trace_##ID##__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (LOG_HEADER_ + LOG_DBG_), (/*(Node).log((tll::log::Type::kTrace), "%s\n", LOG_HEADER_ + LOG_DBG_),*/ tll::log::ContextMap::instance()[tll::util::tid()].push_back(tll::util::fileName(__FILE__)), #ID))

#define TLL_LOGTF(plog) tll::log::Tracer<> trace__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (LOG_HEADER_ + LOG_DBG_), (/*(Node).log((tll::log::Type::kTrace), "%s\n", LOG_HEADER_ + LOG_DBG_),*/ tll::log::ContextMap::instance()[tll::util::tid()].push_back(tll::util::fileName(__FILE__)), __FUNCTION__))

#define TLL_GLOGD(...) TLL_LOGD(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGI(...) TLL_LOGI(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGW(...) TLL_LOGW(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGF(...) TLL_LOGF(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGT(ID) tll::log::Tracer<> trace_##ID##__([](std::string const &log_msg){tll::log::Node::instance().log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::ContextMap::instance()[tll::util::tid()].push_back(tll::util::fileName(__FILE__)), LOG_HEADER_ + LOG_DBG_), (#ID))

// std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)
#define TLL_GLOGTF() tll::log::Tracer<> trace__(std::bind(&tll::log::Node::log<std::string const &>, &tll::log::Node::instance(), (tll::log::Type::kTrace), "%s", std::placeholders::_1), (tll::log::ContextMap::instance()[tll::util::tid()].push_back(tll::util::fileName(__FILE__)), LOG_HEADER_ + LOG_DBG_), (__FUNCTION__))

namespace tll::log
{

// typedef std::tuple<LogFlag, OpenLog, CloseLog, DoLog, size_t, void *> Entity;

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

TLL_INLINE constexpr Flag toFLag(Type type)
{
    return static_cast<Flag>(1U << static_cast<int>(type));
}

template <typename... Args>
constexpr Flag toFLag(Type type, Args ...args)
{
    return toFLag(type) | toFLag(args...);
}


typedef std::function<void *()> OpenLog;
typedef std::function<void(void *&)> CloseLog;
typedef std::function<void(void*, const char*, size_t)> DoLog;

// typedef std::pair<Type, std::string> Element;
struct Message
{
    Type type;
    std::string payload;
};

struct Entity
{
    std::string name;
    Flag flag;
    size_t chunk_size;
    DoLog send;
    OpenLog open;
    CloseLog close;
    void *handle;
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

    std::vector<std::string> &operator[](const std::thread::id &id)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return map_[id];
    }

    size_t level(const std::thread::id &id)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return map_.at(id).size();
    }

    template <int pos>
    std::string get(const std::thread::id &id)
    {
        assert(map_.find(id) != map_.end());
        std::vector<std::string> *ctx;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            ctx = &map_[id];
        }

        if(pos == ContextMap::Prev) return (ctx->size() < 2) ? "" : ctx->operator[](ctx->size() - 2);
        else return ctx->back();
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
        if(on_log)
        {
            // auto id = util::tid();
            // auto &ctx_list = log::ContextMap::instance()[id];
            // auto crr_ctx = ctx_list.back();
            on_log(util::stringFormat("%s{%s}{%.6f(s)}\n",
                                      LOG_HEADER_,
                                      name,
                                      timer.counter().stop()));
            log::ContextMap::instance()[util::tid()].pop_back();
        }
        else if(!name.empty())
            printf(" (%.9f)~%s: %.6f (s)\n", util::timestamp(), name.data(), timer.counter().stop());
    }

    std::string name = "";
    time::List<> timer;
    /// one extra tp for destructor (total time)
    // std::array<typename clock::time_point, num_of_tp+1> time_points = util::make_array<typename clock::time_point, num_of_tp+1>(clock::now());

    std::function<void(std::string const&)> on_log;
}; /// Tracer

class Node
{
public:
    Node() = default;

    template < typename ... LogEnts>
    Node(uint32_t max_log_in_queue, uint32_t wait_ms,
           LogEnts ...ents) :
        ents_{},
        ring_queue_{max_log_in_queue},
        wait_ms_{wait_ms}
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

    TLL_INLINE void start(size_t chunk_size = 0x1000)
    {
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed))
            return;

        broadcast_ = std::thread([this, chunk_size]()
        {
            auto buff_list = start_(chunk_size);
            while(isRunning())
            {
                if(ring_queue_.empty())
                {
                    // join_wait_.notify_one(); /// notify join
                    // std::mutex tmp_mtx; /// FIXME: without this, crash
                    std::unique_lock<std::mutex> lock(mtx_);
                    /// wait timeout
                    bool wait_status = pop_wait_.wait_for(lock, std::chrono::milliseconds(wait_ms_), [this] {
                        return !isRunning() || !this->ring_queue_.empty();
                    });

                    for(auto &entry : buff_list)
                    {
                        auto &flag = entry.first;
                        auto &buff = entry.second;
                        // this->log_signals[flag].emit(buff.data(), buff.size());
                        buff.resize(0);
                    }
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

                for(auto &entry : buff_list)
                {
                    auto &flag = entry.first;
                    auto &buff = entry.second;
                    if((uint32_t)flag & (uint32_t)log_msg.type)
                    {
                        std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(uint32_t)(log_msg.type)], log_msg.payload);
                        size_t const old_size = buff.size();
                        size_t const new_size = old_size + payload.size();
                        buff.resize(new_size);
                        memcpy(buff.data() + old_size, payload.data(), payload.size());
                        /// TODO log exactly chunk_size
                        // if(new_size >= chunk_size)
                        //     this->log_signals[flag].emit(buff.data(), buff.size());

                        buff.resize(0); /// avoid elem's destructor
                    }
                }

                // #pragma omp parallel num_threads(2)
                // {
                //     // FIXME: parallel is 10 times slower???
                //     // #pragma omp parallel for
                //     // #pragma omp for
                //     for(auto &entry : ents_)
                //     {
                //         auto &name = entry.first;
                //         auto &ent = entry.second;
                //         Type const kLogt = log_msg.type;

                //         if((uint32_t)ent.flag & (uint32_t)toFLag(kLogt))
                //         {
                //             std::string msg = util::stringFormat("{%c}%s", kLogTypeString[(uint32_t)(kLogt)], log_msg.payload);
                //             auto &buff = buff_list_[name];
                //             size_t const old_size = buff.size();
                //             size_t const new_size = old_size + msg.size();

                //             buff.resize(new_size);
                //             memcpy(buff.data() + old_size, msg.data(), msg.size());
                //             if(new_size >= ent.chunk_size)
                //             {
                //                 this->send_(name);
                //             }
                //         }
                //     }
                // }
            }
            /// TODO: popBatch
            ring_queue_.popBatch(~0u, [this, &buff_list](uint32_t index, uint32_t elem_num, uint32_t) {
                for(int i=0; i<elem_num; i++)
                {
                    Message &log_msg = ring_queue_.elemAt(index + i);
                    for(auto &entry : buff_list)
                    {
                        auto &flag = entry.first;
                        auto &buff = entry.second;
                        if((uint32_t)flag & (uint32_t)log_msg.type)
                        {
                            std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(uint32_t)(log_msg.type)], log_msg.payload);
                            size_t const old_size = buff.size();
                            size_t const new_size = old_size + payload.size();
                            buff.resize(new_size);
                            memcpy(buff.data() + old_size, payload.data(), payload.size());
                            // this->log_signals[flag].emit(buff.data(), buff.size());

                            buff.resize(0); /// avoid elem's destructor
                        }
                    }
                }
            });
            // this->send_(); /// this is necessary
            // join_wait_.notify_one(); /// notify join
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
        // if (wait) this->wait();
        if(broadcast_.joinable()) broadcast_.join();

        for(auto &entry : ents_)
        {
            auto &ent = entry.second;
            if(ent.close)
                ent.close(ent.handle);
        }
    }

    TLL_INLINE void pause()
    {

    }

    TLL_INLINE void resume()
    {

    }

    TLL_INLINE bool isRunning() const 
    {
        return is_running_.load(std::memory_order_relaxed);
    }

    template < typename ... LogEnts>
    void add(LogEnts ...ents)
    {
        if(isRunning())
            return;

        add_(ents...);
    }

    TLL_INLINE void remove(const std::string &name)
    {
        if(isRunning())
            return;

        // {
            auto it = ents_.find(name);
            if(it != ents_.end())
            {
                ents_.erase(it);
            }
        // }
        // {
        //     auto it = buffers_.find(name);
        //     if(it != buffers_.end())
        //     {
        //         buffers_.erase(it);
        //     }
        // }
    }

    static Node &instance()
    {
        static std::atomic<Node*> singleton;
        for(Node *sin = singleton.load(std::memory_order_relaxed); 
            !sin && !singleton.compare_exchange_weak(sin, new Node(), std::memory_order_release, std::memory_order_acquire);) { }
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
            return !isRunning() || ring_queue_.empty();
        });
    }

private:

    TLL_INLINE std::unordered_map<Flag, std::vector<char>> start_(uint32_t chunk_size)
    {
        std::unordered_map<Flag, std::vector<char>> buff_list;
        for(auto &entry : ents_)
        {
            auto &name = entry.first;
            auto &ent = entry.second;
            if(ent.open && ent.handle == nullptr)
                ent.handle = ent.open();

            buff_list[ent.flag].reserve(chunk_size);
            // log_signals[ent.flag].connect(ent.send);
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
        /// connect all signals

    }

    // TLL_INLINE void send_()
    // {
    //     for(auto &entry : ents_)
    //     {
    //         send_(entry.first);
    //     }
    // }

    // TLL_INLINE void send_(const std::string &name)
    // {
    //     auto &ent = ents_[name];
    //     // auto &buff = buffers_[name];
    //     ent.send(ent.handle, buff.data(), buff.size());
    //     buff.resize(0);
    // }

    // TLL_INLINE void send_(const std::string &name,
    //         Type type, 
    //         const std::string &buff)
    // {
    //     auto &ent = ents_[name];
    //     if(! ((uint32_t)ent.flag & (uint32_t)toFLag(type)) ) return;
    //     ent.send(ent.handle, buff.data(), buff.size());
    // }

    std::atomic<bool> is_running_{false};
    util::LFQueue<Message> ring_queue_{0x1000};
    std::unordered_map<std::string, Entity> ents_/*{{"console", Entity{
                "console",
                Flag::kAll, 0x1000,
                std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)}}}*/;
    uint32_t wait_ms_{100};
    std::thread broadcast_;
    // std::unordered_map<Flag, DoLog> log_signals;
    // std::unordered_map<std::string, std::vector<char>> buffers_;
    std::condition_variable pop_wait_, join_wait_;
    std::mutex mtx_;
};

template <>
TLL_INLINE void Node::log<Mode::kSync>(Message msg)
{
    for (auto &entry : ents_)
    {
        /// TODO: add format
        auto &ent = entry.second;
        if((uint32_t)msg.type & (uint32_t)ent.flag)
        {
            // this->send_(util::stringFormat("{%c}%s", kLogTypeString[(int)msg.type], msg.payload));
            std::string payload = util::stringFormat("{%c}%s", kLogTypeString[(int)msg.type], msg.payload);
            ent.send(ent.handle, payload.data(), payload.size());
            // this->log_signals[ent.flag].emit(payload.data(), payload.size());
        }
    }
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