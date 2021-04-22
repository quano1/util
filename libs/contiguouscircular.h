#pragma once

#include <chrono>

// #if (defined ENABLE_PROFILING) && (ENABLE_PROFILING == 1)

// #define PROF_TIMER(counter) tll::time::Counter<StatDuration, StatClock> counter
// #define PROF_TIMER_START(counter) (counter).start()
// #else
// #define PROF_TIMER(...)
// #define PROF_TIMER_START(...)
// #define PROF_ADD(...)
// #endif

typedef std::chrono::steady_clock StatClock;
typedef std::chrono::duration<size_t, std::ratio<1, 1000000000>> StatDuration; /// ns


// #include "lf/ccfifo.h"
#include "lf/ccfifo2.h"

namespace tll::cc {

// struct Statistic
// {
//     size_t time_push_total=0, time_pop_total=0;
//     size_t time_push_cb=0, time_pop_cb=0;
//     size_t time_push_try=0, time_pop_try=0;
//     size_t time_push_complete=0, time_pop_complete=0;
//     size_t push_size=0, pop_size=0;
//     size_t push_total=0, pop_total=0;
//     size_t push_error=0, pop_error=0;
//     size_t push_miss=0, pop_miss=0;
//     size_t push_exit_miss=0, pop_exit_miss=0;
// };

// typedef std::function<void(size_t index, size_t size)> Callback;

template <typename T>
using Callback = std::function<void(const T *o, size_t s)>;

template <uint8_t type = 3>
void dumpStat(const tll::lf2::Statistics &stats, double real_total_time)
{
    using namespace std::chrono;
    if(type) printf("        count(K) |err(%%) | miss(%%) t:c |try(%%) |comp(%%)| cb(%%) | all(%%)| Mbs   | cb try comp (min:max:avg) (nano sec)\n");

    /// producer
    if(type & 1)
    {
        auto &st = stats.first;
        // double time_total = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_total)).count();
        size_t comp_count = st.try_count - st.error_count;
        size_t time_avg_cb = st.time_cb / comp_count;
        size_t time_avg_try = st.time_try / st.try_count;
        size_t time_avg_comp = st.time_comp / comp_count;

        size_t time_real = st.time_cb + st.time_try + st.time_comp;
        double time_try_rate = st.time_try*100.f/ time_real;
        double time_complete_rate = st.time_comp*100.f/ time_real;
        double time_callback_rate = st.time_cb*100.f/ time_real;
        double time_real_rate = (time_real)*100.f/ st.time_total;

        double try_count = st.try_count * 1.f / 1000;
        double error_rate = (st.error_count*100.f)/st.try_count;
        double try_miss_rate = (st.try_miss_count*100.f)/st.try_count;
        double comp_miss_rate = (st.comp_miss_count*100.f)/st.try_count;

        // double opss = st.total_size * .001f / real_total_time;
        double speed = st.total_size * 1.f / 0x100000 / real_total_time;


        printf(" push: %9.3f | %5.2f | %5.2f:%5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.3f | (%ld:%ld:%ld) (%ld:%ld:%ld) (%ld:%ld:%ld)\n",
               try_count, error_rate, try_miss_rate, comp_miss_rate
               , time_try_rate, time_complete_rate, time_callback_rate, time_real_rate
               , speed,
               st.time_min_cb, st.time_max_cb, time_avg_cb,
               st.time_min_try, st.time_max_try, time_avg_try,
               st.time_min_comp, st.time_max_comp, time_avg_comp
               );
    }

    if(type & 2)
    {
        auto &st = stats.second;
        // double time_total = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_total)).count();
        size_t comp_count = st.try_count - st.error_count;
        size_t time_avg_cb = st.time_cb / comp_count;
        size_t time_avg_try = st.time_try / st.try_count;
        size_t time_avg_comp = st.time_comp / comp_count;

        size_t time_real = st.time_cb + st.time_try + st.time_comp;
        double time_try_rate = st.time_try*100.f/ time_real;
        double time_complete_rate = st.time_comp*100.f/ time_real;
        double time_callback_rate = st.time_cb*100.f/ time_real;
        double time_real_rate = (time_real)*100.f/ st.time_total;

        double try_count = st.try_count * 1.f / 1000;
        double error_rate = (st.error_count*100.f)/st.try_count;
        double try_miss_rate = (st.try_miss_count*100.f)/st.try_count;
        double comp_miss_rate = (st.comp_miss_count*100.f)/st.try_count;

        // double opss = st.total_size * .001f / real_total_time;
        double speed = st.total_size * 1.f / 0x100000 / real_total_time;

        printf(" pop : %9.3f | %5.2f | %5.2f:%5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.3f | (%ld:%ld:%ld) (%ld:%ld:%ld) (%ld:%ld:%ld)\n",
               try_count, error_rate, try_miss_rate, comp_miss_rate
               , time_try_rate, time_complete_rate, time_callback_rate, time_real_rate
               , speed,
               st.time_min_cb, st.time_max_cb, time_avg_cb,
               st.time_min_try, st.time_max_try, time_avg_try,
               st.time_min_comp, st.time_max_comp, time_avg_comp
               );
    }
}
}

// #include "mt/ccbuffer.h"