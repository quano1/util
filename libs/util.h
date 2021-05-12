/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <cstring>
#include <cassert>
#include <string>

#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <algorithm>
#include <utility>

#include <vector>
#include <list>
#include <array>

// #include "timer.h"

#ifdef __ANDROID__
#include <android/log.h>
#define PRINTF(...) __android_log_print(ANDROID_LOG_DEBUG, "TLL", __VA_ARGS__)
#endif

#ifndef STATIC_LIB
#define TLL_INLINE inline
#else
#define TLL_INLINE
#endif

#ifndef PRINTF
#define PRINTF printf
#endif

#define ASSERTM(exp, msg) assert(((void)msg, exp))

#define LOGPD(format, ...) PRINTF("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) PRINTF("%.9f\t%s\t%s:%s:%d\t" format "\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(format, ...) PRINTF("(E)(%.9f)(%s)(%s:%s:%d)(" format ")(%s)\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__, strerror(errno))

#define TRACE(ID) tll::log::Tracer<> tracer_##ID##__(#ID)
#define TRACEF() tll::log::Tracer<> tracer__(std::string(__FUNCTION__) + ":" + std::to_string(__LINE__) + "(" + tll::util::to_string(tll::util::tid()) + ")")

namespace tll{ namespace util{
// #define THIS_THREAD_ID_ std::this_thread::get_id()
// using this_tid std::this_thread::get_id();
template <class T> class Guard;
template <typename T, typename ... Args>
std::basic_string<T> stringFormat(T const * const format, Args const & ... args);

template <typename T=size_t>
constexpr T nextPowerOf2(T val)
{
    val--;
    val |= val >> 1;
    val |= val >> 2;
    if(sizeof(T) >= 1)
        val |= val >> 4;
    if(sizeof(T) >= 2)
        val |= val >> 8;
    if(sizeof(T) >= 4)
        val |= val >> 16;
    if(sizeof(T) > 4)
        val |= val >> 32;
    val++;
    return val;
}

template <typename T=size_t>
constexpr bool isPowerOf2(T val)
{
    return (val & (val - 1)) == 0;
}

template<typename T, std::size_t N, std::size_t... I>
constexpr auto make_array_impl(T && value, std::index_sequence<I...>)
{
    return std::array<std::decay_t<T>, N> {
            (static_cast<void>(I), std::forward<T>(value))..., 
        };
}

template<std::size_t N, typename T>
constexpr auto make_array(T && value)
{
    return make_array_impl<T, N>(std::forward<T>(value), std::make_index_sequence<N>{});
}

template <size_t end, typename T = std::size_t>
constexpr T generate_ith_number(const std::size_t index) {
    static_assert(std::is_integral<T>::value, "T must to be an integral type");
    if(index == 0) return 1;
    if(index == 1) return end/2;
    // if(index == 2) return end;
    // if(index <= (end / 2)) return (index * 2);
    return end * (index - 1);
    // return end * pow(2, index - 2);
}

template <size_t end, std::size_t... Is> 
constexpr auto make_sequence_impl(std::index_sequence<Is...>)
{
    return std::index_sequence<generate_ith_number<end>(Is)...>{};
}

template <std::size_t N, size_t extended> 
constexpr auto make_sequence()
{
    return make_sequence_impl<N>(std::make_index_sequence<3 + extended>{});
}

template <std::size_t... Is>
constexpr auto make_array_from_sequence_impl(std::index_sequence<Is...>)
{
    return std::array<std::size_t, sizeof...(Is)>{Is...};
}

template <typename Seq>
constexpr auto make_array_from_sequence(Seq)
{
    return make_array_from_sequence_impl(Seq{});
}

template<class F, size_t... ints>
constexpr void CallFuncInSeq(F f, std::integer_sequence<size_t, ints...>)
{
    (f(std::integral_constant<size_t, ints>{}),...);
}

template<size_t end, size_t extended, class F>
constexpr auto CallFuncInSeq(F f)
{
    // return CallFuncInSeq<>(f, std::make_integer_sequence<size_t, end>{});
    return CallFuncInSeq<>(f, make_sequence<end, extended>());
}

inline std::thread::id tid()
{
    static const thread_local auto tid=std::this_thread::get_id();
    return tid;
    // return std::this_thread::get_id();
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

inline std::string str_tid()
{
    // static std::atomic<int> __tid__{0};
    // static const thread_local auto ret = tll::util::to_string(__tid__.fetch_add(1, std::memory_order_relaxed));
    static const thread_local auto ret=tll::util::to_string(std::this_thread::get_id());
    return ret;
}

// static int __tid__=0;

inline std::string str_tidcpu()
{
    static std::atomic<int> __tidcpu__{0};
    // static const thread_local auto ret = tll::util::to_string(__tidcpu__.fetch_add(1, std::memory_order_relaxed));
    // static const thread_local auto ret = tll::util::stringFormat("%s/%d", tll::util::to_string(tll::util::tid()).data(), sched_getcpu());
    static const thread_local auto ret = tll::util::stringFormat("%s/%d", tll::util::to_string(__tidcpu__.fetch_add(1, std::memory_order_relaxed)).data(), sched_getcpu());
    return ret;
}

inline std::string str_tid_nice()
{
    static std::atomic<int> __tid__{0};
    static const thread_local auto ret = tll::util::to_string(__tid__.fetch_add(1, std::memory_order_relaxed));
    return ret;
}

static const auto begin_ = std::chrono::steady_clock::now();
// static const auto tse = begin_.time_since_epoch();

// template <typename T=double, typename D=std::ratio<1,1>, typename C=std::chrono::steady_clock>
// template <typename Dur=std::chrono::duration<double, std::ratio<1>>, typename Clk=std::chrono::steady_clock>

template <typename Dur=std::chrono::duration<double, std::ratio<1>>, typename Clk=std::chrono::steady_clock>
typename Dur::rep timestamp(const typename Clk::time_point &t = Clk::now())
{
    // static const auto tse = typename C::time_point(t.time_since_epoch());
    return std::chrono::duration_cast< Dur >( (t - begin_) ).count();
    // return std::chrono::duration_cast< Dur >( (t - begin_) + tse ).count();
    // return std::chrono::duration_cast< Dur >(t.time_since_epoch()).count();
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


// inline int countConsZero(uint64_t v)
// {
//     return ffsll((v >> pos) | (v << (64 - pos))) - 1;
// }

// inline int countConsZero(uint32_t v, int pos)
// {
//     // unsigned int v;  // find the number of trailing zeros in 32-bit v 
//     // int r;           // result goes here

//     v >>= pos;

//     static const int MultiplyDeBruijnBitPosition[32] = 
//     {
//       0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
//       31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
//     };
//     return MultiplyDeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];
// }

// inline uint32_t nextPowerOf2(uint32_t val)
// {
//     val--;
//     val |= val >> 1;
//     val |= val >> 2;
//     val |= val >> 4;
//     val |= val >> 8;
//     val |= val >> 16;
//     val++;
//     return val;
// }

// inline bool isPowerOf2(uint32_t val)
// {
//     return (val & (val - 1)) == 0;
// }

/// Continuous ring buffer
/// no thread-safe, single only
struct ContiRB
{
public:
    ContiRB() = default;

    ContiRB(size_t size)
    {
        reserve(size);
    }

    inline void dump() const
    {
        LOGD("sz:%ld ph:%ld pt:%ld wm:%ld ch:%ld ct:%ld", buffer_.size(), 
             ph_.load(std::memory_order_relaxed), pt_.load(std::memory_order_relaxed), 
             wm_.load(std::memory_order_relaxed), 
             ch_.load(std::memory_order_relaxed), ct_.load(std::memory_order_relaxed));
    }

    inline void reset(size_t new_size=0)
    {
        ph_.store(0,std::memory_order_relaxed);
        ch_.store(0,std::memory_order_relaxed);
        pt_.store(0,std::memory_order_relaxed);
        ct_.store(0,std::memory_order_relaxed);
        wm_.store(0,std::memory_order_relaxed);
        if(new_size)
            reserve(new_size);
    }

    inline void reserve(size_t size)
    {
        size = isPowerOf2(size) ? size : nextPowerOf2(size);
        buffer_.resize(size);
    }

    inline char *tryPop(size_t &cons, size_t &size)
    {
        cons = ch_.load(std::memory_order_relaxed);
        size_t next_cons = cons;
        for(;;)
        {
            size_t prod = pt_.load(std::memory_order_relaxed);
            size_t wmark = wm_.load(std::memory_order_acquire);
            next_cons = cons;
            if(cons == prod)
            {
                // LOGE("Underrun!");dump();
                size = 0;
                return nullptr;
            }

            if(cons == wmark)
            {
                next_cons = next(cons);
                if(prod <= next_cons)
                {
                    // LOGE("Underrun!");dump();
                    size = 0;
                    return nullptr;
                }
                if(size > (prod - next_cons))
                    size = prod - next_cons;
            }
            else if(cons < wmark)
            {
                if(size > (wmark - cons))
                    size = wmark - cons;
            }
            else /// cons > wmark
            {
                if(size > (prod - cons))
                    size = prod - cons;
            }

            if(ch_.compare_exchange_weak(cons, next_cons + size, std::memory_order_relaxed, std::memory_order_relaxed)) break;
        }
        return buffer_.data() + wrap(next_cons);
    }

    inline void completePop(size_t cons, size_t size)
    {
        for(;ct_.load(std::memory_order_relaxed) != cons;)
        {}

        if(cons == wm_.load(std::memory_order_relaxed))
            ct_.store(next(cons) + size, std::memory_order_release);
        else
            ct_.store(cons + size, std::memory_order_release);
    }

    inline bool pop(char *dst, size_t &size)
    {
        // size_t offset;
        size_t cons;
        char *src = tryPop(cons, size);
        if(src != nullptr)
        {
            memcpy(dst, src, size);
            completePop(cons, size);

            return true;
        }

        return false;
    }

    inline char *tryPush(size_t &prod, size_t size)
    {
        prod = ph_.load(std::memory_order_relaxed);
        for(;;)
        {
            size_t wmark = wm_.load(std::memory_order_relaxed);
            size_t cons = ct_.load(std::memory_order_acquire);
            if(size <= buffer_.size() - (prod - cons - unused()))
            {
                /// prod leads
                if(wrap(prod) >= wrap(cons))
                {
                    /// not enough space    ?
                    if(size > buffer_.size() - wrap(prod))
                    {
                        if(size <= wrap(cons))
                        {
                            if(!ph_.compare_exchange_weak(prod, next(prod) + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                            if(!wm_.compare_exchange_weak(wmark, prod, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                            return buffer_.data() + wrap(next(prod));
                        }
                        else
                        {
                            // LOGE("OVERRUN");dump();
                            return nullptr;
                        }
                    }
                    else
                    {
                        if(!ph_.compare_exchange_weak(prod, prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                        else break;
                    }
                }
                /// cons leads
                else
                {
                    if(size > wrap(cons) - wrap(prod))
                    {
                        // LOGE("OVERRUN");dump();
                        return nullptr;
                    }
                    if(!ph_.compare_exchange_weak(prod, prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                    else break;
                }
            }
            else
            {
                // LOGE("OVERRUN");dump();
                return nullptr;
            }
        }
        // wsize_ = size;
        return buffer_.data() + wrap(prod);
    }

    inline void completePush(size_t prod, size_t size)
    {
        for(;pt_.load(std::memory_order_relaxed) != prod;)
        {}

        if(wrap(prod) == 0) wm_.store(prod, std::memory_order_relaxed);

        if(prod == wm_.load(std::memory_order_relaxed))
            pt_.store(next(prod) + size, std::memory_order_release);
        else
            pt_.store(prod + size, std::memory_order_release);
        // dump();
    }

    inline bool push(const char *src, size_t size)
    {
        // size_t offset;
        size_t prod;
        char *dst = tryPush(prod, size);
        if(dst)
        {
            memcpy(dst, src, size);
            completePush(prod, size);
            return true;
        }

        return false;
    }


    inline size_t wrap(size_t index) const
    {
        return index & (buffer_.size() - 1);
    }

    inline size_t size() const
    {
        return pt_.load(std::memory_order_relaxed) - ch_.load(std::memory_order_relaxed) - unused();
    }

    inline size_t freeSize() const
    {
        return buffer_.size() - (ph_.load(std::memory_order_relaxed) - ct_.load(std::memory_order_relaxed) - unused());
    }

    inline size_t unused() const
    {
        size_t cons = ct_.load(std::memory_order_relaxed);
        size_t wmark = wm_.load(std::memory_order_relaxed);
        return cons < wmark ? buffer_.size() - wrap(wmark) : 0;
    }

    inline size_t next(size_t index) const
    {
        size_t tmp = buffer_.size() - 1;
        return (index + tmp) & (~tmp);
        // return wrap(index) ? index + (buffer_.size() - wrap(index)) : index;
    }

    // inline bool isEmpty() { return this->size() == 0; }
    // inline bool isFull() { return this->size() == (buffer.size() - unused()); }

    std::atomic<size_t> ph_{0}, pt_{0}, wm_{0}, ch_{0}, ct_{0};
    // size_t rsize_ = 0, wsize_ = 0, woff_ = 0, roff_=0;
    std::vector<char> buffer_{}; /// 1Kb
    // std::mutex mtx;
};

/// lock-free queue
template <typename T, size_t const kELemSize=sizeof(T)>
class LFQueue
{
public:
    LFQueue(uint32_t num_of_elem)
    {
        capacity_ = isPowerOf2(num_of_elem) ? num_of_elem : nextPowerOf2(num_of_elem);
        buffer_.resize(capacity_ * kELemSize);
    }

    TLL_INLINE bool tryPop(uint32_t &cons)
    {
        cons = ch_.load(std::memory_order_relaxed);

        for(;!isEmpty();)
        {
            if (cons == pt_.load(std::memory_order_relaxed))
                return false;

            if(ch_.compare_exchange_weak(cons, cons + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }

        return false;
    }

    TLL_INLINE bool completePop(uint32_t cons)
    {
        if (ct_.load(std::memory_order_relaxed) != cons)
            return false;

        ct_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template </*typename D,*/ typename F, typename ...Args>
    bool pop(F &&doPop, Args &&...args)
    {
        uint32_t cons;
        while(!tryPop(cons)) 
        {
            if (isEmpty()) 
                return false;

            std::this_thread::yield();
        }

        std::forward<F>(doPop)(elemAt(cons), kELemSize, std::forward<Args>(args)...);

        while(!completePop(cons)) 
            std::this_thread::yield();

        return true;
    }

    /// ONE consumer only
    template </*typename D,*/ typename F, typename ...Args>
    bool popBatch(uint32_t max_num, F &&onPopBatch, Args &&...args)
    {
        if (isEmpty())
            return false;

        uint32_t cons = ch_.load(std::memory_order_relaxed);
        uint32_t elem_num = size();

        if(elem_num > max_num)
            elem_num = max_num;

        while(cons + elem_num > pt_.load(std::memory_order_relaxed))
            std::this_thread::yield();

        ch_.fetch_add(elem_num, std::memory_order_acquire);

        std::forward<F>(onPopBatch)(cons, elem_num, kELemSize, std::forward<Args>(args)...);

        ct_.fetch_add(elem_num, std::memory_order_release);

        return true;
    }

    TLL_INLINE bool tryPush(uint32_t &prod)
    {
        prod = ph_.load(std::memory_order_relaxed);

        for(;!isFull();)
        {
            if (prod == (ct_.load(std::memory_order_relaxed) + capacity_))
                return false;

            if(ph_.compare_exchange_weak(prod, prod + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }
        return false;
    }

    TLL_INLINE bool completePush(uint32_t prod)
    {
        if (pt_.load(std::memory_order_relaxed) != prod)
            return false;

        pt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template </*typename D, */typename F, typename ...Args>
    bool push(F &&doPush, Args&&...args)
    {
        uint32_t prod;
        prod = ph_.load(std::memory_order_relaxed);
        while(!tryPush(prod))
        {
            if(isFull()) return false;
            std::this_thread::yield();
        }

        std::forward<F>(doPush)(elemAt(prod), kELemSize, std::forward<Args>(args)...);
        while(!completePush(prod)) 
            std::this_thread::yield();

        return true;
    }

    TLL_INLINE uint32_t size() const
    {
        return ph_.load(std::memory_order_relaxed) - ch_.load(std::memory_order_relaxed);
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

    std::atomic<uint32_t> pt_{0}, ph_{0}, ct_{0}, ch_{0};
    uint32_t capacity_{0};
    std::vector<T> buffer_{};
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

inline void dump(const char *ptr, size_t size, size_t off=0, size_t hl=-1, bool p_zero=true)
{
    for(size_t i=off; i<size; i++)
    {
        switch (ptr[i])
        {
            case 0:
            {
                if(p_zero)
                    printf(".");
            }
            break;
            case '{':
            case '}':
            case '|':
            {
                printf("%c", ptr[i]);
            }
            break;
            default:
            {
                if(i == hl)
                    printf("[%d]", ptr[i]);
                else if(ptr[i] != 0)
                    printf("%2d", ptr[i]);
            }
            break;
        }

        // if(i == hl)
        //     printf("[%X]", ptr[i]);
        // else if(ptr[i] != 0)
        //     printf("%2X", ptr[i]);
        // else
        //     printf("..");
    }
    printf("\n");
}

}} /// tll::util
