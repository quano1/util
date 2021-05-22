#pragma once
/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)


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


#define NUM_ARGS_(_12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N
#define NUM_ARGS(...) NUM_ARGS_(__VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define FOREACH(MACRO, ...) FOREACH_(NUM_ARGS(__VA_ARGS__), MACRO, __VA_ARGS__)
#define FOREACH_(N, M, ...) FOREACH__(N, M, __VA_ARGS__)
#define FOREACH__(N, M, ...) FOREACH_##N(M, __VA_ARGS__)
#define FOREACH_1(M, A) M(A), A
#define FOREACH_2(M, A, ...) M(A), A, FOREACH_1(M, __VA_ARGS__)
#define FOREACH_3(M, A, ...) M(A), A, FOREACH_2(M, __VA_ARGS__)
#define FOREACH_4(M, A, ...) M(A), A, FOREACH_3(M, __VA_ARGS__)
#define FOREACH_5(M, A, ...) M(A), A, FOREACH_4(M, __VA_ARGS__)
#define FOREACH_6(M, A, ...) M(A), A, FOREACH_5(M, __VA_ARGS__)
#define FOREACH_7(M, A, ...) M(A), A, FOREACH_6(M, __VA_ARGS__)
#define FOREACH_8(M, A, ...) M(A), A, FOREACH_7(M, __VA_ARGS__)
#define FOREACH_9(M, A, ...) M(A), A, FOREACH_8(M, __VA_ARGS__)
#define FOREACH_10(M, A, ...) M(A), A, FOREACH_9(M, __VA_ARGS__)
#define FOREACH_11(M, A, ...) M(A), A, FOREACH_10(M, __VA_ARGS__)
#define FOREACH_12(M, A, ...) M(A), A, FOREACH_11(M, __VA_ARGS__)

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)

#define VAR_STR(...)    tll::util::varStr(FOREACH(STRINGIFY, __VA_ARGS__))

#define LOGPD(format, ...) PRINTF("(D)(%.9f)(%s)(%s:%s:%d)(" format ")\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) PRINTF("%.9f\t%s\t%s:%s:%d\t" format "\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(format, ...) PRINTF("(E)(%.9f)(%s)(%s:%s:%d)(" format ")(%s)\n", tll::util::timestamp(), tll::util::str_tidcpu().data(), tll::util::fileName(__FILE__).data(), __FUNCTION__, __LINE__, ##__VA_ARGS__, strerror(errno))


#define LOGV(...)   LOGD("%s", VAR_STR(__VA_ARGS__).data())
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


template <typename T> std::string toString(const T&);

template <> inline std::string toString<bool>(const bool &val) {return val ? "true" : "false";}
template <> inline std::string toString<char>(const char &val) {return stringFormat("%c", val);}
template <> inline std::string toString<uint8_t>(const uint8_t &val) {return stringFormat("0x%x", val);}
template <> inline std::string toString<int16_t>(const int16_t &val) {return stringFormat("%d", val);}
template <> inline std::string toString<uint16_t>(const uint16_t &val) {return stringFormat("0x%x", val);}
template <> inline std::string toString<int32_t>(const int32_t &val) {return stringFormat("%d", val);}
template <> inline std::string toString<uint32_t>(const uint32_t &val) {return stringFormat("0x%x", val);}
template <> inline std::string toString<int64_t>(const int64_t &val) {return stringFormat("%ld", val);}
template <> inline std::string toString<uint64_t>(const uint64_t &val) {return stringFormat("0x%lx", val);}
template <> inline std::string toString<float>(const float &val) {return stringFormat("%f", val);}
template <> inline std::string toString<double>(const double &val) {return stringFormat("%f", val);}
template <> inline std::string toString<std::string>(const std::string &val) {return val;}

template <typename T>
std::string toString(const std::atomic<T> &val) {return toString<T>(val.load(std::memory_order_relaxed));}

template <typename T>
std::string toString(std::vector<T> const &val)
{
    std::string ret;
    for(const auto &el : val)
        ret += stringFormat("%s,", toString(el).data());
    return ret;
}

template <typename T>
std::string varStr(const char *name, const T &val)
{
     return stringFormat("%s={%s}", name, toString(val).data());
}

template <typename T, typename ... Args>
std::string varStr(const char *name, const T &val, const Args& ... args)
{
    return varStr(name, val) + " | " + varStr(args...);
}


////////////////////////////////////////////////////////////////////////////////
/// StreamBuffer

struct StaticLogInfo
{
    std::string file;
    std::string function;
    std::string format;
    int line=0;
    int severity=0;
    
    // stringFormat(T const * const format, Args const & ... args)

    // std::vector<std::string> subformats;

    // StaticLogInfo()
    // {
    //     std::size_t h_file = std::hash<std::string>{}(s.first_name);
    //     std::size_t h_function = std::hash<std::string>{}(s.first_name);
    //     id = h_file ^ (h_function << 1);
    // }
}; /// StaticLogInfo

struct DynamicLogInfo {
    StaticLogInfo *s_log_info;
    size_t timestamp;
    int tid;
    std::vector<uint8_t> args;
};

struct StreamBuffer {
    StreamBuffer(size_t reserve=0x400) {buff_.reserve(0x400);}
    ~StreamBuffer()=default;

    template <typename T>
    uint16_t getId();
    template <typename T>
    StreamBuffer &operator<< (const T *val);

    template <typename T>
    StreamBuffer &operator<< (T val)
    {
        // LOGPD("");
        uint8_t *crr_pos = buff_.data() + buff_.size();
        buff_.resize(buff_.size() + sizeof(T) + 2);
        *(uint16_t*)crr_pos = getId<T>(); /// primitive id
        crr_pos += 2;
        *(T*)crr_pos = val;

        return *this;
    }

    template <typename T>
    StreamBuffer &operator>> (T &val)
    {
        uint8_t *crr_pos = buff_.data() + buff_.size();
        crr_pos -= sizeof(T);
        val = *(T*)crr_pos;
        buff_.resize(buff_.size() - sizeof(T) - 2);
        return *this;
    }
    
    inline auto giveUp()
    {
        return std::move(buff_);
    }

    std::vector<uint8_t> buff_;
};

enum class TypeId : uint16_t {
    kInt8,
    kUint8,
    kInt16,
    kUint16,
    kInt32,
    kUint32,
    kInt64,
    kUint64,
    kFloat,
    kDouble,

    kCharPtr,
};

template <> inline uint16_t StreamBuffer::getId<int8_t>() {return (uint16_t)TypeId::kInt8;}
template <> inline uint16_t StreamBuffer::getId<uint8_t>() {return (uint16_t)TypeId::kUint8;}
template <> inline uint16_t StreamBuffer::getId<int16_t>() {return (uint16_t)TypeId::kInt16;}
template <> inline uint16_t StreamBuffer::getId<uint16_t>() {return (uint16_t)TypeId::kUint16;}
template <> inline uint16_t StreamBuffer::getId<int32_t>() {return (uint16_t)TypeId::kInt32;}
template <> inline uint16_t StreamBuffer::getId<uint32_t>() {return (uint16_t)TypeId::kUint32;}
template <> inline uint16_t StreamBuffer::getId<int64_t>() {return (uint16_t)TypeId::kInt64;}
template <> inline uint16_t StreamBuffer::getId<uint64_t>() {return (uint16_t)TypeId::kUint64;}
template <> inline uint16_t StreamBuffer::getId<float>() {return (uint16_t)TypeId::kFloat;}
template <> inline uint16_t StreamBuffer::getId<double>() {return (uint16_t)TypeId::kDouble;}
template <> inline uint16_t StreamBuffer::getId<const char*>() {return (uint16_t)TypeId::kCharPtr;}

template <>
inline StreamBuffer &StreamBuffer::operator<< <char>(const char *val)
{
    uint8_t *crr_pos = buff_.data() + buff_.size();
    auto str_len = strlen(val);
    buff_.resize(buff_.size() + str_len + 4);

    *(uint16_t*)crr_pos = getId<const char *>(); /// string id
    crr_pos += 2;

    *(uint16_t*)crr_pos = str_len;
    crr_pos +=2;

    memcpy(crr_pos, val, str_len);
    crr_pos += str_len;

    return *this;
}

template <typename D=std::chrono::duration<double, std::ratio<1>>, typename C=std::chrono::steady_clock>
class Counter
{
public:
    using Clock = C;
    using Duration = D;
    using Type = typename Duration::rep;
    using Ratio = typename Duration::period;
    using Tp = std::chrono::time_point<C,D>;
private:
    Tp begin_=std::chrono::time_point_cast<D>(Clock::now());
    Tp abs_begin_=begin_;

    Duration total_elapsed_{0}, last_elapsed_{0};
public:
    Counter() = default;
    ~Counter() = default;

    auto &start(Tp tp=std::chrono::time_point_cast<D>(Clock::now()))
    {
        begin_ = tp;
        return *this;
    }

    void reset(Tp tp=std::chrono::time_point_cast<D>(Clock::now()))
    {
        begin_ = tp;
        abs_begin_ = begin_;
        total_elapsed_ = Duration::zero();
    }

    Duration elapse() const
    {
        return std::chrono::time_point_cast<D>(Clock::now()) - begin_;
    }

    auto elapsed()
    {
        last_elapsed_ = elapse();
        total_elapsed_ += last_elapsed_;
        return last_elapsed_;
    }

    const Tp &absBegin() const
    {
        return abs_begin_;
    }

    Duration lastElapsed() const
    {
        return last_elapsed_;
    }

    Duration totalElapsed() const
    {
        return total_elapsed_;
    }

    Duration absElapse() const
    {
        return std::chrono::time_point_cast<D>(Clock::now()) - abs_begin_;
    }
}; /// Counter


}} /// tll::util
