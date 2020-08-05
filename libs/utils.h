#pragma once

#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <sstream>
#include <atomic>
#include <cstring>

#include <memory>
#include <functional>
#include <list>
#include <algorithm>
#include <cassert>

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

namespace tll::util
{
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

inline bool isPowerOf2(uint32_t val)
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

/// single thread
/// no overrun/underrun check
// struct ContiRB
// {
// public:
//     ContiRB(size_t size) : head(0), tail(0)
//     {
//         wmark = isPowerOf2(size) ? size : nextPowerOf2(size);
//         buffer(wmark);
//     }

//     inline bool push(/*char *data, */size_t size)
//     {
//         /// 1) wrap & real: head < tail < wmark (*)
//         /// 2)        real: head < wmark < tail
//         ///           wrap: tail < head < wmark
//         /// 3) real & wrap: wmark < head < tail (*)
//         assert(size < size());
//         size_t next_tail = tail + size;
//         /// TODO: overrun checking
//         if(tail <= wmark) /// 1)
//         {
//             if(next_tail > wmark) /// rollover
//             {
//                 wmark = tail;
//                 next_tail += unused();
//             }
//         }
//         else /// 2 & 3)
//         {
//             if(head >= wmark) /// 3)
//             {
//                 /// move wmark to the end of the buffer
//                 wmark = tail + buffer.size() - wrap(tail);
//             }
//         }

//         tail = next_tail;
//         return true;
//     }

//     inline size_t pop(size_t size)
//     {
//         /// 1) wrap & real: head < tail < wmark (*)
//         /// 2)        real: head < wmark < tail
//         ///           wrap: tail < head < wmark
//         /// 3) real & wrap: wmark < head < tail (*)
//         size_t next_head;
//         if(tail <= wmark) /// 1)
//         {
//             size = size > size() ? size() : size;
//             next_head = head + size;
//         }
//         else /// 2 & 3)
//         {
//             if(head < wmark) /// 2)
//             {
//                 size = size > (wmark - head) ? (wmark - head) : size;
//                 next_head = head + size;
//             }
//             else /// 3) rollover
//             {
//                 size_t begin = tail - wrap(tail);
//                 size = size > size() ? size() : size;
//                 next_head = begin + size;
//             }
//         }

//         head = next_head == wmark ? 0 : next_head;
//         return size;
//     }

//     inline size_t size()
//     { 
//         return tail - head - unused();
//     }

//     inline size_t unused()
//     {
//         return head <= wmark ? buffer.size() - wmark : 0;
//     }

//     inline size_t wrap(size_t index)
//     {
//         return index & (buffer.size() - 1);
//     }

//     inline bool isEmpty() { return size() == 0; }
//     inline bool isFull() { return size() == (buffer.size() - unused()); }

//     size_t head, tail, wmark;
//     std::vector<char> buffer;
// };

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
        auto it = std::find_if(slots_.begin(), slots_.end(),
            [id](const Slot &slot)
            {return id == (uintptr_t)(&slot);}
            );
        if(it == slots_.end()) return false;
        slots_.erase(it);
        return true;
    }

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
    std::list<Slot> slots_;
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

} /// tll::util