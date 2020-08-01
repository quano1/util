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
    using Type = typename Duration::rep;
    using Ratio = typename Duration::period;
    using Timepoints = std::pair<typename Clock::time_point, typename Clock::time_point>;
    std::list<Timepoints> tps_lst;

    Counter() = default;
    ~Counter() = default;
    Counter(const Timepoints &tps) : tps_lst({tps})
    {}

    void start(typename Clock::time_point tp=Clock::now())
    {
        tps_lst.push_back({tp, tp});
    }

    Duration stop()
    {
        tps_lst.back().second = Clock::now();
        return tps_lst.back().second - tps_lst.back().first;
    }

    Duration elapse() const
    {
        return Duration{Clock::now() - tps_lst.back().first};
    }

    Duration last() const
    {
        return tps_lst.back().second - tps_lst.back().first;
    }

    Duration total() const
    {
        Duration ret;
        for (auto &tps : tps_lst)
        {
            ret += tps.second - tps.first;
        }

        return ret;
    }
}; /// Counter

template <class Cnt=Counter<>>
class List
{
private:
    using Clock = typename Cnt::Clock;
    const typename Clock::time_point begin_ = Clock::now();
    std::unordered_map<std::string, Cnt> counters_ = {{"", Cnt{{begin_,begin_}}}};

public:
    List() = default;
    ~List() = default;
    List(const std::set<std::string> &tp_lst)
    {
        for (const auto &tp_id : tp_lst)
            counters_[tp_id].start(begin_);
    }

    Cnt const &counter(const std::string tp_id="") const
    {
        return counters_.at(tp_id);
    }

    Cnt &counter(const std::string tp_id="")
    {
        return counters_[tp_id];
    }

}; /// List

} /// tll::time