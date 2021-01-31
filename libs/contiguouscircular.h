#pragma once

namespace tll {
struct StatCCI
{
    size_t time_push_total=0, time_pop_total=0;
    size_t time_push_try=0, time_pop_try=0;
    size_t time_push_complete=0, time_pop_complete=0;
    size_t push_size=0, pop_size=0;
    size_t push_total=0, pop_total=0;
    size_t push_error=0, pop_error=0;
    size_t push_miss=0, pop_miss=0;
};
}

#ifdef ENABLE_STAT_TIME
#define STAT_TIME(counter) tll::time::Counter<std::chrono::duration<size_t, std::ratio<1, 1000000000>>, std::chrono::steady_clock> counter;counter.start()
#define STAT_TIME_ELAPSE(counter) counter.elapse().count()
#else
#define STAT_TIME(...)
#define STAT_TIME_ELAPSE(...) 0
#endif

#if !(defined ENABLE_STAT_COUNTER)
#define STAT_FETCH_ADD(...)
#endif


#include "lf/ccbuffer.h"
#include "mt/ccbuffer.h"
