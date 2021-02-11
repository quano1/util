/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <vector>
#include <chrono>
#include <unordered_map>
#include <set>

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
    Tp begin_life_=begin_;

    Duration total_elapsed_{0}, last_elapsed_{0};
    std::vector<Duration> duration_lst_;
public:
    Counter() = default;
    ~Counter() = default;

    auto &start(Tp tp=std::chrono::time_point_cast<D>(Clock::now()))
    {
        begin_ = tp;
        return *this;
    }

    auto &stop()
    {
        duration_lst_.push_back(elapse());
        return *this;
    }

    auto elapsed()
    {
        last_elapsed_ = elapse();
        total_elapsed_ += last_elapsed_;
        return last_elapsed_;
    }

    auto &clear()
    {
        duration_lst_.clear();
        return *this;
    }

    Duration elapse() const
    {
        return std::chrono::time_point_cast<D>(Clock::now()) - begin_;
    }

    const Tp &begin() const
    {
        return begin_;
    }

    size_t size() const
    {
        return duration_lst_.size();
    }

    Duration duration(size_t idx=-1) const
    {
        if (duration_lst_.empty())
        {
            return elapse();
        }
        else if (idx >= duration_lst_.size())
        {
            return duration_lst_.back();
        }

        return duration_lst_[idx];
    }

    Duration total() const
    {
        Duration ret = Duration{0};
        for (auto &duration : duration_lst_)
        {
            ret += duration;
        }

        return ret;
    }

    Duration lastElapsed() const
    {
        return last_elapsed_;
    }

    Duration totalElapsed() const
    {
        return total_elapsed_;
    }

    Duration life() const
    {
        return std::chrono::time_point_cast<D>(Clock::now()) - begin_life_;
    }
}; /// Counter

// template <typename D=std::chrono::duration<double, std::ratio<1>>, typename C=std::chrono::steady_clock>
// template < template <typename D, typename C> class Cnt = Counter >

// template <template <typename, typename> class> class Map;

template <class Cnt=Counter<>>
class Map 
{
public:
    using CNT = Cnt;
    using Duration = typename Cnt::Duration;
    using Clock = typename Cnt::Clock;

private:
    const std::chrono::time_point<Clock, Duration> begin_ = std::chrono::time_point_cast<Duration>(Clock::now());
    std::unordered_map<std::string, Cnt> counters_ = {{"", Cnt{}}};

public:
    Map() = default;
    ~Map() = default;
    template <typename ...Args>
    Map(Args... args) : Map{{std::forward<std::string>(args)...}}
    {}

    Map(const std::set<std::string> &cnt_lst)
    {
        for (const auto &cnt_id : cnt_lst)
            counters_[cnt_id];
            // counters_[cnt_id].start(begin_);
    }

    Cnt const &operator()(const std::string cnt_id="") const
    {
        return counters_.at(cnt_id);
    }

    Cnt &operator()(const std::string cnt_id="")
    {
        return counters_[cnt_id];
    }

    Cnt const &get(const std::string cnt_id="") const
    {
        return counters_.at(cnt_id);
    }

    Cnt &get(const std::string cnt_id="")
    {
        return counters_[cnt_id];
    }

    auto find(const std::string cnt_id="")
    {
        return counters_.find(cnt_id);
    }

    auto begin()
    {
        return counters_.begin();
    }

    auto end()
    {
        return counters_.end();
    }

    Duration allTotal() const
    {
        Duration ret{};
        for (const auto &cnt : counters_)
        {
            ret += cnt.second.total();
        }
        return ret;
    }

}; /// List

}} /// tll::time

#ifdef PROF_LOG
namespace prof {
tll::time::Map<> timer{{"dolog", "open", "header", "node::log::async", "close"}};
}
#endif