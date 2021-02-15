/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include "../util.h"

#ifdef ENABLE_STAT_COUNTER
    #ifdef STAT_FETCH_ADD
        #undef STAT_FETCH_ADD
    #endif
    #define STAT_FETCH_ADD(atomic, val)  atomic.fetch_add(val, std::memory_order_relaxed)
#endif

namespace tll::lf {
/// Contiguous Circular Index
struct CCIndex
{
public:
    CCIndex() = default;

    CCIndex(size_t size)
    {
        reserve(size);
    }

    inline void dump() const
    {
        printf("sz:%ld ph:%ld pt:%ld wm:%ld ch:%ld ct:%ld\n", size_, 
             ph_.load(std::memory_order_relaxed), pt_.load(std::memory_order_relaxed), 
             wm_.load(std::memory_order_relaxed), 
             ch_.load(std::memory_order_relaxed), ct_.load(std::memory_order_relaxed));
    }

    inline void reset(size_t new_size=0)
    {
        ph_.store(0,std::memory_order_relaxed);
        ch_.store(0,std::memory_order_relaxed);
        pt_.store(0,std::memory_order_relaxed);
        ct_.store(0,std::memory_order_relaxed);
        wm_.store(0,std::memory_order_relaxed);
        if(new_size)
            reserve(new_size);
    }

    inline void reserve(size_t size)
    {
        size = util::isPowerOf2(size) ? size : util::nextPowerOf2(size);
        size_ = size;
    }

    inline size_t tryPop(size_t &cons, size_t &size)
    {
        cons = ch_.load(std::memory_order_relaxed);
        size_t next_cons;
        size_t prod;
        size_t wmark;
        size_t tmp_size = size;
        for(;;STAT_FETCH_ADD(stat_pop_miss, 1))
        {
            next_cons = cons;
            size = tmp_size;
            prod = pt_.load(std::memory_order_relaxed);
            wmark = wm_.load(std::memory_order_relaxed);
            if(next_cons == prod)
            {
                // LOGE("Underrun!");dump();
                size = 0;
                STAT_FETCH_ADD(stat_pop_error, 1);
                break;
            }

            if(next_cons == wmark)
            {
                next_cons = next(next_cons);
                if(prod <= next_cons)
                {
                    // LOGE("Underrun!");dump();
                    size = 0;
                    STAT_FETCH_ADD(stat_pop_error, 1);
                    break;
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
                {
                    size = prod - next_cons;
                }
            }

            if(ch_.compare_exchange_weak(cons, next_cons + size, std::memory_order_acquire, std::memory_order_relaxed)) break;
        }

        STAT_FETCH_ADD(stat_pop_total, 1);
        return next_cons;
    }

    inline void completePop(size_t cons, size_t size)
    {
        for(;ct_.load(std::memory_order_relaxed) != cons;){}
        // {std::this_thread::yield();}

        if(cons == wm_.load(std::memory_order_relaxed))
            ct_.store(next(cons) + size, std::memory_order_relaxed);
        else
            ct_.store(cons + size, std::memory_order_relaxed);
        STAT_FETCH_ADD(stat_pop_size, size);
    }

    inline size_t tryPush(size_t &prod, size_t &size)
    {
        prod = ph_.load(std::memory_order_relaxed);
        size_t next_prod;
        for(;;STAT_FETCH_ADD(stat_push_miss, 1))
        {
            next_prod = prod;
            size_t wmark = wm_.load(std::memory_order_relaxed);
            size_t cons = ct_.load(std::memory_order_relaxed);
            if(size <= size_ - (next_prod - cons))
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
                            if(!ph_.compare_exchange_weak(prod, next_prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                            break;
                        }
                        else
                        {
                            // LOGE("OVERRUN");dump();
                            size = 0;
                            STAT_FETCH_ADD(stat_push_error, 1);
                            break;
                        }
                    }
                    else
                    {
                        if(!ph_.compare_exchange_weak(prod, next_prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                        else break;
                    }
                }
                /// cons leads
                else
                {
                    if(size > wrap(cons) - wrap(next_prod))
                    {
                        // LOGE("OVERRUN");dump();
                        size = 0;
                        STAT_FETCH_ADD(stat_push_error, 1);
                        break;
                    }
                    if(!ph_.compare_exchange_weak(prod, next_prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                    else break;
                }
            }
            else
            {
                // LOGE("OVERRUN");dump();
                size = 0;
                STAT_FETCH_ADD(stat_push_error, 1);
                break;
            }
        }
        STAT_FETCH_ADD(stat_push_total, 1);
        return next_prod;
    }

    inline void completePush(size_t prod, size_t size)
    {
        for(;pt_.load(std::memory_order_relaxed) != prod;){}
        // {std::this_thread::yield();}

        if(prod + size > next(prod))
        {
            wm_.store(prod, std::memory_order_relaxed);
            pt_.store(next(prod) + size, std::memory_order_release);
        }
        else
            pt_.store(prod + size, std::memory_order_release);
        STAT_FETCH_ADD(stat_push_size, size);
    }


    // template <typename ...Args>
    inline size_t pop(const tll::cc::Callback &cb, size_t size)
    {
        size_t cons;
        STAT_TIMER(timer);
        size_t next_cons = tryPop(cons, size);
        STAT_FETCH_ADD(time_pop_try, timer.elapse().count());
        STAT_TIMER_START(timer);
        if(size)
        {
            cb(next_cons, size);
            STAT_FETCH_ADD(time_pop_cb, timer.elapse().count());
            STAT_TIMER_START(timer);
            completePop(cons, size);
            STAT_FETCH_ADD(time_pop_complete, timer.elapse().count());
        }
        STAT_FETCH_ADD(time_pop_total, timer.life().count());
        return size;
    }

    // template <typename ...Args>
    inline size_t push(const tll::cc::Callback &cb, size_t size)
    {
        size_t prod;
        STAT_TIMER(timer);
        size_t next_prod = tryPush(prod, size);
        STAT_FETCH_ADD(time_push_try, timer.elapse().count());
        STAT_TIMER_START(timer);
        if(size)
        {
            cb(next_prod, size);
            STAT_FETCH_ADD(time_push_cb, timer.elapse().count());
            STAT_TIMER_START(timer);
            completePush(prod, size);
            STAT_FETCH_ADD(time_push_complete, timer.elapse().count());
        }
        STAT_FETCH_ADD(time_push_total, timer.life().count());
        return size;
    }

    inline size_t capacity() const
    {
        return size_;
    }

    inline size_t size() const
    {
        return pt_.load(std::memory_order_relaxed) - ch_.load(std::memory_order_relaxed);
    }

    inline bool empty() const
    {
        return pt_.load(std::memory_order_relaxed) == ch_.load(std::memory_order_relaxed);
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
            .time_push_total = time_push_total.load(std::memory_order_relaxed), .time_pop_total = time_pop_total.load(std::memory_order_relaxed),
            .time_push_cb = time_push_cb.load(std::memory_order_relaxed), .time_pop_cb = time_pop_cb.load(std::memory_order_relaxed),
            .time_push_try = time_push_try.load(std::memory_order_relaxed), .time_pop_try = time_pop_try.load(std::memory_order_relaxed),
            .time_push_complete = time_push_complete.load(std::memory_order_relaxed), .time_pop_complete = time_pop_complete.load(std::memory_order_relaxed),
            .push_size = stat_push_size.load(std::memory_order_relaxed), .pop_size = stat_pop_size.load(std::memory_order_relaxed),
            .push_total = stat_push_total.load(std::memory_order_relaxed), .pop_total = stat_pop_total.load(std::memory_order_relaxed),
            .push_error = stat_push_error.load(std::memory_order_relaxed), .pop_error = stat_pop_error.load(std::memory_order_relaxed),
            .push_miss = stat_push_miss.load(std::memory_order_relaxed), .pop_miss = stat_pop_miss.load(std::memory_order_relaxed),
        };
    }
// private:
    std::atomic<size_t> ph_{0}, pt_{0}, wm_{0}, ch_{0}, ct_{0};
    size_t size_;
    /// Statistic
    std::atomic<size_t> stat_push_size{0}, stat_pop_size{0};
    std::atomic<size_t> stat_push_total{0}, stat_pop_total{0};
    std::atomic<size_t> stat_push_error{0}, stat_pop_error{0};
    std::atomic<size_t> stat_push_miss{0}, stat_pop_miss{0};

    std::atomic<size_t> time_push_total{0}, time_pop_total{0};
    std::atomic<size_t> time_push_cb{0}, time_pop_cb{0};
    std::atomic<size_t> time_push_try{0}, time_pop_try{0};
    std::atomic<size_t> time_push_complete{0}, time_pop_complete{0};
};


/// Contiguous Circular FIFO
template <typename T>
struct CCFIFO
{
public:
    CCFIFO() = default;

    CCFIFO(size_t size)
    {
        reserve(size);
    }

    inline void dump() const
    {
        cci_.dump();
    }

    inline void reset(size_t new_size=0)
    {
        cci_.reset(new_size);
    }

    inline void reserve(size_t size)
    {
        cci_.reserve(size);
#if !defined PERF_TUN
        buffer_.resize(cci_.capacity());
#endif
    }

    inline auto pop(const tll::cc::Callback &cb, size_t size=1)
    {
        return cci_.pop(cb, size);
    }

    inline auto push(const tll::cc::Callback &cb, size_t size=1)
    {
        return cci_.push(cb, size);
    }

    inline auto capacity() const
    {
        return cci_.capacity();
    }

    inline auto size() const
    {
        return cci_.size();
    }

    inline auto empty() const
    {
        return cci_.empty();
    }

    inline auto wrap(size_t index) const
    {
        return cci_.wrap(index);
    }

    inline auto next(size_t index) const
    {
        return cci_.next(index);
    }

    inline auto stat() const
    {
        return cci_.stat();
    }

    inline T *elemAt(size_t id)
    {
        return buffer_.data() + wrap(id) * sizeof(T);
    }

private:
    CCIndex cci_;
    std::vector<T> buffer_;
};


} /// tll::lf
