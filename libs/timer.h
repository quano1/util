#pragma once

#include <vector>
#include <chrono>
#include <unordered_map>
#include <set>
#include <cassert>
#include <mutex>

namespace tll::time {

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

    auto &restart()
    {
        return stop().start();
    }

    const Tp &begin() const
    {
        return begin_;
    }

    size_t size() const
    {
        return duration_lst_.size();
    }

    Duration elapse() const
    {
        return (std::chrono::time_point_cast<D>(Clock::now()) - begin_);
    }

    Duration period(int idx) const
    {
        assert(idx < duration_lst_.size());
        return duration_lst_[idx];
    }

    Duration last() const
    {
        assert(!duration_lst_.empty());
        return duration_lst_.back();
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
}; /// Counter

template <typename D=std::chrono::duration<double, std::ratio<1>>, typename C=std::chrono::steady_clock>
class List
{
public:
    using Duration = D;
    using Clock = typename Counter<D,C>::Clock;

private:
    const std::chrono::time_point<C,D> begin_ = std::chrono::time_point_cast<D>(Clock::now());
    std::unordered_map<std::string, Counter<D,C>> counters_ = {{"", Counter<D,C>{}}};

public:
    List() = default;
    ~List() = default;
    List(const std::set<std::string> &cnt_lst)
    {
        for (const auto &cnt_id : cnt_lst)
            counters_[cnt_id].start(begin_);
    }

    Counter<D,C> const &operator()(const std::string cnt_id="") const
    {
        return counters_.at(cnt_id);
    }

    Counter<D,C> &operator()(const std::string cnt_id="")
    {
        return counters_[cnt_id];
    }

    Counter<D,C> const &counter(const std::string cnt_id="") const
    {
        return counters_.at(cnt_id);
    }

    Counter<D,C> &counter(const std::string cnt_id="")
    {
        return counters_[cnt_id];
    }

}; /// List

} /// tll::time