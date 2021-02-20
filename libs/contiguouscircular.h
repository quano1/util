#pragma once

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

typedef std::function<void(size_t index, size_t size)> Callback;

}

typedef std::chrono::steady_clock StatClock;
typedef std::chrono::duration<size_t, std::ratio<1, 1000000000>> StatDuration;

#if (defined ENABLE_PROFILING) && (ENABLE_PROFILING == 1)

#define PROF_TIMER(counter) tll::time::Counter<StatDuration, StatClock> counter
#define PROF_TIMER_START(counter) (counter).start()
#else
#define PROF_TIMER(...)
#define PROF_TIMER_START(...)
#define PROF_ADD(...)
#endif

#include "lf/ccfifo.h"
#include "mt/ccbuffer.h"