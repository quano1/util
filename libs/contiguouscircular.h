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

#ifdef ENABLE_STAT_TIMER

#define STAT_TIMER(counter) tll::time::Counter<StatDuration, StatClock> counter
#define STAT_TIMER_START(counter) (counter).start()
// #define STAT_TIMER_ELAPSE(counter) counter.elapse().count()
#else
#define STAT_TIMER(...)
#define STAT_TIMER_START(...)
#endif

#if !(defined ENABLE_STAT_COUNTER)
#define STAT_FETCH_ADD(...)
#endif


#include "lf/ccfifo.h"
#include "mt/ccbuffer.h"