#pragma once

#include <cstring>
#include <cassert>
#include <string>
#include <sstream>

#include <vector>
#include <list>

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <algorithm>

#ifndef STATIC_LIB
#define TLL_INLINE inline
#else
#define TLL_INLINE
#endif

#define ASSERTM(exp, msg) assert(((void)msg, exp))

#define LOGPD(format, ...) printf("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::util::timestamp(), tll::util::to_string(tll::util::tid()).data(), tll::util::fileName(__FILE__).data(), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) printf("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::util::timestamp(), tll::util::to_string(tll::util::tid()).data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(format, ...) printf("(E)(%.9f)(%s)(%s:%s:%d)(" format ")(%s)\n", tll::util::timestamp(), tll::util::to_string(tll::util::tid()).data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__, strerror(errno))

#define TRACE(ID) tll::log::Tracer<> tracer_##ID##__(#ID)
#define TRACEF() tll::log::Tracer<> tracer__(std::string(__FUNCTION__) + ":" + std::to_string(__LINE__) + "(" + tll::util::to_string(tll::util::tid()) + ")")

namespace tll{ namespace util{
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

inline bool isPowerOf2(uint32_t val)
{
    return (val & (val - 1)) == 0;
}

/// Continuous ring buffer
/// no thread-safe, single only
struct ContiRB
{
public:
    ContiRB() = default;

    ContiRB(size_t size) : head(0), tail(0), wmark(0)
    {
        size = isPowerOf2(size) ? size : nextPowerOf2(size);
        buffer.resize(size);
    }

    inline void dump() const
    {
        LOGD("%ld %ld %ld", head, tail, wmark);
    }

    inline void reserve(size_t size)
    {
        size = isPowerOf2(size) ? size : nextPowerOf2(size);
        buffer.resize(size);
    }

    inline char *popHalf(size_t &size, size_t &offset)
    {
        if(isEmpty()) return nullptr;
        offset = 0;
        /// tail is rollover?
        if(head < wmark)
        {
            if(size >= (wmark - head))
            {
                size = wmark - head;
                offset = unused();
            }
        }
        else
        {
            if(size > (tail - head))
            {
                size = tail - head;
            }
        }

        return buffer.data() + wrap(head);
    }

    inline void completePop(size_t size, size_t offset)
    {
        head += size + offset;
    }

    inline char *pushHalf(size_t &size, size_t &offset)
    {
        if(isFull()) return nullptr;
        offset = 0;
        /// rollover?
        if(size > next(tail))
        {
            offset = next(tail);
        }
        /// enough space?
        if(size <= buffer.size() - this->size() - offset)
        {
            return buffer.data() + wrap(tail + offset);
        }
        else
        {
            /// overrun
            assert((false) && "Ooops! overrun");
        }

        return nullptr;
    }

    inline void completePush(size_t size, size_t offset)
    {
        if(offset) {
            wmark = tail;
        }
        tail = tail + offset + size;
    }


    inline size_t push(const char *data, size_t size)
    {
        size_t offset;
        char *ptr = pushHalf(size, offset);
        if(ptr)
        {
            memcpy(ptr, data, size);
            completePush(size, offset);
        }

        return size;
    }

    inline size_t pop(char *data, size_t size)
    {
        size_t offset;
        char *ptr = popHalf(size, offset);
        if(ptr != nullptr)
        {
            memcpy(data, ptr, size);
            completePop(size, offset);
        }
        return size;
    }

    inline size_t size() const
    {
        return tail - head - unused();
    }

    inline size_t unused() const
    {
        return head < wmark ? buffer.size() - wrap(wmark) : 0;
    }

    inline size_t wrap(size_t index) const
    {
        return index & (buffer.size() - 1);
    }

    inline size_t next(size_t index) const
    {
        return buffer.size() - wrap(index);
    }

    inline bool isEmpty() { return this->size() == 0; }
    inline bool isFull() { return this->size() == (buffer.size() - unused()); }

    size_t head, tail, wmark;
    std::vector<char> buffer;
    std::mutex mtx;
};

/// lock-free queue
template <typename T, size_t const kELemSize=sizeof(T)>
class LFQueue
{
public:
    LFQueue(uint32_t num_of_elem) : prod_tail_(0), prod_head_(0), cons_tail_(0), cons_head_(0)
    {
        capacity_ = isPowerOf2(num_of_elem) ? num_of_elem : nextPowerOf2(num_of_elem);
        buffer_.resize(capacity_ * kELemSize);
    }

    TLL_INLINE bool tryPop(uint32_t &cons_head)
    {
        cons_head = cons_head_.load(std::memory_order_relaxed);

        for(;!isEmpty();)
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
            if (isEmpty()) 
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
        if (isEmpty())
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

        for(;!isFull();)
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
            if(isFull()) return false;
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

    TLL_INLINE bool isEmpty() const {
        return size() == 0;
    }

    TLL_INLINE bool isFull() const {
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

// template <typename Signature>
// class Slot
// {
// public:
//     Slot()=default;
//     ~Slot()=default;
//     Slot(std::function<Signature> cb) : cb_(cb) {}
//     operator bool() const
//     {
//         return cb_ != nullptr;
//     }

//     template <typename ...Args>
//     void operator()(Args &&...args) const
//     {
//         cb_(std::forward<Args>(args)...);
//     }
// private:
//     std::function<Signature> cb_;
// };

template <typename, typename...> class Signal;

template <typename R, class... Args>
class Signal<R (Args...)>
{
public:
    using Slot = std::function<R (Args...)>;
    uintptr_t connect(Slot slot)
    {
        slots_.emplace_back(slot);
        return (uintptr_t)(&(slots_.back()));
    }

    bool disconnect(uintptr_t id)
    {
        std::lock_guard<std::mutex> guard{mtx_};
        auto it = std::find_if(slots_.begin(), slots_.end(),
            [id](const Slot &slot)
            {return id == (uintptr_t)(&slot);}
            );
        if(it == slots_.end()) return false;
        slots_.erase(it);
        return true;
    }

    void emit(Args &&...args)
    {
        std::lock_guard<std::mutex> guard{mtx_};
        for(auto &slot : slots_)
        {
            if(slot)
            {
                slot(std::forward<Args>(args)...);
            }
        }
    }
private:
    std::list<Slot> slots_;
    std::mutex mtx_;
};

template <class T> class Guard;

template <class T>
class Guard
{
public:
    Guard(T &t) : t_(t) {t_.start();}
    ~Guard() {t_.stop();}
private:
    T &t_;
}; /// Guard ref

template <class T>
class Guard<T*>
{
public:
    Guard(T *t) : t_(t) {t_->start();}
    ~Guard() {t_->stop();}
private:
    T *t_;
}; /// Guard pointer

template <class T>
struct SingletonT
{
static T &instance()
{
    static std::atomic<T*> singleton{nullptr};
    static std::atomic<bool> init{false};

    if (singleton.load(std::memory_order_relaxed))
        return *singleton.load(std::memory_order_acquire);

    if(init.exchange(true))
    {
        while(!singleton.load(std::memory_order_relaxed)){}
    }
    else
    {
        singleton.store(new T{}, std::memory_order_release);
    }

    return *singleton.load(std::memory_order_acquire);
}

};

}} /// tll::util