#pragma once

#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <sstream>

#define LOGPD(format, ...) printf("[D](%ld)%s(%s:%d)(%s):" format "\n", ::utils::timestamp<std::chrono::high_resolution_clock, std::milli>(std::chrono::high_resolution_clock::now()), __FILE__, __PRETTY_FUNCTION__, __LINE__, ::utils::tid().data(), ##__VA_ARGS__)
#define LOGD(format, ...) printf("[D](%ld)%s(%s:%d)(%s):" format "\n", ::utils::timestamp<std::chrono::high_resolution_clock, std::milli>(std::chrono::high_resolution_clock::now()), __FILE__, __FUNCTION__, __LINE__, ::utils::tid().data(), ##__VA_ARGS__)
#define LOGE(format, ...) printf("[E](%ld)%s(%s:%d)(%s):" format "%s\n", ::utils::timestamp<std::chrono::high_resolution_clock, std::milli>(std::chrono::high_resolution_clock::now()), __FILE__, __FUNCTION__, __LINE__, ::utils::tid().data(), ##__VA_ARGS__, strerror(errno))

#define TIMER(ID) ::utils::Timer __timer_##ID(#ID)
#define TRACE() ::utils::Timer __tracer(std::string(__FUNCTION__) + ":" + std::to_string(__LINE__) + "(" + ::utils::tid() + ")")

template<typename T>
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

    using template_type = typename Trait<T>::template_type;
private:
    crtp(){}
    friend crtpType<T>;
};


namespace utils {

inline std::string tid()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

template <typename C=std::chrono::high_resolution_clock, typename D=std::ratio<1,1>>
size_t timestamp(typename C::time_point const &t)
{
    return std::chrono::duration_cast<std::chrono::duration<size_t,D>>(t.time_since_epoch()).count();
}

struct Timer
{
    using __clock = std::chrono::high_resolution_clock;

    Timer() : id_(""), begin_(__clock::now()) {}
    Timer(std::string id) : id_(std::move(id)), begin_(__clock::now()) {printf(" (%ld)%s\n", ::utils::timestamp<__clock, std::milli>(__clock::now()), id_.data());}
    ~Timer()
    {
        if(!id_.empty())
            printf(" (%ld)~%s: %.3f (ms)\n", ::utils::timestamp<__clock, std::milli>(__clock::now()), id_.data(), elapse<double,std::milli>());
    }

    template <typename T=double, typename D=std::milli>
    T reset()
    {
        T ret = elapse<T,D>();
        begin_ = __clock::now();
        return ret;
    }

    template <typename T=double, typename D=std::milli>
    T elapse() const
    {
        using namespace std::chrono;
        return duration_cast<std::chrono::duration<T,D>>(__clock::now() - begin_).count();
    }

    template <typename T=double, typename D=std::milli>
    std::chrono::duration<T,D> duration() const
    {
        using namespace std::chrono;
        auto ret = duration_cast<std::chrono::duration<T,D>>(__clock::now() - begin_);
        return ret;
    }

    __clock::time_point begin_;
    std::string id_;
};
}