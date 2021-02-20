/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include "../util.h"

#if (defined ENABLE_PROFILING) && (ENABLE_PROFILING == 1)
    #ifdef PROF_ADD
        #undef PROF_ADD
    #endif
    #define PROF_ADD(atomic, val)  atomic+=val
#endif

namespace tll::mt {
/// Contiguous Circular Buffer
struct CCBuffer
{
public:
    CCBuffer() = default;

    CCBuffer(size_t size)
    {
        reserve(size);
    }

    inline void dump() const
    {
        LOGD("sz:0x%lx ph:%ld(%ld) pt:%ld(%ld) wm:%ld(%ld) ch:%ld(%ld) ct:%ld(%ld)", size_, 
             wrap(ph_), ph_, wrap(pt_), pt_, 
             wrap(wm_), wm_, 
             wrap(ch_), ch_, wrap(ct_), ct_);
    }

    inline void reset(size_t new_size=0)
    {
        std::scoped_lock lock(push_mtx_, pop_mtx_);
        ph_ = 0;
        ch_ = 0;
        pt_ = 0;
        ct_ = 0;
        wm_ = 0;
        if(new_size)
            reserve(new_size);
    }

    inline void reserve(size_t size)
    {
        std::scoped_lock lock(push_mtx_, pop_mtx_);
        size_ = util::isPowerOf2(size) ? size : util::nextPowerOf2(size);
        // dump();
#if (!defined PERF_TUNNEL) || (PERF_TUNNEL==0)
        buffer_.resize(size_);
#endif
    }

    inline size_t tryPop(size_t &cons, size_t &size)
    {
        std::scoped_lock lock(pop_mtx_);
        cons = ch_;
        size_t next_cons = cons;
        size_t prod = pt_;
        size_t wmark = wm_;
        if(next_cons == prod)
        {
            // LOGE("Underrun!");dump();
            size = 0;
            PROF_ADD(stat_pop_error, 1);
            // return nullptr;
        }
        else if(next_cons == wmark)
        {
            next_cons = next(cons);
            if(prod <= next_cons)
            {
                // LOGE("Underrun!");dump();
                size = 0;
                PROF_ADD(stat_pop_error, 1);
                // return nullptr;
            }
            if(size > (prod - next_cons))
                size = prod - next_cons;
        }
        else if(next_cons < wmark)
        {
            if(size > (wmark - next_cons))
                size = wmark - next_cons;
        }
        else /// cons > wmark
        {
            if(size > (prod - next_cons))
                size = prod - next_cons;
        }

        ch_ = next_cons + size;
        PROF_ADD(stat_pop_total, 1);
        return next_cons;
    }

    inline void completePop(size_t cons, size_t size)
    {
        std::scoped_lock lock(pop_mtx_);
        if(cons == wm_)
            ct_ = next(cons) + size;
        else
            ct_ = cons + size;
        PROF_ADD(stat_pop_size, size);
    }

    inline size_t tryPush(size_t &prod, size_t &size)
    {
        std::scoped_lock lock(push_mtx_);
        prod = ph_;
        size_t next_prod = prod;
        size_t cons = ct_;
        if(size <= size_ - (prod - cons))
        {
            /// prod leads
            if(wrap(next_prod) >= wrap(cons))
            {
                /// not enough space    ?
                if(size > size_ - wrap(next_prod))
                {
                    if(size <= wrap(cons))
                    {
                        next_prod = next(next_prod);
                        ph_ = next_prod + size;
                        // break;
                    }
                    else
                    {
                        // LOGE("OVERRUN");dump();
                        size = 0;
                        PROF_ADD(stat_push_error, 1);
                        // break;
                    }
                }
                else
                {
                    ph_ = next_prod + size;
                    // break;
                }
            }
            /// cons leads
            else
            {
                if(size > wrap(cons) - wrap(prod))
                {
                    // LOGE("OVERRUN");dump();
                    size = 0;
                    PROF_ADD(stat_push_error, 1);
                    // break;
                }
                ph_ = next_prod + size;
            }
        }
        else
        {
            // LOGE("OVERRUN");dump();
            size = 0;
            PROF_ADD(stat_push_error, 1);
            // break;
        }
        PROF_ADD(stat_push_total, 1);
        return next_prod;
    }

    inline void completePush(size_t prod, size_t size)
    {
        std::scoped_lock lock(push_mtx_);
        if(prod + size > next(prod))
        {
            wm_ = prod;
            pt_ = next(prod) + size;
        }
        else
            pt_ = prod + size;
        PROF_ADD(stat_push_size, size);
    }

    inline size_t pop(const tll::cc::Callback &cb, size_t size)
    {
        PROF_TIMER(timer);
        std::scoped_lock lock(pop_mtx_);
        size_t cons;
        size_t id = tryPop(cons, size);
        PROF_ADD(time_pop_try, timer.elapse().count());
        PROF_TIMER_START(timer);
        if(size)
        {
            cb(id, size);
            PROF_ADD(time_pop_cb, timer.elapse().count());
            PROF_TIMER_START(timer);
            completePop(cons, size);
            PROF_ADD(time_pop_complete, timer.elapse().count());
        }
        PROF_ADD(time_pop_total, timer.absElapse().count());
        return size;
    }

    inline size_t push(const tll::cc::Callback &cb, size_t size)
    {
        PROF_TIMER(timer);
        std::scoped_lock lock(push_mtx_);
        size_t cons;
        size_t id = tryPush(cons, size);
        PROF_ADD(time_push_try, timer.elapse().count());
        PROF_TIMER_START(timer);
        if(size)
        {
            cb(id, size);
            PROF_ADD(time_push_cb, timer.elapse().count());
            PROF_TIMER_START(timer);
            completePush(cons, size);
            PROF_ADD(time_push_complete, timer.elapse().count());
        }
        // dump();
        PROF_ADD(time_push_total, timer.absElapse().count());
        return size;
    }

    inline char *elemAt(size_t id)
    {
        return buffer_.data() + wrap(id);
    }

    inline size_t wrap(size_t index) const
    {
        return index & (size_ - 1);
    }

    inline size_t next(size_t index) const
    {
        size_t tmp = size_ - 1;
        return (index + tmp) & (~tmp);
    }

    inline tll::cc::Stat stat() const
    {
        return tll::cc::Stat{
            .time_push_total = time_push_total, .time_pop_total = time_pop_total,
            .time_push_cb = time_push_cb, .time_pop_cb = time_pop_cb,
            .time_push_try = time_push_try, .time_pop_try = time_pop_try,
            .time_push_complete = time_push_complete, .time_pop_complete = time_pop_complete,
            .push_size = stat_push_size, .pop_size = stat_pop_size,
            .push_total = stat_push_total, .pop_total = stat_pop_total,
            .push_error = stat_push_error, .pop_error = stat_pop_error,
            .push_miss = 0, .pop_miss = 0,
        };
    }
private:
    size_t stat_push_size=0, stat_pop_size=0;
    size_t stat_push_total=0, stat_pop_total=0;
    size_t stat_push_error=0, stat_pop_error=0;

    size_t time_push_total=0, time_pop_total=0;
    size_t time_push_cb=0, time_pop_cb=0;
    size_t time_push_try=0, time_pop_try=0;
    size_t time_push_complete=0, time_pop_complete=0;

    size_t ph_=0, pt_=0, ch_=0, ct_=0, wm_=0;
    size_t size_=0;
    std::vector<char> buffer_{}; /// 1Kb
    std::recursive_mutex push_mtx_, pop_mtx_;
};
} /// tll::lk
