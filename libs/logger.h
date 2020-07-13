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

#include <unordered_map>
#include <atomic>
#include <cstring>
#include <memory>
#include <list>
#include <algorithm>
#include <omp.h>

#define LOGPD(format, ...) printf("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::timestamp(), tll::to_string(tll::tid()).data(), tll::fileName(__FILE__).data(), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) printf("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::timestamp(), tll::to_string(tll::tid()).data(), tll::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(format, ...) printf("(E)(%.9f)(%s)(%s:%s:%d)(" format ")(%s)\n", tll::timestamp(), tll::to_string(tll::tid()).data(), tll::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__, strerror(errno))

#define TIMER(ID) tll::Timer __timer_##ID(#ID)
#define TRACE() tll::Timer __tracer(std::string(__FUNCTION__) + ":" + std::to_string(__LINE__) + "(" + tll::to_string(tll::tid()) + ")")

namespace Simple {

namespace Lib {

/// ProtoSignal is the template implementation for callback list.
template<typename,typename> class ProtoSignal;   // undefined

/// CollectorInvocation invokes signal handlers differently depending on return type.
template<typename,typename> struct CollectorInvocation;

/// CollectorLast returns the result of the last signal handler from a signal emission.
template<typename Result>
struct CollectorLast {
    using CollectorResult = Result;
    explicit        CollectorLast ()              : last_() {}
    inline bool     operator()    (Result r)      {
        last_ = r;
        return true;
    }
    CollectorResult result        ()              {
        return last_;
    }
private:
    Result last_;
};

/// CollectorDefault implements the default signal handler collection behaviour.
template<typename Result>
struct CollectorDefault : CollectorLast<Result>
{};

/// CollectorDefault specialisation for signals with void return type.
template<>
struct CollectorDefault<void> {
    using CollectorResult = void;
    void                  result     ()           {}
    inline bool           operator() (void)       {
        return true;
    }
};

/// CollectorInvocation specialisation for regular signals.
template<class Collector, class R, class... Args>
struct CollectorInvocation<Collector, R (Args...)> {
    inline bool
    invoke (Collector &collector, const std::function<R (Args...)> &cbf, Args... args) const
    {
        return collector (cbf (args...));
    }
};

/// CollectorInvocation specialisation for signals with void return type.
template<class Collector, class... Args>
struct CollectorInvocation<Collector, void (Args...)> {
    inline bool
    invoke (Collector &collector, const std::function<void (Args...)> &cbf, Args... args) const
    {
        cbf (args...);
        return collector();
    }
};

/// ProtoSignal template specialised for the callback signature and collector.
template<class Collector, class R, class... Args>
class ProtoSignal<R (Args...), Collector> : private CollectorInvocation<Collector, R (Args...)> {
protected:
    using CbFunction = std::function<R (Args...)>;
    using Result = typename CbFunction::result_type;
    using CollectorResult = typename Collector::CollectorResult;

private:
    /*copy-ctor*/
    ProtoSignal (const ProtoSignal&) = delete;
    ProtoSignal&  operator=   (const ProtoSignal&) = delete;

    using CallbackSlot = std::shared_ptr<CbFunction>;
    using CallbackList = std::list<CallbackSlot>;
    CallbackList callback_list_;

    size_t add_cb(const CbFunction& cb)
    {
        callback_list_.emplace_back(std::make_shared<CbFunction>(cb));
        return size_t (callback_list_.back().get());
    }

    bool remove_cb(size_t id)
    {
        auto it =std::remove_if(begin(callback_list_), end(callback_list_),
        [id](const CallbackSlot& slot) {
            return size_t(slot.get()) == id;
        });
        bool const removed = it != end(callback_list_);
        callback_list_.erase(it, end(callback_list_));
        return removed;
    }

public:
    /// ProtoSignal constructor, connects default callback if non-nullptr.
    ProtoSignal (const CbFunction &method)
    {
        if (method)
            add_cb(method);
    }
    /// ProtoSignal destructor releases all resources associated with this signal.
    ~ProtoSignal ()
    {
    }

    /// Operator to add a new function or lambda as signal handler, returns a handler connection ID.
    size_t connect (const CbFunction &cb)      {
        return add_cb(cb);
    }
    /// Operator to remove a signal handler through it connection ID, returns if a handler was removed.
    bool   disconnect (size_t connection)         {
        return remove_cb(connection);
    }

    /// Emit a signal, i.e. invoke all its callbacks and collect return types with the Collector.
    CollectorResult
    emit (Args... args) const
    {
        Collector collector;
        for (auto &slot : callback_list_) {
            if (slot) {
                const bool continue_emission = this->invoke (collector, *slot, args...);
                if (!continue_emission)
                    break;
            }
        }
        return collector.result();
    }
    // Number of connected slots.
    std::size_t
    size () const
    {
        return callback_list_.size();
    }

    operator bool() const {
        return size() > 0;
    }
};

} // Lib
// namespace Simple

/**
 * Signal is a template type providing an interface for arbitrary callback lists.
 * A signal type needs to be declared with the function signature of its callbacks,
 * and optionally a return result collector class type.
 * Signal callbacks can be added with operator+= to a signal and removed with operator-=, using
 * a callback connection ID return by operator+= as argument.
 * The callbacks of a signal are invoked with the emit() method and arguments according to the signature.
 * The result returned by emit() depends on the signal collector class. By default, the result of
 * the last callback is returned from emit(). Collectors can be implemented to accumulate callback
 * results or to halt a running emissions in correspondance to callback results.
 * The signal implementation is safe against recursion, so callbacks may be removed and
 * added during a signal emission and recursive emit() calls are also safe.
 * The overhead of an unused signal is intentionally kept very low, around the size of a single pointer.
 * Note that the Signal template types is non-copyable.
 */
template <typename SignalSignature, class Collector = Lib::CollectorDefault<typename std::function<SignalSignature>::result_type> >
struct Signal /*final*/ :
    Lib::ProtoSignal<SignalSignature, Collector>
{
    using ProtoSignal = Lib::ProtoSignal<SignalSignature, Collector>;
    using CbFunction = typename ProtoSignal::CbFunction;
    /// Signal constructor, supports a default callback as argument.
    Signal (const CbFunction &method = CbFunction()) : ProtoSignal (method) {}
};

/// This function creates a std::function by binding @a object to the member function pointer @a method.
template<class Instance, class Class, class R, class... Args> std::function<R (Args...)>
slot (Instance &object, R (Class::*method) (Args...))
{
    return [&object, method] (Args... args) {
        return (object .* method) (args...);
    };
}

/// This function creates a std::function by binding @a object to the member function pointer @a method.
template<class Class, class R, class... Args> std::function<R (Args...)>
slot (Class *object, R (Class::*method) (Args...))
{
    return [object, method] (Args... args) {
        return (object ->* method) (args...);
    };
}

/// Keep signal emissions going while all handlers return !0 (true).
template<typename Result>
struct CollectorUntil0 {
    using CollectorResult = Result;
    explicit                      CollectorUntil0 ()      : result_() {}
    const CollectorResult&        result          ()      {
        return result_;
    }
    inline bool
    operator() (Result r)
    {
        result_ = r;
        return result_ ? true : false;
    }
private:
    CollectorResult result_;
};

/// Keep signal emissions going while all handlers return 0 (false).
template<typename Result>
struct CollectorWhile0 {
    using CollectorResult = Result;
    explicit                      CollectorWhile0 ()      : result_() {}
    const CollectorResult&        result          ()      {
        return result_;
    }
    inline bool
    operator() (Result r)
    {
        result_ = r;
        return result_ ? false : true;
    }
private:
    CollectorResult result_;
};

/// CollectorVector returns the result of the all signal handlers from a signal emission in a std::vector.
template<typename Result>
struct CollectorVector {
    using CollectorResult = std::vector<Result>;
    const CollectorResult&        result ()       {
        return result_;
    }
    inline bool
    operator() (Result r)
    {
        result_.push_back (r);
        return true;
    }
private:
    CollectorResult result_;
};

} // Simple

namespace tll {

// #define THIS_THREAD_ID_ std::this_thread::get_id()
// using this_tid std::this_thread::get_id();

template <typename... Args>
auto tid(Args&&... args) -> decltype(std::this_thread::get_id(std::forward<Args>(args)...))
{
    return std::this_thread::get_id(std::forward<Args>(args)...);
}

// namespace utils {
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

template <typename T=double, typename D=std::ratio<1,1>, typename C=std::chrono::high_resolution_clock>
T timestamp(typename C::time_point &&t = C::now())
{
    return std::chrono::duration_cast<std::chrono::duration<T,D>>(std::forward<typename C::time_point>(t).time_since_epoch()).count();
}

struct StackMap
{
    enum class Pos
    {
        Previous = 0,
        Current = 1
    };

    static StackMap &instance()
    {
        static std::atomic<StackMap*> singleton;
        for(StackMap *sin = singleton.load(std::memory_order_relaxed); 
            !sin && !singleton.compare_exchange_weak(sin, new StackMap(), std::memory_order_acquire, std::memory_order_relaxed);) { }
        return *singleton.load(std::memory_order_relaxed);
    }

    std::vector<std::string> &operator[](const std::thread::id &id)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return stack_[id];
    }

    size_t level(const std::thread::id &id)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return stack_.at(id).size();
    }

    template <Pos pos>
    std::string get(const std::thread::id &id)
    {
        assert(stack_.find(id) != stack_.end());
        std::vector<std::string> *ctx;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            ctx = &stack_[id];
        }

        if(pos == Pos::Previous) return (ctx->size() < 2) ? "" : ctx->operator[](ctx->size() - 2);
        else return ctx->back();
    }

private:
    std::mutex mtx_;
    std::unordered_map<std::thread::id, std::vector<std::string>> stack_;
};

template <typename T, size_t const kELemSize=sizeof(T)>
class BSDLFQ
{
public:
    BSDLFQ(uint32_t num_of_elem) : prod_tail_(0), prod_head_(0), cons_tail_(0), cons_head_(0)
    {
        capacity_ = powerOf2(num_of_elem) ? num_of_elem : nextPowerOf2(num_of_elem);
        buffer_.resize(capacity_ * kELemSize);
    }

    template <typename D, typename F, typename ...Args>
    void pop(D &&delay, F &&doPop, Args &&...args)
    {
        uint32_t cons_head;
        //  = cons_head_.load(std::memory_order_relaxed);
        // for(;;)
        // {
        //     if (cons_head == prod_tail_.load(std::memory_order_relaxed))
        //         continue;

        //     if(cons_head_.compare_exchange_weak(cons_head, cons_head + 1, std::memory_order_acquire, std::memory_order_relaxed))
        //         break;
        // }
        while(!tryPop(cons_head)) {
            std::forward<D>(delay)();
        }

        std::forward<F>(doPop)(elemAt(cons_head), kELemSize, std::forward<Args>(args)...);
        // while (cons_tail_.load(std::memory_order_relaxed) != cons_head);

        // cons_tail_.fetch_add(1, std::memory_order_release);
        while(!completePop(cons_head)) {
            std::forward<D>(delay)();
        }
    }

    template <typename D, typename F, typename ...Args>
    void push(D &&delay, F &&doPush, Args&&...args)
    {
        uint32_t prod_head;
        //  = prod_head_.load(std::memory_order_relaxed);
        // for(;;)
        // {
        //     if (prod_head == (cons_tail_.load(std::memory_order_relaxed) + capacity_))
        //         continue;

        //     if(prod_head_.compare_exchange_weak(prod_head, prod_head + 1, std::memory_order_acquire, std::memory_order_relaxed))
        //         break;
        // }
        while(!tryPush(prod_head)) {
            std::forward<D>(delay)();
        }
        std::forward<F>(doPush)(elemAt(prod_head), kELemSize, std::forward<Args>(args)...);
        // while (prod_tail_.load(std::memory_order_relaxed) != prod_head);

        // prod_tail_.fetch_add(1, std::memory_order_release);
        while(!completePush(prod_head)) {
            std::forward<D>(delay)();
        }
    }

    inline bool tryPop(uint32_t &cons_head)
    {
        cons_head = cons_head_.load(std::memory_order_relaxed);

        for(;;)
        {
            if (cons_head == prod_tail_.load(std::memory_order_relaxed))
                return false;

            if(cons_head_.compare_exchange_weak(cons_head, cons_head + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }

        return false;
    }

    inline bool completePop(uint32_t cons_head)
    {
        if (cons_tail_.load(std::memory_order_relaxed) != cons_head)
            return false;

        cons_tail_.fetch_add(1, std::memory_order_release);
        return true;
    }

    inline bool tryPush(uint32_t &prod_head)
    {
        prod_head = prod_head_.load(std::memory_order_relaxed);

        for(;;)
        {
            if (prod_head == (cons_tail_.load(std::memory_order_relaxed) + capacity_))
                return false;

            if(prod_head_.compare_exchange_weak(prod_head, prod_head + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }
        return false;
    }

    inline bool completePush(uint32_t prod_head)
    {
        if (prod_tail_.load(std::memory_order_relaxed) != prod_head)
            return false;

        prod_tail_.fetch_add(1, std::memory_order_release);
        return true;
    }

    inline bool empty() const {
        return size() == 0;
    }

    inline uint32_t size() const
    {
        return prod_tail_.load(std::memory_order_relaxed) - cons_tail_.load(std::memory_order_relaxed);
    }

    inline uint32_t wrap(uint32_t index) const
    {
        return index & (capacity_ - 1);
    }

    inline uint32_t capacity() const {
        return capacity_;
    }

    inline T &elemAt(uint32_t index)
    {
        return buffer_[kELemSize * wrap(index)];
    }

    inline T const &elemAt(uint32_t index) const
    {
        return buffer_[kELemSize * wrap(index)];
    }

    inline size_t elemSize() const
    {
        return kELemSize;
    }

private:

    std::atomic<uint32_t> prod_tail_, prod_head_, cons_tail_, cons_head_;
    uint32_t capacity_;
    std::vector<T> buffer_;
};

struct Timer
{
    using hrc = std::chrono::high_resolution_clock;

    Timer() : name(""), begin(hrc::now()) {}
    Timer(std::string id) : name(std::move(id)), begin(hrc::now())
    {
        printf(" (%.9f)%s\n", tll::timestamp(), name.data());
    }

    Timer(std::function<void(std::string const&)> logf, std::string start_log, std::string id="") : name(std::move(id)), begin(hrc::now())
    {
        sig_log.connect(logf);
        sig_log.emit(stringFormat("%s{%s}\n", start_log, name));
    }

    ~Timer()
    {
        if(sig_log)
        {
            auto id = tll::tid();
            auto &ctx_list = StackMap::instance()[id];
            auto crr_ctx = ctx_list.back();
            ctx_list.pop_back();
            sig_log.emit(stringFormat("{%.9f}{%s}{%ld}{%s}{%s}{%s}{%.6f(s)}\n",
                                      tll::timestamp(),
                                      tll::to_string(tll::tid()),
                                      ctx_list.size() + 1,
                                      ctx_list.empty() ? "" : ctx_list.back(),
                                      crr_ctx,
                                      name,
                                      elapse()));
        }
        else if(!name.empty())
            printf(" (%.9f)~%s: %.6f (s)\n", tll::timestamp(), name.data(), elapse());
    }

    template <typename T=double, typename D=std::ratio<1,1>>
    T reset()
    {
        T ret = elapse<T,D>();
        begin = hrc::now();
        return ret;
    }

    template <typename T=double, typename D=std::ratio<1,1>>
    T elapse() const
    {
        using namespace std::chrono;
        return duration_cast<std::chrono::duration<T,D>>(hrc::now() - begin).count();
    }

    template <typename T=double, typename D=std::ratio<1,1>>
    std::chrono::duration<T,D> duration() const
    {
        using namespace std::chrono;
        auto ret = duration_cast<std::chrono::duration<T,D>>(hrc::now() - begin);
        return ret;
    }

    std::string name;
    hrc::time_point begin;

    Simple::Signal<void(std::string const&)> sig_log;
};

// } /// utils

#ifdef STATIC_LIB
#define TLL_INLINE inline
#else
#define TLL_INLINE
#endif

typedef uint32_t LogFlag;

enum class LogType /// LogType
{
    kTrace=0,
    kDebug,
    kInfo,
    kWarn,
    kFatal,
};

char const kLogTypeString[]="TDIWF";

TLL_INLINE constexpr LogFlag toFLag(LogType type)
{
    return 1U << static_cast<LogFlag>(type);
}

template <typename... Args>
constexpr LogFlag toFLag(tll::LogType type, Args ...args)
{
    return toFLag(type) | toFLag(args...);
}

namespace mask {
constexpr LogFlag trace = toFLag(LogType::kTrace);
constexpr LogFlag debug = toFLag(LogType::kDebug);
constexpr LogFlag info = toFLag(LogType::kInfo);
constexpr LogFlag warn = toFLag(LogType::kWarn);
constexpr LogFlag fatal = toFLag(LogType::kFatal);
constexpr LogFlag all = trace | debug | info | warn | fatal;
}

typedef std::function<void *()> OpenLog;
typedef std::function<void(void *)> CloseLog;
typedef std::function<void(void *,const char*, size_t)> DoLog;

typedef std::pair<LogType, std::string> LogInfo;
// typedef std::tuple<LogFlag, OpenLog, CloseLog, DoLog, size_t, void *> LogEntity;

struct LogEntity
{
    std::string name;
    LogFlag flag;
    size_t chunk_size;
    std::function<void(void *,const char*, size_t)> send;
    std::function<void *()> open;
    std::function<void (void *&)> close;
    void *handle;
};

class Logger
{
public:
    Logger() : 
        async_(false), 
        wait_ms_(1000), 
        is_running_(false),
        ring_queue_(0x1000)
    {
        /// printf
        addLogEnt_(LogEntity{
            "console",
            mask::all, 0x1000,
            std::bind(printf, "%.*s", std::placeholders::_3, std::placeholders::_2)});
    }

    template < typename ... LogEnts>
    Logger(uint32_t max_log_in_queue, uint32_t wait_ms,
           LogEnts ...log_ents) : 
        async_(false), 
        wait_ms_(wait_ms), 
        is_running_(false),
        ring_queue_(max_log_in_queue)
    {
        addLogEnt_(log_ents...);
    }

    ~Logger()
    {
        stop();
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    TLL_INLINE bool async() const
    {
        return async_;
    }

    TLL_INLINE bool &async()
    {
        return async_;
    }

    /// TODO: separate function to sync & async
    template <typename... Args>
    void log(int type, const char *format, Args &&...args)
    {
        std::string message = stringFormat(format, std::forward<Args>(args)...);

        if(async_)
        {
            ring_queue_.push(
            []() {
                std::this_thread::yield();
            },
            [](LogInfo &elem, uint32_t size, LogInfo &&log_msg)
            {
                elem = std::move(log_msg);
            }, LogInfo{static_cast<LogType>(type), message}
            );

            pop_wait_.notify_one();
        }
        else
        {
            for (auto &entry : log_ents_)
            {
                this->send_(entry.first, type, 
                             stringFormat("{%c}%s", kLogTypeString[type], message));
            }
        }
    }

    // template <typename... Args>
    // void logd(const char *format, Args &&...args)
    // {
    //     this->log(static_cast<uint32_t>(LogType::kDebug), format, args...);
    // }
    // template <typename... Args>
    // void logt(const char *format, Args &&...args)
    // {
    //     this->log(static_cast<uint32_t>(LogType::kTrace), format, args...);
    // }
    // template <typename... Args>
    // void logi(const char *format, Args &&...args)
    // {
    //     this->log(static_cast<uint32_t>(LogType::kInfo), format, args...);
    // }
    // template <typename... Args>
    // void logw(const char *format, Args &&...args)
    // {
    //     this->log(static_cast<uint32_t>(LogType::kWarn), format, args...);
    // }
    // template <typename... Args>
    // void logf(const char *format, Args &&...args)
    // {
    //     this->log(static_cast<uint32_t>(LogType::kFatal), format, args...);
    // }

    TLL_INLINE void start()
    {
        init_();
        bool val = false;
        if(!is_running_.compare_exchange_strong(val, true, std::memory_order_relaxed)) return;
        async_ = true;
        broadcast_ = std::thread([this]()
        {
            while(isRunning())
            {
                if(ring_queue_.empty())
                {
                    join_wait_.notify_one(); /// notify join
                    std::mutex tmp_mtx; /// FIXME: without this, crash
                    std::unique_lock<std::mutex> tmp_lock(tmp_mtx);
                    /// wait timeout
                    bool wait_status = pop_wait_.wait_for(tmp_lock, std::chrono::milliseconds(wait_ms_), [this] {
                        return !isRunning() || !this->ring_queue_.empty();
                    });

                    this->send_();
                    /// wait timeout or running == false
                    if( !wait_status || !isRunning())
                    {
                        continue;
                    }
                }

                LogInfo log_inf;
                ring_queue_.pop(
                []() {
                    std::this_thread::yield();
                },
                [&log_inf](LogInfo &elem, uint32_t)
                {
                    log_inf = std::move(elem);
                });
                // #pragma omp parallel num_threads(2)
                {
                    // FIXME: parallel is 10 times slower???
                    // #pragma omp parallel for
                    // #pragma omp for
                    for(auto &entry : log_ents_)
                    {
                        auto &name = entry.first;
                        auto &log_ent = entry.second;
                        LogType const kLogt = log_inf.first;

                        if(log_ent.flag & tll::toFLag(kLogt))
                        {
                            std::string msg = stringFormat("{%c}%s", kLogTypeString[(uint32_t)(kLogt)], log_inf.second);
                            auto &buff = buffers_[name];
                            size_t const old_size = buff.size();
                            size_t const new_size = old_size + msg.size();

                            buff.resize(new_size);
                            memcpy(buff.data() + old_size, msg.data(), msg.size());
                            if(new_size >= log_ent.chunk_size)
                            {
                                this->send_(name);
                            }
                        }
                    }
                }
            }

            join_wait_.notify_one(); /// notify join
        });
    }

    TLL_INLINE void stop(bool wait=true)
    {
        if(!isRunning()) return;

        if (wait) this->wait();
        async_ = false;
        is_running_.store(false); // write release
        if(broadcast_.joinable()) broadcast_.join();

        this->send_(); /// this is necessary
        for(auto &entry : log_ents_)
        {
            auto &log_ent = entry.second;
            if(log_ent.close)
                log_ent.close(log_ent.handle);
        }
    }

    TLL_INLINE bool isRunning() const 
    {
        return is_running_.load(std::memory_order_relaxed);
    }

    template < typename ... LogEnts>
    void addLogEnt(LogEnts ...log_ents)
    {
        if(isRunning())
            return;

        addLogEnt_(log_ents...);
    }

    TLL_INLINE void remLogEnt(const std::string &name)
    {
        if(isRunning())
            return;

        {
            auto it = log_ents_.find(name);
            if(it != log_ents_.end())
            {
                log_ents_.erase(it);
            }
        }
        {
            auto it = buffers_.find(name);
            if(it != buffers_.end())
            {
                buffers_.erase(it);
            }
        }
    }

    static Logger &instance()
    {
        // static Logger instance;
        // return &instance;
        static std::atomic<Logger*> singleton;
        for(Logger *sin = singleton.load(std::memory_order_relaxed); 
            !sin && !singleton.compare_exchange_weak(sin, new Logger(), std::memory_order_acquire, std::memory_order_relaxed);) { }
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

    TLL_INLINE void init_()
    {
        if(isRunning())
            return;

        for(auto &entry : log_ents_)
        {
            auto &name = entry.first;
            auto &log_ent = entry.second;
            if(log_ent.chunk_size > 0) buffers_[name].reserve(log_ent.chunk_size*2);

            if(log_ent.open && log_ent.handle == nullptr)
                log_ent.handle = log_ent.open();
        }
    }

    template <typename ... LogEnts>
    void addLogEnt_(LogEntity log_ent, LogEnts ...log_ents)
    {
        log_ents_[log_ent.name] = log_ent;
        addLogEnt_(log_ents...);
    }

    TLL_INLINE void addLogEnt_(LogEntity log_ent)
    {
        log_ents_[log_ent.name] = log_ent;
    }

    TLL_INLINE void send_()
    {
        for(auto &entry : log_ents_)
        {
            send_(entry.first);
        }
    }

    TLL_INLINE void send_(const std::string &name)
    {
        auto &log_ent = log_ents_[name];
        auto &buff = buffers_[name];
        log_ent.send(log_ent.handle, buff.data(), buff.size());
        buff.resize(0);
    }

    TLL_INLINE void send_(const std::string &name,
            int type, 
            const std::string &buff)
    {
        auto &log_ent = log_ents_[name];
        if(! (log_ent.flag & toFLag(LogType(type))) ) return;
        log_ent.send(log_ent.handle, buff.data(), buff.size());
    }

    bool async_;
    uint32_t wait_ms_;
    std::atomic<bool> is_running_;
    std::thread broadcast_;
    std::unordered_map<std::string, LogEntity> log_ents_;
    std::unordered_map<std::string, std::vector<char>> buffers_;
    std::condition_variable pop_wait_, join_wait_;
    BSDLFQ<LogInfo> ring_queue_;
    std::mutex mtx_;
};

} // tll

#define LOG_HEADER_ tll::stringFormat("{%.9f}{%s}{%d}{%s}{%s}{%s}{%d}",\
  tll::timestamp<double>(),\
  tll::to_string(tll::tid()),\
  tll::StackMap::instance().level(tll::tid()),\
  tll::StackMap::instance().get<tll::StackMap::Pos::Previous>(tll::tid()),\
  tll::StackMap::instance().get<tll::StackMap::Pos::Current>(tll::tid()),\
  __FUNCTION__,\
  __LINE__)

#define TLL_LOGD(plog, format, ...) (plog)->log(static_cast<uint32_t>(tll::LogType::kDebug), "%s{" format "}\n", LOG_HEADER_ , ##__VA_ARGS__)

#define TLL_LOGI(plog, format, ...) (plog)->log(static_cast<uint32_t>(tll::LogType::kInfo), "%s{" format "}\n", LOG_HEADER_ , ##__VA_ARGS__)

#define TLL_LOGW(plog, format, ...) (plog)->log(static_cast<uint32_t>(tll::LogType::kWarn), "%s{" format "}\n", LOG_HEADER_ , ##__VA_ARGS__)

#define TLL_LOGF(plog, format, ...) (plog)->log(static_cast<uint32_t>(tll::LogType::kFatal), "%s{" format "}\n", LOG_HEADER_ , ##__VA_ARGS__)

#define TLL_LOGT(plog, ID) tll::Timer timer_##ID##_([plog](std::string const &log_msg){(plog)->log(static_cast<uint32_t>(tll::LogType::kTrace), "%s", log_msg);}, (LOG_HEADER_), (/*(logger).log(static_cast<uint32_t>(tll::LogType::kTrace), "%s\n", LOG_HEADER_),*/ tll::StackMap::instance()[tll::tid()].push_back(tll::fileName(__FILE__)), #ID))

#define TLL_LOGTF(plog) tll::Timer timer__([plog](std::string const &log_msg){(plog)->log(static_cast<uint32_t>(tll::LogType::kTrace), "%s", log_msg);}, (LOG_HEADER_), (/*(logger).log(static_cast<uint32_t>(tll::LogType::kTrace), "%s\n", LOG_HEADER_),*/ tll::StackMap::instance()[tll::tid()].push_back(tll::fileName(__FILE__)), __FUNCTION__))

#define TLL_GLOGD(...) TLL_LOGD(&tll::Logger::instance(), ##__VA_ARGS__)
#define TLL_GLOGI(...) TLL_LOGI(&tll::Logger::instance(), ##__VA_ARGS__)
#define TLL_GLOGW(...) TLL_LOGW(&tll::Logger::instance(), ##__VA_ARGS__)
#define TLL_GLOGF(...) TLL_LOGF(&tll::Logger::instance(), ##__VA_ARGS__)
#define TLL_GLOGT(ID) tll::Timer timer_##ID##_([](std::string const &log_msg){tll::Logger::instance().log(static_cast<uint32_t>(tll::LogType::kTrace), "%s", log_msg);}, (tll::StackMap::instance()[tll::tid()].push_back(tll::fileName(__FILE__)), LOG_HEADER_), (#ID))

#define TLL_GLOGTF() tll::Timer timer__([](std::string const &log_msg){tll::Logger::instance().log(static_cast<uint32_t>(tll::LogType::kTrace), "%s", log_msg);}, (tll::StackMap::instance()[tll::tid()].push_back(tll::fileName(__FILE__)), LOG_HEADER_), (__FUNCTION__))

#ifdef STATIC_LIB
#include "logger.cc"
#endif