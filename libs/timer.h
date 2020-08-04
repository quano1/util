#pragma once

#include <list>
#include <chrono>
#include <unordered_map>
#include <set>
#include <cassert>

namespace tll::time {

template <typename D=std::chrono::duration<double, std::ratio<1>>, typename C=std::chrono::steady_clock>
class Counter
{
public:
    using Clock = C;
    using Duration = D;
    // using Type = typename Duration::rep;
    // using Ratio = typename Duration::period;
    using Timepoint = typename Clock::time_point;
    using Timepoints = std::pair<Timepoint, Timepoint>;
    std::list<Timepoints> tps_lst = {{Clock::now(), Timepoint{}}};

    Counter() = default;
    ~Counter() = default;
    Counter(const Timepoints &tps) : tps_lst({tps})
    {}

    void start(Timepoint tp=Clock::now())
    {
        tps_lst.push_back({tp, tp});
    }

    Duration stop()
    {
        tps_lst.back().second = Clock::now();
        return std::chrono::duration_cast<Duration>(tps_lst.back().second - tps_lst.back().first);
    }

    Duration restart()
    {
        Duration ret = stop();
        start();
        return ret;
    }

    Duration elapse() const
    {
        return std::chrono::duration_cast<Duration>(Clock::now() - tps_lst.back().first);
    }

    Duration last() const
    {
        return std::chrono::duration_cast<Duration>(tps_lst.back().second - tps_lst.back().first);
    }

    Duration total() const
    {
        Duration ret;
        for (auto &tps : tps_lst)
        {
            ret += std::chrono::duration_cast<Duration>(tps.second - tps.first);
        }

        return ret;
    }
}; /// Counter

template <typename D=std::chrono::duration<double, std::ratio<1>>, typename C=std::chrono::steady_clock>
class List
{
private:
    using Clock = typename Counter<D,C>::Clock;
    const typename Clock::time_point begin_ = Clock::now();
    std::unordered_map<std::string, Counter<D,C>> counters_ = {{"", Counter<D,C>{{begin_,begin_}}}};

public:
    List() = default;
    ~List() = default;
    List(const std::set<std::string> &tp_lst)
    {
        for (const auto &tp_id : tp_lst)
            counters_[tp_id].start(begin_);
    }

    Counter<D,C> const &operator()(const std::string tp_id="") const
    {
        return counters_.at(tp_id);
    }

    Counter<D,C> &operator()(const std::string tp_id="")
    {
        return counters_[tp_id];
    }

    Counter<D,C> const &counter(const std::string tp_id="") const
    {
        return counters_.at(tp_id);
    }

    Counter<D,C> &counter(const std::string tp_id="")
    {
        return counters_[tp_id];
    }

}; /// List

} /// tll::time