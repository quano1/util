#pragma once

#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <sstream>
#include <atomic>
#include <cstring>

#include "SimpleSignal.hpp"

#define LOGPD(format, ...) printf("[D](%.6f)%s:%s:%d[%s]:" format "\n", utils::timestamp<double>(), __FILE__, __PRETTY_FUNCTION__, __LINE__, utils::tid().data(), ##__VA_ARGS__)
#define LOGD(format, ...) printf("[D](%.6f)%s:%s:%d[%s]:" format "\n", utils::timestamp<double>(), __FILE__, __FUNCTION__, __LINE__, utils::tid().data(), ##__VA_ARGS__)
#define LOGE(format, ...) printf("[E](%.6f)%s:%s:%d[%s]:" format "%s\n", utils::timestamp<double>(), __FILE__, __FUNCTION__, __LINE__, utils::tid().data(), ##__VA_ARGS__, strerror(errno))

#define TIMER(ID) utils::Timer __timer_##ID(#ID)
#define TRACE() utils::Timer __tracer(std::string(__FUNCTION__) + ":" + std::to_string(__LINE__) + "(" + utils::tid() + ")")

template<typename T, class...>
struct Trait
{
    using template_type = T;
};

template <template <class, class...> class G, class V, class ...Ts>
struct Trait<G<V,Ts...>>
{
    using template_type = V;
};

template <typename T, template<typename ...> class crtpType>
struct crtp
{
    T& underlying() { return static_cast<T&>(*this); }
    T const& underlying() const { return static_cast<T const&>(*this); }

    // using template_type = typename Trait<T>::template_type;
private:
    crtp(){}
    friend crtpType<T>;
};


namespace utils {

/// format
template <typename T>
T Argument(T value) noexcept
{
    return value;
}

template <typename T>
T const * Argument(std::basic_string<T> const & value) noexcept
{
    return value.data();
}

template <typename ... Args>
int StringPrint(char * const buffer,
                size_t const bufferCount,
                char const * const format,
                Args const & ... args) noexcept
{
    int const result = snprintf(buffer,
                              bufferCount,
                              format,
                              Argument(args) ...);
    // assert(-1 != result);
    return result;
}

template <typename ... Args>
int StringPrint(wchar_t * const buffer,
                size_t const bufferCount,
                wchar_t const * const format,
                Args const & ... args) noexcept
{
    int const result = swprintf(buffer,
                              bufferCount,
                              format,
                              Argument(args) ...);
    // assert(-1 != result);
    return result;
}

template <typename T, typename ... Args>
std::basic_string<T> Format(
            size_t size,
            T const * const format,
            Args const & ... args)
{
    std::basic_string<T> buffer;
    buffer.resize(size);
    int len = StringPrint(&buffer[0], buffer.size(), format, args ...);
    buffer.resize(len);
    return buffer;
}

template <typename T, typename ... Args>
std::basic_string<T> Format(
            T const * const format,
            Args const & ... args)
{
    std::basic_string<T> buffer;
    // size_t const size = 0x100;
    size_t const size = StringPrint(&buffer[0], 0, format, args ...);
    if (size > 0)
    {
        buffer.resize(size + 1); /// extra for null
        StringPrint(&buffer[0], buffer.size(), format, args ...);
    }

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

template <typename T, size_t const kELemSize=sizeof(T)>
class BSDLFQ
{
public:
    BSDLFQ(uint32_t num_of_elem) : prod_tail_(0), prod_head_(0), cons_tail_(0), cons_head_(0)
    {
        capacity_ = powerOf2(num_of_elem) ? num_of_elem : nextPowerOf2(num_of_elem);
        buffer_.resize(capacity_ * kELemSize);
    }

    template <typename F, typename ...Args>
    void pop(F &&doPop, Args &&...elems)
    {
        uint32_t cons_head = cons_head_.load(std::memory_order_relaxed);
        for(;;)
        {
            if (cons_head == prod_tail_.load(std::memory_order_relaxed))
                continue;

            if(cons_head_.compare_exchange_weak(cons_head, cons_head + 1, std::memory_order_acquire, std::memory_order_relaxed))
                break;
        }
        std::forward<F>(doPop)(elemAt(cons_head), kELemSize, std::forward<Args>(elems)...);
        while (cons_tail_.load(std::memory_order_relaxed) != cons_head);

        cons_tail_.fetch_add(1, std::memory_order_release);
    }

    template <typename F, typename ...Args>
    void push(F &&doPush, Args&&...elems)
    {
        uint32_t prod_head = prod_head_.load(std::memory_order_relaxed);
        for(;;)
        {
            if (prod_head == (cons_tail_.load(std::memory_order_relaxed) + capacity_))
                continue;

            if(prod_head_.compare_exchange_weak(prod_head, prod_head + 1, std::memory_order_acquire, std::memory_order_relaxed))
                break;
        }
        std::forward<F>(doPush)(elemAt(prod_head), kELemSize, std::forward<Args>(elems)...);
        while (prod_tail_.load(std::memory_order_relaxed) != prod_head);

        prod_tail_.fetch_add(1, std::memory_order_release);
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
        while (cons_tail_.load(std::memory_order_relaxed) != cons_head);

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
        while (prod_tail_.load(std::memory_order_relaxed) != prod_head);

        prod_tail_.fetch_add(1, std::memory_order_release);
        return true;
    }

    inline bool empty() const { return size() == 0; }

    inline uint32_t size() const
    {
        return prod_tail_.load(std::memory_order_relaxed) - cons_tail_.load(std::memory_order_relaxed);
    }

    inline uint32_t wrap(uint32_t index) const
    {
        return index & (capacity_ - 1);
    }

    inline uint32_t capacity() const { return capacity_; }

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


inline std::string tid()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

template <typename T=size_t, typename D=std::ratio<1,1>, typename C=std::chrono::high_resolution_clock>
T timestamp(typename C::time_point &&t = C::now())
{
    return std::chrono::duration_cast<std::chrono::duration<T,D>>(std::forward<typename C::time_point>(t).time_since_epoch()).count();
}

struct Timer
{
    using clock__= std::chrono::high_resolution_clock;

    Timer() : name_(""), begin_(clock__::now()) {}
    Timer(std::string id) : name_(std::move(id)), begin_(clock__::now()) 
    {
        printf(" (%.6f)%s\n", utils::timestamp<double>(), name_.data());
    }

    Timer(std::function<void(std::string const&)> logf, std::string id="") : name_(std::move(id)), begin_(clock__::now()) 
    {
        sig_log_.connect(logf);
        sig_log_.emit(Format("(%s)\n", utils::timestamp<double>(), name_.data()));
    }

    ~Timer()
    {
        if(sig_log_)
            sig_log_.emit(Format("   (%.6f)[%s](~%s) %.3f (ms)\n", utils::timestamp<double>(), utils::tid(), name_.data(), elapse<double,std::milli>()));
        else if(!name_.empty())
            printf(" (%.6f)~%s: %.3f (ms)\n", utils::timestamp<double>(), name_.data(), elapse<double,std::milli>());
    }

    template <typename T=double, typename D=std::milli>
    T reset()
    {
        T ret = elapse<T,D>();
        begin_ = clock__::now();
        return ret;
    }

    template <typename T=double, typename D=std::milli>
    T elapse() const
    {
        using namespace std::chrono;
        return duration_cast<std::chrono::duration<T,D>>(clock__::now() - begin_).count();
    }

    template <typename T=double, typename D=std::milli>
    std::chrono::duration<T,D> duration() const
    {
        using namespace std::chrono;
        auto ret = duration_cast<std::chrono::duration<T,D>>(clock__::now() - begin_);
        return ret;
    }

    clock__::time_point begin_;
    std::string name_;

    Simple::Signal<void(std::string const&)> sig_log_;
};

} /// utils