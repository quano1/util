/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include "../util.h"

#ifdef ENABLE_STAT
    #ifdef STAT_FETCH_ADD
        #undef STAT_FETCH_ADD
    #endif
    #define STAT_FETCH_ADD(atomic, val)  atomic+=val
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
        LOGD("sz:%ld ph:%ld(%ld) pt:%ld(%ld) wm:%ld(%ld) ch:%ld(%ld) ct:%ld(%ld)", buffer_.size(), 
             wrap(ph_), ph_, wrap(pt_), pt_, 
             wrap(wm_), wm_, 
             wrap(ch_), ch_, wrap(ct_), ct_);
    }

    inline void dumpStat() const
    {
        using namespace std::chrono;
        StatCCI st = stat();
        double time_push_total = duration_cast<duration<double, std::ratio<1>>>(duration<size_t, std::ratio<1,1000000000>>(st.time_push_total)).count();
        double time_push_try = duration_cast<duration<double>>(duration<size_t, std::ratio<1,1000000000>>(st.time_push_try)).count();
        double time_push_complete = duration_cast<duration<double>>(duration<size_t, std::ratio<1,1000000000>>(st.time_push_complete)).count();
        double time_push_try_rate = st.time_push_try*100.f/ st.time_push_total;
        double time_push_complete_rate = st.time_push_complete*100.f/ st.time_push_total;
        double time_push_one = st.time_push_total*1.f / st.push_total;

        double push_error_rate = (st.push_error*100.f)/st.push_total;
        double push_miss_rate = (st.push_miss*100.f)/st.push_total;

        double push_size = st.push_size*1.f / 0x100000;
        double push_speed = push_size / time_push_total;

        printf("push: (%.6f(s)|%6.2f%%|%6.2f%%|%.3f(ns)) (%ld|%6.2f%%|%6.2f%%) %.2f(Mbs)\n",
               time_push_total, time_push_try_rate, time_push_complete_rate, time_push_one,
               st.push_total, push_error_rate, push_miss_rate,
               push_speed);

        double time_pop_total = duration_cast<duration<double, std::ratio<1>>>(duration<size_t, std::ratio<1,1000000000>>(st.time_pop_total)).count();
        double time_pop_try = duration_cast<duration<double>>(duration<size_t, std::ratio<1,1000000000>>(st.time_pop_try)).count();
        double time_pop_complete = duration_cast<duration<double>>(duration<size_t, std::ratio<1,1000000000>>(st.time_pop_complete)).count();
        double time_pop_try_rate = st.time_pop_try*100.f/ st.time_pop_total;
        double time_pop_complete_rate = st.time_pop_complete*100.f/ st.time_pop_total;
        double time_pop_one = st.time_pop_total*1.f / st.pop_total;

        double pop_error_rate = (st.pop_error*100.f)/st.pop_total;
        double pop_miss_rate = (st.pop_miss*100.f)/st.pop_total;

        double pop_size = st.pop_size*1.f / 0x100000;
        double pop_speed = pop_size / time_pop_total;

        printf("pop: (%.6f(s)|%6.2f%%|%6.2f%%|%.3f(ns)) (%ld|%6.2f%%|%6.2f%%) %.2f(Mbs)\n",
               time_pop_total, time_pop_try_rate, time_pop_complete_rate, time_pop_one,
               st.pop_total, pop_error_rate, pop_miss_rate,
               pop_speed);
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
        size = util::isPowerOf2(size) ? size : util::nextPowerOf2(size);
        buffer_.resize(size);
    }

    inline char *tryPop(size_t &cons, size_t &size)
    {
        STAT_COUNTER(counter);
        std::scoped_lock lock(pop_mtx_);
        cons = ch_;
        size_t next_cons = cons;
        size_t prod = pt_;
        size_t wmark = wm_;
        if(next_cons == prod)
        {
            // LOGE("Underrun!");dump();
            size = 0;
            STAT_FETCH_ADD(stat_pop_error, 1);
            return nullptr;
        }

        if(next_cons == wmark)
        {
            next_cons = next(cons);
            if(prod <= next_cons)
            {
                // LOGE("Underrun!");dump();
                size = 0;
                STAT_FETCH_ADD(stat_pop_error, 1);
                return nullptr;
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
        STAT_FETCH_ADD(time_pop_try, counter.elapse().count());
        STAT_FETCH_ADD(stat_pop_total, 1);
        return buffer_.data() + wrap(next_cons);
    }

    inline void completePop(size_t cons, size_t size)
    {
        STAT_COUNTER(counter);
        std::scoped_lock lock(pop_mtx_);
        if(cons == wm_)
            ct_ = next(cons) + size;
        else
            ct_ = cons + size;
        STAT_FETCH_ADD(time_pop_complete, counter.elapse().count());
        STAT_FETCH_ADD(stat_pop_size, size);
    }

    inline bool pop(char *dst, size_t &size)
    {
        STAT_COUNTER(counter);
        std::scoped_lock lock(pop_mtx_);
        size_t cons;
        char *src = tryPop(cons, size);
        if(src != nullptr)
        {
            memcpy(dst, src, size);
            completePop(cons, size);
            STAT_FETCH_ADD(time_pop_total, counter.elapse().count());
            return true;
        }
        // dump();
        STAT_FETCH_ADD(time_pop_total, counter.elapse().count());
        return false;
    }

    inline char *tryPush(size_t &prod, size_t &size)
    {
        STAT_COUNTER(counter);
        std::scoped_lock lock(push_mtx_);
        prod = ph_;
        size_t next_prod = prod;
        size_t cons = ct_;
        if(size <= buffer_.size() - (prod - cons))
        {
            /// prod leads
            if(wrap(next_prod) >= wrap(cons))
            {
                /// not enough space    ?
                if(size > buffer_.size() - wrap(next_prod))
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
                        STAT_FETCH_ADD(stat_push_error, 1);
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
                    STAT_FETCH_ADD(stat_push_error, 1);
                    // break;
                }
                ph_ = next_prod + size;
            }
        }
        else
        {
            // LOGE("OVERRUN");dump();
            size = 0;
            STAT_FETCH_ADD(stat_push_error, 1);
            // break;
        }
        STAT_FETCH_ADD(time_push_try, counter.elapse().count());
        STAT_FETCH_ADD(stat_push_total, 1);
        return buffer_.data() + wrap(next_prod);
    }

    inline void completePush(size_t prod, size_t size)
    {
        STAT_COUNTER(counter);
        std::scoped_lock lock(push_mtx_);
        if(prod + size > next(prod))
        {
            wm_ = prod;
            pt_ = next(prod) + size;
        }
        else
            pt_ = prod + size;
        STAT_FETCH_ADD(time_push_complete, counter.elapse().count());
        STAT_FETCH_ADD(stat_push_size, size);
    }

    inline bool push(const char *src, size_t size)
    {
        STAT_COUNTER(counter);
        std::scoped_lock lock(push_mtx_);
        size_t prod;
        char *dst = tryPush(prod, size);
        if(dst)
        {
            memcpy(dst, src, size);
            completePush(prod, size);
            STAT_FETCH_ADD(time_push_total, counter.elapse().count());
            return true;
        }
        // dump();]
        STAT_FETCH_ADD(time_push_total, counter.elapse().count());
        return false;
    }


    inline size_t wrap(size_t index) const
    {
        return index & (buffer_.size() - 1);
    }

    inline size_t size() const
    {
        return pt_ - ch_ - unused();
    }

    inline size_t freeSize() const
    {
        return buffer_.size() - (ph_ - ct_ - unused());
    }

    inline size_t unused() const
    {
        return ct_ < wm_ ? buffer_.size() - wrap(wm_) : 0;
    }

    inline size_t next(size_t index) const
    {
        size_t tmp = buffer_.size() - 1;
        return (index + tmp) & (~tmp);
    }

    inline StatCCI stat() const
    {
        return StatCCI{
            .time_push_total = time_push_total, .time_pop_total = time_pop_total,
            .time_push_try = time_push_try, .time_pop_try = time_pop_try,
            .time_push_complete = time_push_complete, .time_pop_complete = time_pop_complete,
            .push_size = stat_push_size, .pop_size = stat_pop_size,
            .push_total = stat_push_total, .pop_total = stat_pop_total,
            .push_error = stat_push_error, .pop_error = stat_pop_error,
            .push_miss = 0, .pop_miss = 0,
        };
    }

    size_t stat_push_size=0, stat_pop_size=0;
    size_t stat_push_total=0, stat_pop_total=0;
    size_t stat_push_error=0, stat_pop_error=0;

    size_t time_push_total=0, time_pop_total=0;
    size_t time_push_try=0, time_pop_try=0;
    size_t time_push_complete=0, time_pop_complete=0;

    size_t ph_=0, pt_=0, ch_=0, ct_=0, wm_=0;
    std::vector<char> buffer_{}; /// 1Kb
    std::recursive_mutex push_mtx_, pop_mtx_;
};
} /// tll::lk
