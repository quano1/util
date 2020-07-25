#pragma once

#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <sstream>
#include <atomic>
#include <cstring>

#include <memory>
#include <functional>
#include <list>
#include <algorithm>
#include <cassert>

namespace tll::util
{

template <typename T=double, typename C=std::chrono::steady_clock>
class Timer
{
public:
    using clock = C;
private:
    const typename clock::time_point begin_ = clock::now();
    std::unordered_map<std::string, T> elapsed_lst_;
    std::unordered_map<std::string, typename clock::time_point> tp_lst_;

public:
    Timer() = default;
    ~Timer() = default;
    Timer(const std::set<std::string> &tp_lst)
    {
        for (const auto &tp_id : tp_lst)
            add(tp_id, clock::now());
    }

    template <typename D=std::ratio<1,1>>
    T save(const std::string &tp_id)
    {
        assert(!tp_id.empty());
        T period = elapse(tp_id);
        elapsed_lst_[tp_id] += period;
        return period;
    }

    void add(const std::string &tp_id, typename clock::time_point tp=clock::now())
    {
        assert(!tp_id.empty());
        tp_lst_[tp_id] = tp;
        elapsed_lst_[tp_id] = 0;
    }

    void remove(const std::string &tp_id)
    {
        assert(!tp_id.empty());
        tp_lst_.erase(tp_id);
        elapsed_lst_.erase(tp_id);
    }

    T get(const std::string &tp_id) const
    {
        assert(!tp_id.empty());
        return elapsed_lst_.at(tp_id);
    }

    typename clock::time_point timePoint(const std::string &tp_id="") const
    {
        if(tp_id.empty()) return begin_;
        else return elapsed_lst_.at(tp_id);
    }

    template <typename D=std::ratio<1,1>>
    T reset(const std::string &tp_id)
    {
        assert(!tp_id.empty());
        T period = elapse<D>(tp_id);
        tp_lst_[tp_id] = clock::now();
        return period;
    }

    template <typename D=std::ratio<1,1>>
    T elapse(const std::string &tp_id="") const
    {
        if(tp_id.empty())
            return std::chrono::duration_cast<std::chrono::duration<T,D>>(clock::now() - begin_).count();
        else
            return std::chrono::duration_cast<std::chrono::duration<T,D>>(clock::now() - tp_lst_.at(tp_id)).count();
    }

    template <typename T2, typename D=std::ratio<1,1>>
    std::chrono::duration<T2,D> duration(const std::string &tp_id="") const
    {
        if(tp_id.empty())
            return std::chrono::duration_cast<std::chrono::duration<T2,D>>(clock::now() - begin_);
        else
            return std::chrono::duration_cast<std::chrono::duration<T2,D>>(clock::now() - tp_lst_.at(tp_id));
    }
}; /// Timer

}