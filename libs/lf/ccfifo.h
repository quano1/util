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
template <size_t num_threads=128>
struct CCIndex
{
public:
    CCIndex() = default;

    CCIndex(size_t size)
    {
        reserve(size);
    }

    inline auto dump() const
    {
        return util::stringFormat("sz:%ld ph:%ld pt:%ld wm:%ld ch:%ld ct:%ld", capacity(), 
             ph_.load(std::memory_order_relaxed), pt_.load(std::memory_order_relaxed), 
             wm_.load(std::memory_order_relaxed), 
             ch_.load(std::memory_order_relaxed), ct_.load(std::memory_order_relaxed));
    }

    inline void reset(size_t new_size=0)
    {
        if(new_size)
        {
            reserve(new_size);
        }
        else
        {
            ph_.store(capacity_,std::memory_order_relaxed);
            ch_.store(capacity_,std::memory_order_relaxed);
            pt_.store(capacity_,std::memory_order_relaxed);
            ct_.store(capacity_,std::memory_order_relaxed);
            wm_.store(0,std::memory_order_relaxed);
        }
    }

    inline void reserve(size_t size)
    {
        // num_threads_ = util::isPowerOf2(num_threads) ? num_threads : util::nextPowerOf2(num_threads);
        // pi_.resize(num_threads_);
        // ci_.resize(num_threads_);
        // LOGD("%ld", num_threads_);
        // memset(pi_.data(), kDone, num_threads_ * sizeof(bool));
        // memset(ci_.data(), kDone, num_threads_ * sizeof(bool));

        size = util::isPowerOf2(size) ? size : util::nextPowerOf2(size);
        capacity_= size;
        ph_.store(capacity_,std::memory_order_relaxed);
        ch_.store(capacity_,std::memory_order_relaxed);
        pt_.store(capacity_,std::memory_order_relaxed);
        ct_.store(capacity_,std::memory_order_relaxed);
        wm_.store(0,std::memory_order_relaxed);
        for(int i=0; i<num_threads; i++)
        {
            pi_[i].store(0, std::memory_order_relaxed);
            ci_[i].store(0, std::memory_order_relaxed);
        }
    }

    inline size_t tryPop(size_t &cons, size_t &size)
    {
        cons = ch_.load(std::memory_order_relaxed);
        size_t next_cons;
        size_t prod;
        size_t wmark;
        size_t tmp_size = size;
        for(;;PROF_ADD(stat_pop_miss, 1))
        {
            next_cons = cons;
            size = tmp_size;
            prod = pt_.load(std::memory_order_relaxed);
            wmark = wm_.load(std::memory_order_relaxed);
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

            if(ch_.compare_exchange_weak(cons, next_cons + size, std::memory_order_acquire, std::memory_order_relaxed)) break;
        }

        PROF_ADD(stat_pop_total, 1);
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
        PROF_ADD(stat_pop_size, size);
    }

    inline size_t tryPush(size_t &prod, size_t &size)
    {
        prod = ph_.load(std::memory_order_relaxed);
        size_t next_prod;
        for(;;PROF_ADD(stat_push_miss, 1))
        {
            next_prod = prod;
            size_t wmark = wm_.load(std::memory_order_relaxed);
            size_t cons = ct_.load(std::memory_order_relaxed);
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
                            if(!ph_.compare_exchange_weak(prod, next_prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
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
                        PROF_ADD(stat_push_error, 1);
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
                PROF_ADD(stat_push_error, 1);
                break;
            }
        }
        PROF_ADD(stat_push_total, 1);
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
        PROF_ADD(stat_push_size, size);
    }

    // template <typename ...Args>
    inline size_t pop(const tll::cc::Callback &cb, size_t size)
    {
        size_t cons;
        PROF_TIMER(timer);
        size_t next_cons = tryPop(cons, size);
        PROF_ADD(time_pop_try, timer.elapse().count());
        PROF_TIMER_START(timer);
        if(size)
        {
            cb(next_cons, size);
            PROF_ADD(time_pop_cb, timer.elapse().count());
            PROF_TIMER_START(timer);
            completePop(cons, size);
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
        size_t next_prod = tryPush(prod, size);
        PROF_ADD(time_push_try, timer.elapse().count());
        PROF_TIMER_START(timer);
        if(size)
        {
            cb(next_prod, size);
            PROF_ADD(time_push_cb, timer.elapse().count());
            PROF_TIMER_START(timer);
            completePush(prod, size);
            PROF_ADD(time_push_complete, timer.elapse().count());
        }
        PROF_ADD(time_push_total, timer.absElapse().count());
        return size;
    }

    inline bool completeQueue(size_t idx, std::atomic<size_t> &tail, std::atomic<size_t> index [])
    {
        size_t t_pos = wrap(idx, num_threads);

        if(idx >= (tail.load(std::memory_order_relaxed) + num_threads)) return false;
        
        size_t const kIdx = idx;

        for(;;)
        {
            size_t next = index[wrap(idx, num_threads)].load(std::memory_order_relaxed);
            size_t tmp_tail = tail.load(std::memory_order_relaxed);
            // LOGD(">(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads), tmp_tail, next, this->to_string(index).data());
            if(tmp_tail > idx)
            {
                // LOGD("E-(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads), tmp_tail, next, this->to_string(index).data());
                break;
            }
            else if (tmp_tail == idx)
            {
                size_t cnt = 1;
                next = index[wrap(idx + cnt, num_threads)].load(std::memory_order_relaxed);
                for(; (cnt<num_threads) && (next>tmp_tail); cnt++)
                {
                    next = index[wrap(idx + cnt + 1, num_threads)].load(std::memory_order_relaxed);
                }

                idx += cnt;
                // LOGD("=-(%ld:%ld:%ld:%ld) [%ld] +%ld {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads), tmp_tail, cnt, next, this->to_string(index).data());
                // tail.store(tmp_tail + cnt);
                tail.fetch_add(cnt, std::memory_order_relaxed);
            }

            if(index[wrap(idx, num_threads)].compare_exchange_strong(next, kIdx, std::memory_order_relaxed, std::memory_order_relaxed)) break;

            // LOGD("M-(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads), tmp_tail, next, this->to_string(index).data());
        }

        return true;
    }

    inline bool deQueue(const tll::cc::Callback &cb)
    {
        size_t idx = ch_.load(std::memory_order_relaxed);
        for(;;)
        {
            if (idx == pt_.load(std::memory_order_relaxed))
                return false;

            if(ch_.compare_exchange_weak(idx, idx + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                break;
        }

        cb(idx, 1);
        while(!completeQueue(idx, ct_, ci_)) {std::this_thread::yield();}
        return true;
    }

    inline bool enQueue(const tll::cc::Callback &cb)
    {
        size_t idx = ph_.load(std::memory_order_relaxed);

        for(;;)
        {
            if (idx == (ct_.load(std::memory_order_relaxed) + capacity_))
                return false;

            if(ph_.compare_exchange_weak(idx, idx + 1, std::memory_order_acquire, std::memory_order_relaxed))
                break;
        }

        cb(idx, 1);
        while(!completeQueue(idx, pt_, pi_)) {std::this_thread::yield();}
        return true;
    }

    inline size_t capacity() const
    {
        return capacity_;
    }

    inline size_t size() const
    {
        return pt_.load(std::memory_order_relaxed) - ch_.load(std::memory_order_relaxed);
    }

    inline bool empty() const
    {
        return pt_.load(std::memory_order_relaxed) == ch_.load(std::memory_order_relaxed);
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

    inline size_t ph() const {return ph_.load(std::memory_order_relaxed);}
    inline size_t pt() const {return pt_.load(std::memory_order_relaxed);}
    inline size_t ch() const {return ch_.load(std::memory_order_relaxed);}
    inline size_t ct() const {return ct_.load(std::memory_order_relaxed);}
    inline size_t wm() const {return wm_.load(std::memory_order_relaxed);}
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

    // static constexpr size_t kMaxThreads = 8;
    // size_t num_threads_=num_threads;
    static constexpr int8_t kDone = 0, kWorking = 1;
    // class enum
    // {
    //     kDone = 0,
    //     kWorking
    // };
    std::atomic<size_t> ph_{0}, pt_{0}, wm_{0}, ch_{0}, ct_{0};
    // std::atomic<size_t> pe_{0}, ce_{0};
    std::atomic<size_t> pi_[num_threads], ci_[num_threads];
    // std::bitset<8> pi_, ci_;
    // std::atomic<size_t> prod_exit_flag_{-1llu}, cons_exit_flag_{-1llu};
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

    // std::vector<bool> pi_, ci_;
    // constexpr util::make_array<kMaxThreads, bool> pi_{false}, ci_{false};
    // static constexpr size_t kExitMasks[kMaxThreads] = {
    //     0x0,0x2,0x6,0xe,0x1e,0x3e,0x7e,0xfe,0x1fe,0x3fe,0x7fe,0xffe,0x1ffe,0x3ffe,0x7ffe,0xfffe,0x1fffe,0x3fffe,0x7fffe,0xffffe,0x1ffffe,0x3ffffe,0x7ffffe,0xfffffe,0x1fffffe,0x3fffffe,0x7fffffe,0xffffffe,0x1ffffffe,0x3ffffffe,0x7ffffffe,0xfffffffe,0x1fffffffe,0x3fffffffe,0x7fffffffe,0xffffffffe,0x1ffffffffe,0x3ffffffffe,0x7ffffffffe,0xfffffffffe,0x1fffffffffe,0x3fffffffffe,0x7fffffffffe,0xffffffffffe,0x1ffffffffffe,0x3ffffffffffe,0x7ffffffffffe,0xfffffffffffe,0x1fffffffffffe,0x3fffffffffffe,0x7fffffffffffe,0xffffffffffffe,0x1ffffffffffffe,0x3ffffffffffffe,0x7ffffffffffffe,0xfffffffffffffe,0x1fffffffffffffe,0x3fffffffffffffe,0x7fffffffffffffe,0xffffffffffffffe,0x1ffffffffffffffe,0x3ffffffffffffffe,0x7ffffffffffffffe};

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

    inline auto dump() const
    {
        return cci_.dump();
    }

    inline void reset(size_t new_size=0)
    {
        cci_.reset(new_size);
    }

    inline void reserve(size_t size)
    {
        cci_.reserve(size);
#if (!defined PERF_TUNNEL) || (PERF_TUNNEL==0)
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

private:
    CCIndex<> cci_;
    std::vector<T> buffer_;
};

} /// tll::lf
