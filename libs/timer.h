#pragma once

#include <list>
#include <chrono>
#include <unordered_map>
#include <set>
#include <cassert>
#include "utils.h"

namespace tll::time {

template <typename D=std::chrono::duration<double, std::ratio<1>>, typename C=std::chrono::steady_clock>
class Counter
{
public:
    using Clock = C;
    using Duration = D;
    using Type = typename Duration::rep;
    using Ratio = typename Duration::period;
    using Timepoint = std::chrono::time_point<C,D>;
    using Timepoints = std::pair<Timepoint, Timepoint>;
    Timepoint begin=std::chrono::time_point_cast<D>(Clock::now());
    std::vector<Duration> duration_lst;

    Counter() = default;
    ~Counter() = default;

    auto &start(Timepoint tp=std::chrono::time_point_cast<D>(Clock::now()))
    {
        begin = tp;
        return *this;
    }

    auto &stop()
    {
        duration_lst.push_back(elapse());
        return *this;
    }

    auto &restart()
    {
        return stop().start();
    }

    size_t size() const
    {
        return duration_lst.size();
    }

    Duration elapse() const
    {
        return (std::chrono::time_point_cast<D>(Clock::now()) - begin);
    }

    Duration period(int idx) const
    {
        assert(idx < duration_lst.size());
        return duration_lst[idx];
        // return (tps_lst[idx].second - tps_lst[idx].first);
    }

    Duration last() const
    {
        assert(!duration_lst.empty());
        return duration_lst.back();
        // return (tps_lst.back().second - tps_lst.back().first);
    }

    Duration total() const
    {
        Duration ret = Duration{0};
        for (auto &duration : duration_lst)
        {
            ret += duration;
        }

        return ret;
    }
}; /// Counter

template <typename D=std::chrono::duration<double, std::ratio<1>>, typename C=std::chrono::steady_clock>
class List
{
private:
    using Duration = D;
    using Clock = typename Counter<D,C>::Clock;

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