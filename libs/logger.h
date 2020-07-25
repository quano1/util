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

#ifndef STATIC_LIB
#define TLL_INLINE inline
#else
#define TLL_INLINE
#endif

#define LOGPD(format, ...) printf("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::util::timestamp(), tll::util::to_string(tll::util::tid()).data(), tll::util::fileName(__FILE__).data(), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) printf("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::util::timestamp(), tll::util::to_string(tll::util::tid()).data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(format, ...) printf("(E)(%.9f)(%s)(%s:%s:%d)(" format ")(%s)\n", tll::util::timestamp(), tll::util::to_string(tll::util::tid()).data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__, strerror(errno))

#define TRACE(ID) tll::log::Tracer<> tracer_##ID##__(#ID)
#define TRACEF() tll::log::Tracer<> tracer__(std::string(__FUNCTION__) + ":" + std::to_string(__LINE__) + "(" + tll::util::to_string(tll::util::tid()) + ")")

namespace tll {

namespace util {
// #define THIS_THREAD_ID_ std::this_thread::get_id()
// using this_tid std::this_thread::get_id();

template<typename T, std::size_t N, std::size_t... I>
constexpr auto make_array_impl(T && value, std::index_sequence<I...>)
{
    return std::array<std::decay_t<T>, N> {
            (static_cast<void>(I), std::forward<T>(value))..., 
        };
}

template<typename T, std::size_t N>
constexpr auto make_array(T && value)
{
    return make_array_impl<T, N>(std::forward<T>(value), std::make_index_sequence<N>{});
}

// template <typename... Args>
inline std::thread::id tid()
{
    return std::this_thread::get_id();
}

inline std::string fileName(const std::string &path)
{
    ssize_t pos = path.find_last_of("/");
    if(pos > 0)
        return path.substr(pos + 1);
    return path;
}

/// format
template <typename T>
T argument(T value) noexcept
{
    return value;
}

template <typename T>
T const * argument(std::basic_string<T> const & value) noexcept
{
    return value.data();
}

template <typename ... Args>
int stringPrint(char * const buffer,
                size_t const bufferCount,
                char const * const format,
                Args const & ... args) noexcept
{
    int const result = snprintf(buffer,
                                bufferCount,
                                format,
                                argument(args) ...);
    return result;
}

template <typename T, typename ... Args>
std::basic_string<T> stringFormat(
    size_t size,
    T const * const format,
    Args const & ... args)
{
    std::basic_string<T> buffer;
    buffer.resize(size);
    int len = stringPrint(&buffer[0], buffer.size(), format, args ...);
    buffer.resize(len);
    return buffer;
}

template <typename T, typename ... Args>
std::basic_string<T> stringFormat(
    T const * const format,
    Args const & ... args)
{
    constexpr size_t kLogSize = 0x1000;
    // std::basic_string<T> buffer;
    // size_t const size = stringPrint(&buffer[0], 0, format, args ...);
    // if (size > 0)
    // {
    //     buffer.resize(size + 1); /// extra for null
    //     stringPrint(&buffer[0], buffer.size(), format, args ...);
    // }
    std::basic_string<T> buffer;
    buffer.resize(kLogSize);
    int len = stringPrint(&buffer[0], kLogSize, format, args ...);
    buffer.resize(len);
    return buffer;
}

inline uint32_t nextPowerOf2(uint32_t val)
{
    val--;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val++;
    return val;
}

inline bool powerOf2(uint32_t val)
{
    return (val & (val - 1)) == 0;
}

template <typename T>
inline std::string to_string(T val)
{
    std::stringstream ss;
    ss << val;
    return ss.str();
}

template <typename T=double, typename D=std::ratio<1,1>, typename C=std::chrono::system_clock>
T timestamp(typename C::time_point &&t = C::now())
{
    return std::chrono::duration_cast<std::chrono::duration<T,D>>(std::forward<typename C::time_point>(t).time_since_epoch()).count();
}

struct RingBuffer
{
public:
    inline void push(char *data, size_t size)
    {

    }
    inline void pop(char *data, size_t size)
    {

    }
    inline size_t size() 
    { 
        return (tail >= head) ? tail - head : wmark - head + tail;
    }

    inline bool empty() { return tail == head; }

    size_t capacity, head, tail, wmark;
    // size_t size_of_chunk_;
    // size_t number_of_chunks_;
};

/// lock-free queue
template <typename T, size_t const kELemSize=sizeof(T)>
class LFQueue
{
public:
    LFQueue(uint32_t num_of_elem) : prod_tail_(0), prod_head_(0), cons_tail_(0), cons_head_(0)
    {
        capacity_ = powerOf2(num_of_elem) ? num_of_elem : nextPowerOf2(num_of_elem);
        buffer_.resize(capacity_ * kELemSize);
    }

    TLL_INLINE bool tryPop(uint32_t &cons_head)
    {
        cons_head = cons_head_.load(std::memory_order_relaxed);

        for(;!empty();)
        {
            if (cons_head == prod_tail_.load(std::memory_order_relaxed))
                return false;

            if(cons_head_.compare_exchange_weak(cons_head, cons_head + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }

        return false;
    }

    TLL_INLINE bool completePop(uint32_t cons_head)
    {
        if (cons_tail_.load(std::memory_order_relaxed) != cons_head)
            return false;

        cons_tail_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template </*typename D,*/ typename F, typename ...Args>
    bool pop(F &&doPop, Args &&...args)
    {
        uint32_t cons_head;
        while(!tryPop(cons_head)) 
        {
            if (empty()) 
                return false;

            std::this_thread::yield();
        }

        std::forward<F>(doPop)(elemAt(cons_head), kELemSize, std::forward<Args>(args)...);

        while(!completePop(cons_head)) 
            std::this_thread::yield();

        return true;
    }

    /// ONE consumer only
    template </*typename D,*/ typename F, typename ...Args>
    bool popBatch(uint32_t max_num, F &&onPopBatch, Args &&...args)
    {
        if (empty())
            return false;

        uint32_t cons_head = cons_head_.load(std::memory_order_relaxed);
        uint32_t elem_num = size();

        if(elem_num > max_num)
            elem_num = max_num;

        while(cons_head + elem_num > prod_tail_.load(std::memory_order_relaxed))
            std::this_thread::yield();

        cons_head_.fetch_add(elem_num, std::memory_order_acquire);

        std::forward<F>(onPopBatch)(cons_head, elem_num, kELemSize, std::forward<Args>(args)...);

        cons_tail_.fetch_add(elem_num, std::memory_order_release);

        return true;
    }

    TLL_INLINE bool tryPush(uint32_t &prod_head)
    {
        prod_head = prod_head_.load(std::memory_order_relaxed);

        for(;!full();)
        {
            if (prod_head == (cons_tail_.load(std::memory_order_relaxed) + capacity_))
                return false;

            if(prod_head_.compare_exchange_weak(prod_head, prod_head + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }
        return false;
    }

    TLL_INLINE bool completePush(uint32_t prod_head)
    {
        if (prod_tail_.load(std::memory_order_relaxed) != prod_head)
            return false;

        prod_tail_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template </*typename D, */typename F, typename ...Args>
    bool push(F &&doPush, Args&&...args)
    {
        uint32_t prod_head;
        prod_head = prod_head_.load(std::memory_order_relaxed);
        while(!tryPush(prod_head))
        {
            if(full()) return false;
            std::this_thread::yield();
        }

        std::forward<F>(doPush)(elemAt(prod_head), kELemSize, std::forward<Args>(args)...);
        while(!completePush(prod_head)) 
            std::this_thread::yield();

        return true;
    }

    TLL_INLINE uint32_t size() const
    {
        return prod_head_.load(std::memory_order_relaxed) - cons_head_.load(std::memory_order_relaxed);
    }

    TLL_INLINE uint32_t wrap(uint32_t index) const
    {
        return index & (capacity_ - 1);
    }

    TLL_INLINE uint32_t capacity() const {
        return capacity_;
    }

    TLL_INLINE bool empty() const {
        return size() == 0;
    }

    TLL_INLINE bool full() const {
        return size() == capacity();
    }

    TLL_INLINE T &elemAt(uint32_t index)
    {
        return buffer_[kELemSize * wrap(index)];
    }

    TLL_INLINE T const &elemAt(uint32_t index) const
    {
        return buffer_[kELemSize * wrap(index)];
    }

    TLL_INLINE size_t elemSize() const
    {
        return kELemSize;
    }

private:

    std::atomic<uint32_t> prod_tail_, prod_head_, cons_tail_, cons_head_;
    uint32_t capacity_;
    std::vector<T> buffer_;
}; /// LFQueue

template <typename Signature>
class Slot
{
public:
    Slot()=default;
    ~Slot()=default;
    Slot(std::function<Signature> cb) : cb_(cb) {}
    operator bool() const
    {
        return cb_ != nullptr;
    }

    template <typename ...Args>
    void operator()(Args &&...args) const
    {
        cb_(std::forward<Args>(args)...);
    }
private:
    std::function<Signature> cb_;
};

template <typename Signature>
class Signal
{
public:
    template<typename SL>
    uintptr_t connect(SL &&slot)
    {
        auto it = slots_.emplace(std::forward<SL>(slot));
        return (uintptr_t)(&*it.first);
    }

    bool disconnect(uintptr_t id)
    {
        auto it = std::find_if(slots_.begin(), slots_.end(),
            [id](const Slot<Signature> &slot)
            {return id == (uintptr_t)(&slot);}
            );
        if(it == slots_.end()) return false;
        slots_.erase(it);
        return true;
    }

    template <typename ...Args>
    void emit(Args &&...args) const
    {
        for(auto &slot : slots_)
        {
            if(slot)
            {
                slot(std::forward<Args>(args)...);
            }
        }
    }
private:
    std::set<Slot<Signature>> slots_;
};

} /// util

namespace log 
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
            auto id = util::tid();
            auto &ctx_list = log::ContextMap::instance()[id];
            auto crr_ctx = ctx_list.back();
            ctx_list.pop_back();
            on_log(util::stringFormat("{%.9f}{%s}{%ld}{%s}{%s}{%s}{%.6f(s)}\n",
                                      util::timestamp(),
                                      util::to_string(util::tid()),
                                      ctx_list.size() + 1,
                                      ctx_list.empty() ? "" : ctx_list.back(),
                                      crr_ctx,
                                      name,
                                      timer.elapse()));
        }
        else if(!name.empty())
            printf(" (%.9f)~%s: %.6f (s)\n", util::timestamp(), name.data(), timer.elapse());
    }

    std::string name = "";
    util::Timer<> timer;
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
        /// TODO: force push
    }
    else
    {
        ring_queue_.push([](Message &elem, uint32_t size, Message msg) {
            elem = std::move(msg);
        }, msg);

        pop_wait_.notify_one();
    }
}

} /// log
} /// tll

#define LOG_HEADER_ tll::util::stringFormat("{%.9f}{%s}{%d}{%s}{%s}{%s}{%d}",\
  tll::util::timestamp<double>(),\
  tll::util::to_string(tll::util::tid()),\
  tll::log::ContextMap::instance().level(tll::util::tid()),\
  tll::log::ContextMap::instance().get<tll::log::ContextMap::Prev>(tll::util::tid()),\
  tll::log::ContextMap::instance().get<tll::log::ContextMap::Curr>(tll::util::tid()),\
  __FUNCTION__,\
  __LINE__)

#define TLL_LOGD(plog, format, ...) (plog)->log((tll::log::Type::kDebug), "%s{" format "}\n", LOG_HEADER_ , ##__VA_ARGS__)

#define TLL_LOGI(plog, format, ...) (plog)->log((tll::log::Type::kInfo), "%s{" format "}\n", LOG_HEADER_ , ##__VA_ARGS__)

#define TLL_LOGW(plog, format, ...) (plog)->log((tll::log::Type::kWarn), "%s{" format "}\n", LOG_HEADER_ , ##__VA_ARGS__)

#define TLL_LOGF(plog, format, ...) (plog)->log((tll::log::Type::kFatal), "%s{" format "}\n", LOG_HEADER_ , ##__VA_ARGS__)

#define TLL_LOGT(plog, ID) tll::log::Tracer<> trace_##ID##__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (LOG_HEADER_), (/*(Node).log((tll::log::Type::kTrace), "%s\n", LOG_HEADER_),*/ tll::log::ContextMap::instance()[tll::util::tid()].push_back(tll::util::fileName(__FILE__)), #ID))

#define TLL_LOGTF(plog) tll::log::Tracer<> trace__([plog](std::string const &log_msg){(plog)->log((tll::log::Type::kTrace), "%s", log_msg);}, (LOG_HEADER_), (/*(Node).log((tll::log::Type::kTrace), "%s\n", LOG_HEADER_),*/ tll::log::ContextMap::instance()[tll::util::tid()].push_back(tll::util::fileName(__FILE__)), __FUNCTION__))

#define TLL_GLOGD(...) TLL_LOGD(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGI(...) TLL_LOGI(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGW(...) TLL_LOGW(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGF(...) TLL_LOGF(&tll::log::Node::instance(), ##__VA_ARGS__)
#define TLL_GLOGT(ID) tll::log::Tracer<> trace_##ID##__([](std::string const &log_msg){tll::log::Node::instance().log((tll::log::Type::kTrace), "%s", log_msg);}, (tll::log::ContextMap::instance()[tll::util::tid()].push_back(tll::util::fileName(__FILE__)), LOG_HEADER_), (#ID))

// std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)
#define TLL_GLOGTF() tll::log::Tracer<> trace__(std::bind(&tll::log::Node::log<std::string const &>, &tll::log::Node::instance(), (tll::log::Type::kTrace), "%s", std::placeholders::_1), (tll::log::ContextMap::instance()[tll::util::tid()].push_back(tll::util::fileName(__FILE__)), LOG_HEADER_), (__FUNCTION__))
#ifdef STATIC_LIB
#include "Node.cc"
#endif