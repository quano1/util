#pragma once

#include <chrono>

#if (defined ENABLE_PROFILING) && (ENABLE_PROFILING == 1)

#define PROF_TIMER(counter) tll::time::Counter<StatDuration, StatClock> counter
#define PROF_TIMER_START(counter) (counter).start()
#else
#define PROF_TIMER(...)
#define PROF_TIMER_START(...)
#define PROF_ADD(...)
#endif

typedef std::chrono::steady_clock StatClock;
typedef std::chrono::duration<size_t, std::ratio<1, 1000000000>> StatDuration;

namespace tll::cc {

struct Stat
{
    size_t time_push_total=0, time_pop_total=0;
    size_t time_push_cb=0, time_pop_cb=0;
    size_t time_push_try=0, time_pop_try=0;
    size_t time_push_complete=0, time_pop_complete=0;
    size_t push_size=0, pop_size=0;
    size_t push_total=0, pop_total=0;
    size_t push_error=0, pop_error=0;
    size_t push_miss=0, pop_miss=0;
};

// typedef std::function<void(size_t index, size_t size)> Callback;

template <typename T>
using Callback = std::function<void(const T *o, size_t s)>;

template <uint8_t type = 3>
void dumpStat(const tll::cc::Stat &st, double real_total_time)
{
    using namespace std::chrono;
    if(type) printf("        count(K) | err(%%)|miss(%%)| try(%%)|comp(%%)| cb(%%) | all(%%)| ops/ms\n");

    if(type & 1)
    {
        double time_push_total = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_push_total)).count();
        double time_push_try = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_push_try)).count();
        double time_push_complete = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_push_complete)).count();

        size_t time_push_real = st.time_push_cb + st.time_push_try + st.time_push_complete;
        double time_push_try_rate = st.time_push_try*100.f/ time_push_real;
        double time_push_complete_rate = st.time_push_complete*100.f/ time_push_real;
        double time_push_callback_rate = st.time_push_cb*100.f/ time_push_real;
        double time_push_real_rate = (time_push_real)*100.f/ st.time_push_total;

        double push_total = st.push_total * 1.f / 1000;
        double push_error_rate = (st.push_error*100.f)/st.push_total;
        double push_miss_rate = (st.push_miss*100.f)/st.push_total;

        // double push_size = st.push_size*1.f / 0x100000;
        // size_t push_success = st.push_total - st.push_error;
        // double push_success_size_one = push_size/push_total;
        // double push_speed = push_success_size_one;

        // double time_push_one = st.time_push_total*1.f / st.push_total;
        // double avg_time_push_one = time_push_one / thread_num;
        // double avg_push_speed = push_size*thread_num/time_push_total;

        double opss = st.push_total * 0.001f / real_total_time;

        printf(" push: %9.3f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.f\n",
               push_total, push_error_rate, push_miss_rate
               , time_push_try_rate, time_push_complete_rate, time_push_callback_rate, time_push_real_rate
               // avg_time_push_one, avg_push_speed
               , opss
               );
    }

    if(type & 2)
    {
        double time_pop_total = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_pop_total)).count();
        double time_pop_try = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_pop_try)).count();
        double time_pop_complete = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_pop_complete)).count();

        double time_pop_real = st.time_pop_cb + st.time_pop_try + st.time_pop_complete;
        double time_pop_try_rate = st.time_pop_try*100.f/ time_pop_real;
        double time_pop_complete_rate = st.time_pop_complete*100.f/ time_pop_real;
        double time_pop_callback_rate = st.time_pop_cb*100.f/ time_pop_real;
        double time_pop_real_rate = (time_pop_real)*100.f/ st.time_pop_total;

        double pop_total = st.pop_total * 1.f / 1000;
        double pop_error_rate = (st.pop_error*100.f)/st.pop_total;
        double pop_miss_rate = (st.pop_miss*100.f)/st.pop_total;

        // double pop_size = st.pop_size*1.f / 0x100000;
        // size_t pop_success = st.pop_total - st.pop_error;
        // double pop_success_size_one = pop_size/pop_total;
        // double pop_speed = pop_success_size_one;

        // double time_pop_one = st.time_pop_total*1.f / st.pop_total;
        // double avg_time_pop_one = time_pop_one / thread_num;
        // double avg_pop_speed = pop_size*thread_num/time_pop_total;
        double opss = st.pop_total * 0.001f / real_total_time;
        printf(" pop : %9.3f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.f\n",
               pop_total, pop_error_rate, pop_miss_rate
               , time_pop_try_rate, time_pop_complete_rate, time_pop_callback_rate, time_pop_real_rate
               // avg_time_pop_one
               , opss
               );
    }
}
}

#include "lf/ccfifo.h"
#include "lf/ccfifo2.h"
// #include "mt/ccbuffer.h"