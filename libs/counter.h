/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <chrono>

namespace tll{ namespace time{

// template <typename, typename>
// class Counter;

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

}} /// tll::time

