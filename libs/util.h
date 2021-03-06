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

#define TRACE(ID) tll::log::Tracer<> tracer_##ID##__(#ID)
#define TRACEF() tll::log::Tracer<> tracer__(std::string(__FUNCTION__) + ":" + std::to_string(__LINE__) + "(" + tll::util::to_string(tll::util::tid()) + ")")

namespace tll{ namespace util{

template <class T> class Guard;
template <typename ... Args>
std::string stringFormat(const char *format, Args const & ... args);

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

/// https://stackoverflow.com/questions/57756557/initializing-a-stdarray-with-a-constant-value
namespace detail
{
    template <typename T, std::size_t ... Is>
    constexpr std::array<T, sizeof...(Is)>
    create_array(T value, std::index_sequence<Is...>)
    {
        // cast Is to void to remove the warning: unused value
        return {{(static_cast<void>(Is), value)...}};
    }
}

template <typename T, std::size_t N>
constexpr std::array<T, N> create_array(const T& value)
{
    return detail::create_array(value, std::make_index_sequence<N>());
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
}

inline int tid_nice()
{
    static std::atomic<int> __tid__{0};
    static const thread_local auto ret = __tid__.fetch_add(1, std::memory_order_relaxed);
    return ret;
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
    static const thread_local auto ret=tll::util::to_string(tid());
    return ret;
}

inline std::string str_tidcpu()
{
    using std::to_string;
    static std::atomic<int> __tidcpu__{0};
    static const thread_local auto ret = to_string(__tidcpu__.fetch_add(1, std::memory_order_relaxed)) + "/" + to_string(sched_getcpu());
    return ret;
}

inline std::string str_tid_nice()
{
    static const thread_local auto ret = tll::util::to_string(tid_nice());
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

inline const char *argument(const std::string &value) noexcept
{
    return value.data();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
template <typename ... Args>
std::string stringFormat(
    const char *format,
    Args const & ... args)
{
    constexpr size_t kLogSize = 0x1000;
    constexpr size_t kArgSize = sizeof...(Args);

    if(kArgSize)
    {
        std::string buffer;
        buffer.resize(kLogSize);
        int len = snprintf(&buffer[0],
                                buffer.size(),
                                format,
                                argument(args) ...);
        buffer.resize(len);
        return buffer;
    }
    else
    {
        return format;
    }
}
#pragma GCC diagnostic pop


template <uint32_t num_of_counters=1, typename D=std::chrono::duration<double, std::ratio<1>>, typename C=std::chrono::steady_clock>
class Counter
{
public:
    using Clock = C;
    using Duration = D;
    using Type = typename Duration::rep;
    using Ratio = typename Duration::period;
    using Tp = std::chrono::time_point<C,D>;
private:
    std::array<Tp, num_of_counters> begs_ = create_array<Tp, num_of_counters>(std::chrono::time_point_cast<D>(Clock::now()));
public:
    Counter() = default;
    ~Counter() = default;

    void start(uint32_t id=-1, Tp tp=std::chrono::time_point_cast<D>(Clock::now()))
    {
        if(id > begs_.size())
            for (auto &cnter: begs_) cnter = tp;
        else
            begs_[id] = tp;
    }

    void reset(uint32_t id=-1, Tp tp=std::chrono::time_point_cast<D>(Clock::now()))
    {
        if(id > begs_.size())
            for (auto &cnter: begs_) cnter = tp;
        else
            begs_[id] = tp;
    }

    Duration elapse(uint32_t id=0) const
    {
        if(id > begs_.size()) return Duration{-1};
        return std::chrono::time_point_cast<D>(Clock::now()) - begs_[id];
    }

    Duration elapsed(uint32_t id=0)
    {
        auto ret = elapse(id);
        start(id);
        return ret;
    }
}; /// Counter


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
        return *singleton.load(std::memory_order_relaxed);

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

template <> inline std::string toString<bool>(const bool &val) {return val ? "(bool)true" : "(bool)false";}
template <> inline std::string toString<char>(const char &val) {return stringFormat("(char)%c", val);}
template <> inline std::string toString<int8_t>(const int8_t &val) {return stringFormat("(int8)%d", val);}
template <> inline std::string toString<uint8_t>(const uint8_t &val) {return stringFormat("(uint8)0x%x", val);}
template <> inline std::string toString<int16_t>(const int16_t &val) {return stringFormat("(int16)%d", val);}
template <> inline std::string toString<uint16_t>(const uint16_t &val) {return stringFormat("(uint16)0x%x", val);}
template <> inline std::string toString<int32_t>(const int32_t &val) {return stringFormat("(int32)%d", val);}
template <> inline std::string toString<uint32_t>(const uint32_t &val) {return stringFormat("(uint32)0x%x", val);}
template <> inline std::string toString<int64_t>(const int64_t &val) {return stringFormat("(int64)%ld", val);}
template <> inline std::string toString<uint64_t>(const uint64_t &val) {return stringFormat("(uint64)0x%lx", val);}
template <> inline std::string toString<float>(const float &val) {return stringFormat("(float)%f", val);}
template <> inline std::string toString<double>(const double &val) {return stringFormat("(double)%f", val);}
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



// struct DynamicLogInfo {
//     StaticLogInfo *s_log_info;
//     size_t timestamp;
//     int tid;
//     std::vector<uint8_t> args;
// };


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

struct StreamBuffer {
    StreamBuffer(size_t capa=0x400) : capacity(capa) { internal_buff.resize(capa); }
    StreamBuffer(char *ptr, size_t capa) : external_buff(ptr), capacity(capa) {}

    template <typename T>
    TypeId getId();
    template <typename T>
    StreamBuffer &operator<< (const T *val);

    template <typename T>
    StreamBuffer &operator<< (T val)
    {
        char *buffer_ptr = (external_buff != nullptr) ? (external_buff) : (!internal_buff.empty()) ? (internal_buff.data() + size) : nullptr;
        size_t payload_size = kIdSize + sizeof(T);
        if((size + payload_size) <= capacity)
        {
            size += payload_size;
            if(buffer_ptr != nullptr)
            {
                *(TypeId*)buffer_ptr = getId<T>(); /// primitive id
                *(T*)(buffer_ptr + kIdSize) = val;
                buffer_ptr += payload_size;
            }
        }
        return *this;
    }

    static constexpr uint8_t kIdSize = sizeof(TypeId);
    char *external_buff=nullptr;
    size_t size=0;
    size_t capacity=0;
    std::vector<char> internal_buff;
};


template <> inline TypeId StreamBuffer::getId<int8_t>() {return TypeId::kInt8;}
template <> inline TypeId StreamBuffer::getId<uint8_t>() {return TypeId::kUint8;}
template <> inline TypeId StreamBuffer::getId<int16_t>() {return TypeId::kInt16;}
template <> inline TypeId StreamBuffer::getId<uint16_t>() {return TypeId::kUint16;}
template <> inline TypeId StreamBuffer::getId<int32_t>() {return TypeId::kInt32;}
template <> inline TypeId StreamBuffer::getId<uint32_t>() {return TypeId::kUint32;}
template <> inline TypeId StreamBuffer::getId<int64_t>() {return TypeId::kInt64;}
template <> inline TypeId StreamBuffer::getId<uint64_t>() {return TypeId::kUint64;}
template <> inline TypeId StreamBuffer::getId<float>() {return TypeId::kFloat;}
template <> inline TypeId StreamBuffer::getId<double>() {return TypeId::kDouble;}
template <> inline TypeId StreamBuffer::getId<const char*>() {return TypeId::kCharPtr;}

template <>
inline StreamBuffer &StreamBuffer::operator<< <char>(const char *val)
{
    char *buffer_ptr = (external_buff != nullptr) ? (external_buff) : (!internal_buff.empty()) ? (internal_buff.data() + size) : nullptr;

    auto str_len = strlen(val);
    size_t payload_size = kIdSize + 2 + str_len;
    if((size + payload_size) <= capacity)
    {
        size += payload_size;
        if(buffer_ptr != nullptr)
        {
            *(TypeId*)buffer_ptr = getId<const char *>(); /// string id
            buffer_ptr += kIdSize;
            *(uint16_t*)(buffer_ptr+kIdSize) = str_len;
            memcpy(buffer_ptr+kIdSize+2, val, str_len);
            buffer_ptr += payload_size;
        }
    }
    return *this;
}

template <typename ...Args>
std::vector<char> compress(const char *format, Args ... args)
{
    std::vector<char> ret;
    decompress(ret, args...);
    return ret;
}

template <typename ...Args>
std::string decompress(const std::vector<char> &buffer, Args... args)
{
    std::string ret;
    if(!buffer.empty())
    {

    }
    return ret;
}

}} /// tll::util
