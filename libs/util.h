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


#define _NUM_ARGS(X100, X99, X98, X97, X96, X95, X94, X93, X92, X91, X90, X89, X88, X87, X86, X85, X84, X83, X82, X81, X80, X79, X78, X77, X76, X75, X74, X73, X72, X71, X70, X69, X68, X67, X66, X65, X64, X63, X62, X61, X60, X59, X58, X57, X56, X55, X54, X53, X52, X51, X50, X49, X48, X47, X46, X45, X44, X43, X42, X41, X40, X39, X38, X37, X36, X35, X34, X33, X32, X31, X30, X29, X28, X27, X26, X25, X24, X23, X22, X21, X20, X19, X18, X17, X16, X15, X14, X13, X12, X11, X10, X9, X8, X7, X6, X5, X4, X3, X2, X1, N, ...)   N

#define NUM_ARGS(...) _NUM_ARGS(__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define EXPAND(X)       X
#define FIRSTARG(X, ...)    (X)
#define RESTARGS(X, ...)    (__VA_ARGS__)
#define FOREACH(MACRO, LIST)    FOREACH_(NUM_ARGS LIST, MACRO, LIST)
#define FOREACH_(N, M, LIST)    FOREACH__(N, M, LIST)
#define FOREACH__(N, M, LIST)   FOREACH_##N(M, LIST)
#define FOREACH_1(M, LIST)  M LIST, LIST
#define FOREACH_2(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_1(M, RESTARGS LIST)
#define FOREACH_3(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_2(M, RESTARGS LIST)
#define FOREACH_4(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_3(M, RESTARGS LIST)
#define FOREACH_5(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_4(M, RESTARGS LIST)
#define FOREACH_6(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_5(M, RESTARGS LIST)
#define FOREACH_7(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_6(M, RESTARGS LIST)
#define FOREACH_8(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_7(M, RESTARGS LIST)
#define FOREACH_9(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_8(M, RESTARGS LIST)
#define FOREACH_10(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_9(M, RESTARGS LIST)
#define FOREACH_11(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_10(M, RESTARGS LIST)
#define FOREACH_12(M, LIST)  EXPAND(M FIRSTARG LIST), FIRSTARG LIST, FOREACH_11(M, RESTARGS LIST)

#define STRINGIFY_(X)    #X
#define STRINGIFY(...)    FOREACH(STRINGIFY_, (__VA_ARGS__))

#define DUMPVAR(...)    dumpVar(STRINGIFY(__VA_ARGS__))

#define LOGPD(format, ...) PRINTF("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) PRINTF("%.9f\t%s\t%s:%s:%d\t" format "\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(format, ...) PRINTF("(E)(%.9f)(%s)(%s:%s:%d)(" format ")(%s)\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__, strerror(errno))


#define LOGV(...)   LOGD("%s", tll::util::toString(#__VA_ARGS__).data())
// #define LOGV(var) tll::util::Write(PRINTF, "%\t%\t%:%:%\t" #var ":{%}\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, var)

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
    static const thread_local auto ret=tll::util::to_string(std::this_thread::get_id());
    return ret;
}

inline std::string str_tidcpu()
{
    static std::atomic<int> __tidcpu__{0};
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

template <typename Dur=std::chrono::duration<double, std::ratio<1>>, typename Clk=std::chrono::steady_clock>
typename Dur::rep timestamp(const typename Clk::time_point &t = Clk::now())
{
    return std::chrono::duration_cast< Dur >( (t - begin_) ).count();
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


inline std::string toString(bool val) {return val ? "true" : "false";}
inline std::string toString(char val) {return stringFormat("%c", val);}
inline std::string toString(uint8_t val) {return stringFormat("0x%x", val);}
inline std::string toString(int16_t val) {return stringFormat("%d", val);}
inline std::string toString(uint16_t val) {return stringFormat("0x%x", val);}
inline std::string toString(int32_t val) {return stringFormat("%d", val);}
inline std::string toString(uint32_t val) {return stringFormat("0x%x", val);}
inline std::string toString(int64_t val) {return stringFormat("%ld", val);}
inline std::string toString(uint64_t val) {return stringFormat("0x%lx", val);}

inline std::string toString(float val) {return stringFormat("%f", val);}
inline std::string toString(double val) {return stringFormat("%f", val);}

inline std::string toString(void *val) {return stringFormat("%p", val);}
inline std::string toString(char const * val) {LOGD("%s", val);return stringFormat("%s", val);}
inline std::string toString(std::string const &val) {return val;}

template <typename T>
std::string toString(std::vector<T> const &val)
{
    std::string ret;
    for(const auto &el : val)
        ret += stringFormat("{%s}", toString(el).data());
    return ret;
}

template <typename T>
std::string toString(const char *name, const T &t)
{
    return stringFormat("{%s:%s}", name, toString(t).data());
}

template <typename F>
std::string dumpVar(const char *name, const F &f)
{
    return toString(name, f);
}

template <typename F, typename ... Args>
std::string dumpVar(const char *name, const F &f, const Args& ... args)
{
    return toString(name, f) + dumpVar(args...);
}

}} /// tll::util
