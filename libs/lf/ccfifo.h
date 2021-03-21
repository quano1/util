/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include <mutex>
#include <bitset>
#include "../counter.h"
#include "../util.h"

#if (defined ENABLE_PROFILING) && (ENABLE_PROFILING == 1)
    #ifdef PROF_ADD
        #undef PROF_ADD
    #endif
    #define PROF_ADD(atomic, val)  atomic.fetch_add(val, std::memory_order_relaxed)
#endif

namespace tll::lf {

/// Contiguous Circular Index
template <size_t num_threads=0x400>
struct CCIndex
{
public:
    CCIndex() = default;

    CCIndex(size_t size)
    {
        static_assert(util::isPowerOf2(num_threads));
        reserve(size);
    }

    ~CCIndex()
    {
        delete prod_out_;
        delete cons_out_;
    }

    inline auto dump() const
    {
        return util::stringFormat("sz:%ld ph:%ld pt:%ld wm:%ld ch:%ld ct:%ld", capacity(), 
             prod_head_.load(std::memory_order_relaxed), prod_tail_.load(std::memory_order_relaxed), 
             water_mark_.load(std::memory_order_relaxed), 
             cons_head_.load(std::memory_order_relaxed), cons_tail_.load(std::memory_order_relaxed));
    }

    inline void reset()
    {
        prod_head_.store(0,std::memory_order_relaxed);
        cons_head_.store(0,std::memory_order_relaxed);
        prod_tail_.store(0,std::memory_order_relaxed);
        cons_tail_.store(0,std::memory_order_relaxed);
        water_mark_.store(0,std::memory_order_relaxed);
        prod_out_ = new std::atomic<size_t> [num_threads];
        cons_out_ = new std::atomic<size_t> [num_threads];
        for(int i=0; i<num_threads; i++)
        {
            prod_out_[i].store(0, std::memory_order_relaxed);
            cons_out_[i].store(0, std::memory_order_relaxed);
        }
    }

    inline void reserve(size_t size)
    {
        size = util::isPowerOf2(size) ? size : util::nextPowerOf2(size);
        capacity_= size;
        reset();
    }

    inline size_t tryPop(size_t &cons, size_t &size)
    {
        cons = cons_head_.load(std::memory_order_relaxed);
        size_t next_cons;
        size_t prod;
        size_t wmark;
        size_t tmp_size = size;
        for(;;PROF_ADD(stat_pop_miss, 1))
        {
            next_cons = cons;
            size = tmp_size;
            prod = prod_tail_.load(std::memory_order_relaxed);
            wmark = water_mark_.load(std::memory_order_relaxed);
            if(next_cons == prod)
            {
                // LOGE("Underrun!");dump();
                size = 0;
                PROF_ADD(stat_pop_error, 1);
                break;
            }

            if(next_cons == wmark)
            {
                next_cons = next(next_cons);
                if(prod <= next_cons)
                {
                    // LOGE("Underrun!");dump();
                    size = 0;
                    PROF_ADD(stat_pop_error, 1);
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

            if(cons_head_.compare_exchange_weak(cons, next_cons + size, std::memory_order_acquire, std::memory_order_relaxed)) break;
        }

        PROF_ADD(stat_pop_total, 1);
        return next_cons;
    }

    inline void completePop(size_t cons, size_t next, size_t size)
    {
        for(;cons_tail_.load(std::memory_order_relaxed) != cons;){}
        // {std::this_thread::yield();}
        cons_tail_.store(next + size, std::memory_order_relaxed);
        PROF_ADD(stat_pop_size, size);
    }

    inline size_t tryPush(size_t &prod, size_t &size)
    {
        prod = prod_head_.load(std::memory_order_relaxed);
        size_t next_prod;
        for(;;PROF_ADD(stat_push_miss, 1))
        {
            next_prod = prod;
            size_t wmark = water_mark_.load(std::memory_order_relaxed);
            size_t cons = cons_tail_.load(std::memory_order_relaxed);
            if(size <= capacity() - (next_prod - cons))
            {
                /// prod leads
                if(wrap(next_prod) >= wrap(cons))
                {
                    /// not enough space    ?
                    if(size > capacity() - wrap(next_prod))
                    {
                        if(size <= wrap(cons))
                        {
                            next_prod = next(next_prod);
                            if(!prod_head_.compare_exchange_weak(prod, next_prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                            break;
                        }
                        else
                        {
                            // LOGE("OVERRUN");dump();
                            size = 0;
                            PROF_ADD(stat_push_error, 1);
                            break;
                        }
                    }
                    else
                    {
                        if(!prod_head_.compare_exchange_weak(prod, next_prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
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
                        PROF_ADD(stat_push_error, 1);
                        break;
                    }
                    if(!prod_head_.compare_exchange_weak(prod, next_prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                    else break;
                }
            }
            else
            {
                // LOGE("OVERRUN");dump();
                size = 0;
                PROF_ADD(stat_push_error, 1);
                break;
            }
        }
        PROF_ADD(stat_push_total, 1);
        return next_prod;
    }

    inline void completePush(size_t prod, size_t next, size_t size)
    {
        for(;prod_tail_.load(std::memory_order_relaxed) != prod;){}
        // {std::this_thread::yield();}

        if(prod < next)
        {
            water_mark_.store(prod, std::memory_order_relaxed);
        }
        prod_tail_.store(next + size, std::memory_order_release);
        PROF_ADD(stat_push_size, size);
    }

    // template <typename ...Args>
    inline size_t pop(const tll::cc::Callback &cb, size_t size)
    {
        size_t cons;
        PROF_TIMER(timer);
        size_t next = tryPop(cons, size);
        PROF_ADD(time_pop_try, timer.elapse().count());
        PROF_TIMER_START(timer);
        if(size)
        {
            cb(next, size);
            PROF_ADD(time_pop_cb, timer.elapse().count());
            PROF_TIMER_START(timer);
            completePop(cons, next, size);
            PROF_ADD(time_pop_complete, timer.elapse().count());
        }
        PROF_ADD(time_pop_total, timer.absElapse().count());
        return size;
    }

    // template <typename ...Args>
    inline size_t push(const tll::cc::Callback &cb, size_t size)
    {
        size_t prod;
        PROF_TIMER(timer);
        size_t next = tryPush(prod, size);
        PROF_ADD(time_push_try, timer.elapse().count());
        PROF_TIMER_START(timer);
        if(size)
        {
            cb(next, size);
            PROF_ADD(time_push_cb, timer.elapse().count());
            PROF_TIMER_START(timer);
            completePush(prod, next, size);
            PROF_ADD(time_push_complete, timer.elapse().count());
        }
        PROF_ADD(time_push_total, timer.absElapse().count());
        return size;
    }

    inline bool completeQueue(size_t idx, std::atomic<size_t> &tail, std::atomic<size_t> out_index [])
    {
        size_t t_pos = wrap(idx, num_threads);

        if(idx >= (tail.load(std::memory_order_relaxed) + num_threads)) return false;
        
        size_t const kIdx = idx;

        for(;;)
        {
            size_t next_idx = out_index[wrap(idx, num_threads)].load(std::memory_order_relaxed);
            size_t crr_tail = tail.load(std::memory_order_relaxed);
            // LOGD(">(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads), crr_tail, next_idx, this->to_string(out_index).data());
            if(crr_tail > idx)
            {
                // LOGD("E-(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads), crr_tail, next_idx, this->to_string(out_index).data());
                break;
            }
            else if (crr_tail == idx)
            {
                size_t cnt = 1;
                next_idx = out_index[wrap(idx + cnt, num_threads)].load(std::memory_order_relaxed);
                for(; (cnt<num_threads) && (next_idx>crr_tail); cnt++)
                {
                    next_idx = out_index[wrap(idx + cnt + 1, num_threads)].load(std::memory_order_relaxed);
                }

                idx += cnt;
                // LOGD("=-(%ld:%ld:%ld:%ld) [%ld] +%ld {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads), crr_tail, cnt, next_idx, this->to_string(out_index).data());
                // tail.store(crr_tail + cnt);
                tail.fetch_add(cnt, std::memory_order_relaxed);
            }

            if(out_index[wrap(idx, num_threads)].compare_exchange_weak(next_idx, kIdx, std::memory_order_relaxed, std::memory_order_relaxed)) break;

            // LOGD("M-(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads), crr_tail, next_idx, this->to_string(out_index).data());
        }

        return true;
    }

    inline bool deQueue(const tll::cc::Callback &cb)
    {
        PROF_TIMER(timer);
        bool ret = false;
        size_t idx = cons_head_.load(std::memory_order_relaxed);
        for(;;)
        {
            if (idx == prod_tail_.load(std::memory_order_relaxed))
            {
                PROF_ADD(stat_pop_error, 1);
                break;
            }

            if(cons_head_.compare_exchange_weak(idx, idx + 1, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                ret = true;
                break;
            }
            PROF_ADD(stat_pop_miss, 1);
        }
        PROF_ADD(time_pop_try, timer.elapse().count());
        PROF_TIMER_START(timer);
        if(ret)
        {
            cb(idx, 1);
            PROF_ADD(time_pop_cb, timer.elapse().count());
            PROF_TIMER_START(timer);
            while(!completeQueue(idx, cons_tail_, cons_out_)) {std::this_thread::yield();}
            PROF_ADD(time_pop_complete, timer.elapse().count());
            PROF_ADD(stat_pop_size, 1);
        }
        PROF_ADD(time_pop_total, timer.absElapse().count());
        PROF_ADD(stat_pop_total, 1);
        return ret;
    }

    inline bool enQueue(const tll::cc::Callback &cb)
    {
        PROF_TIMER(timer);
        bool ret = false;
        size_t idx = prod_head_.load(std::memory_order_relaxed);
        for(;;)
        {
            if (idx == (cons_tail_.load(std::memory_order_relaxed) + capacity_))
            {
                PROF_ADD(stat_push_error, 1);
                break;
            }

            if(prod_head_.compare_exchange_weak(idx, idx + 1, std::memory_order_acquire, std::memory_order_relaxed))
            {
                ret = true;
                break;
            }
            PROF_ADD(stat_push_miss, 1);
        }
        PROF_ADD(time_push_try, timer.elapse().count());
        PROF_TIMER_START(timer);
        if(ret)
        {
            cb(idx, 1);
            PROF_ADD(time_push_cb, timer.elapse().count());
            PROF_TIMER_START(timer);
            while(!completeQueue(idx, prod_tail_, prod_out_)) {std::this_thread::yield();}
            PROF_ADD(time_push_complete, timer.elapse().count());
            PROF_ADD(stat_push_size, 1);
        }
        PROF_ADD(time_push_total, timer.absElapse().count());
        PROF_ADD(stat_push_total, 1);
        return ret;
    }

    inline size_t capacity() const
    {
        return capacity_;
    }

    inline size_t size() const
    {
        return prod_tail_.load(std::memory_order_relaxed) - cons_head_.load(std::memory_order_relaxed);
    }

    inline bool empty() const
    {
        return prod_tail_.load(std::memory_order_relaxed) == cons_head_.load(std::memory_order_relaxed);
    }

    inline size_t wrap(size_t index, size_t mask) const
    {
        assert(util::isPowerOf2(mask));
        return index & (mask - 1);
    }

    inline size_t wrap(size_t index) const
    {
        return wrap(index, capacity());
    }

    inline size_t next(size_t index) const
    {
        size_t tmp = capacity() - 1;
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

    inline size_t ph() const {return prod_head_.load(std::memory_order_relaxed);}
    inline size_t pt() const {return prod_tail_.load(std::memory_order_relaxed);}
    inline size_t ch() const {return cons_head_.load(std::memory_order_relaxed);}
    inline size_t ct() const {return cons_tail_.load(std::memory_order_relaxed);}
    inline size_t wm() const {return water_mark_.load(std::memory_order_relaxed);}
    // inline size_t pef() const {return prod_exit_flag_.load(std::memory_order_relaxed);}
    // inline size_t cef() const {return cons_exit_flag_.load(std::memory_order_relaxed);}
private:

    inline std::string to_string(std::atomic<size_t> idxs [])
    {
        std::string ret;
        ret.resize(num_threads);
        size_t ct = this->ct();
        for(int i=0; i<num_threads; i++)
        {
            if(idxs[i].load(std::memory_order_relaxed) > ct)
                ret[i] = '1';
            else
                ret[i] = '0';
        }
        return ret;
    }

    std::atomic<size_t> prod_head_{0}, prod_tail_{0}, water_mark_{0}, cons_head_{0}, cons_tail_{0};
    // std::atomic<size_t> *prod_in_[num_threads], *cons_in_[num_threads];
    std::atomic<size_t> *prod_out_, *cons_out_;
    size_t capacity_;
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
template <typename T, size_t num_threads=0x400>
struct CCFIFO
{
public:
    CCFIFO() = default;

    CCFIFO(size_t size)
    {
        reserve(size);
    }

    inline auto dump() const
    {
        return cci_.dump();
    }

    inline void reset()
    {
        cci_.reset();
    }

    inline void reserve(size_t size)
    {
        cci_.reserve(size);
#if (!defined NO_ALLOCATE)
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

    inline auto enQueue(const tll::cc::Callback &cb, size_t size=1)
    {
        return cci_.enQueue(cb);
    }

    inline auto deQueue(const tll::cc::Callback &cb, size_t size=1)
    {
        return cci_.deQueue(cb);
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
        return &buffer_[wrap(id)];
    }

// private:
    CCIndex<num_threads> cci_;
    std::vector<T> buffer_;
};

} /// tll::lf
