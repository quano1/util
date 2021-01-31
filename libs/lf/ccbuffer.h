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
struct CCI
{
public:
    CCI() = default;

    CCI(size_t size)
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
        STAT_TIME(timer);
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

        STAT_FETCH_ADD(time_pop_try, STAT_TIME_ELAPSE(timer));
        STAT_FETCH_ADD(stat_pop_total, 1);
        return next_cons;
    }

    inline void completePop(size_t cons, size_t size)
    {
        STAT_TIME(timer);
        for(;ct_.load(std::memory_order_relaxed) != cons;)
        {std::this_thread::yield();}

        if(cons == wm_.load(std::memory_order_relaxed))
            ct_.store(next(cons) + size, std::memory_order_relaxed);
        else
            ct_.store(cons + size, std::memory_order_relaxed);
        STAT_FETCH_ADD(time_pop_complete, STAT_TIME_ELAPSE(timer));
        STAT_FETCH_ADD(stat_pop_size, size);
    }

    // template <typename ...Args>
    inline bool pop(const std::function<void(size_t, size_t)> &func, size_t &size)
    {
        STAT_TIME(timer);
        size_t cons;
        size_t next_cons = tryPop(cons, size);
        if(size)
        {
            func(next_cons, size);
            completePop(cons, size);
            STAT_FETCH_ADD(time_pop_total, STAT_TIME_ELAPSE(timer));
            return true;
        }
        STAT_FETCH_ADD(time_pop_total, STAT_TIME_ELAPSE(timer));
        return false;
    }

    inline size_t tryPush(size_t &prod, size_t &size)
    {
        STAT_TIME(timer);
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
        STAT_FETCH_ADD(time_push_try, STAT_TIME_ELAPSE(timer));
        STAT_FETCH_ADD(stat_push_total, 1);
        return next_prod;
    }

    inline void completePush(size_t prod, size_t size)
    {
        STAT_TIME(timer);
        for(;pt_.load(std::memory_order_relaxed) != prod;)
        {std::this_thread::yield();}

        if(prod + size > next(prod))
        {
            wm_.store(prod, std::memory_order_relaxed);
            pt_.store(next(prod) + size, std::memory_order_release);
        }
        else
            pt_.store(prod + size, std::memory_order_release);
        STAT_FETCH_ADD(time_push_complete, STAT_TIME_ELAPSE(timer));
        STAT_FETCH_ADD(stat_push_size, size);
    }

    // template <typename ...Args>
    inline bool push(const std::function<void (size_t, size_t)> &func, size_t size)
    {
        STAT_TIME(timer);
        size_t prod;
        size_t next_prod = tryPush(prod, size);
        if(size)
        {
            func(next_prod, size);
            completePush(prod, size);
            STAT_FETCH_ADD(time_push_total, STAT_TIME_ELAPSE(timer));
            return true;
        }
        STAT_FETCH_ADD(time_push_total, STAT_TIME_ELAPSE(timer));
        return false;
    }

    inline size_t buffSize() const
    {
        return size_;
    }

    inline size_t dataSize() const
    {
        return pt_.load(std::memory_order_relaxed) - ch_.load(std::memory_order_relaxed) - unused();
    }

    inline size_t freeSize() const
    {
        return size_ - (ph_.load(std::memory_order_relaxed) - ct_.load(std::memory_order_relaxed) - unused());
    }

    inline size_t unused() const
    {
        size_t cons = ct_.load(std::memory_order_relaxed);
        size_t wmark = wm_.load(std::memory_order_relaxed);
        return cons < wmark ? size_ - wrap(wmark) : 0;
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

    inline StatCCI stat() const
    {
        return StatCCI{
            .time_push_total = time_push_total.load(std::memory_order_relaxed), .time_pop_total = time_pop_total.load(std::memory_order_relaxed),
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
    std::atomic<size_t> time_push_try{0}, time_pop_try{0};
    std::atomic<size_t> time_push_complete{0}, time_pop_complete{0};
};


/// Generic Contiguous Circular Container
template <typename T>
struct GCCC
{
public:
    GCCC() = default;

    GCCC(size_t size)
    {
        reserve(size);
    }

    inline void dump() const
    {
        cci_.dump();
    }

    inline void dumpStat() const
    {
        using namespace std::chrono;
        StatCCI st = stat();
        // double time_push_total = duration_cast<duration<double, std::ratio<1>>>(duration<size_t, std::ratio<1,1000000000>>(st.time_push_total)).count();
        // double time_push_try = duration_cast<duration<double>>(duration<size_t, std::ratio<1,1000000000>>(st.time_push_try)).count();
        // double time_push_complete = duration_cast<duration<double>>(duration<size_t, std::ratio<1,1000000000>>(st.time_push_complete)).count();
        // double time_push_try_rate = st.time_push_try*100.f/ st.time_push_total;
        // double time_push_complete_rate = st.time_push_complete*100.f/ st.time_push_total;
        // double time_push_one = st.time_push_total*1.f / st.push_total;

        double push_total = st.push_total * 1.f / 1000000;
        double push_error_rate = (st.push_error*100.f)/st.push_total;
        double push_miss_rate = (st.push_miss*100.f)/st.push_total;

        // double push_size = st.push_size*sizeof(T)*1.f / 0x100000;
        
        // size_t push_success = st.push_total - st.push_error;
        // double push_success_size_one = st.push_size*sizeof(T)*1.f/push_success;
        // double push_speed = push_success_size_one / time_push_total;
        // LOGD("%ld %.f", push_success, push_success_size_one);

        printf("        count(m) | err(%%) | miss(%%)\n");
        printf(" push: %9.3f | %6.2f | %6.2f\n",
               push_total, push_error_rate, push_miss_rate);

        // double time_pop_total = duration_cast<duration<double, std::ratio<1>>>(duration<size_t, std::ratio<1,1000000000>>(st.time_pop_total)).count();
        // double time_pop_try = duration_cast<duration<double>>(duration<size_t, std::ratio<1,1000000000>>(st.time_pop_try)).count();
        // double time_pop_complete = duration_cast<duration<double>>(duration<size_t, std::ratio<1,1000000000>>(st.time_pop_complete)).count();
        // double time_pop_try_rate = st.time_pop_try*100.f/ st.time_pop_total;
        // double time_pop_complete_rate = st.time_pop_complete*100.f/ st.time_pop_total;
        // double time_pop_one = st.time_pop_total*1.f / st.pop_total;

        double pop_total = st.pop_total * 1.f / 1000000;
        double pop_error_rate = (st.pop_error*100.f)/st.pop_total;
        double pop_miss_rate = (st.pop_miss*100.f)/st.pop_total;

        // double pop_size = st.pop_size*sizeof(T)*1.f / 0x100000;

        // size_t pop_success = st.pop_total - st.pop_error;
        // double pop_success_size_one = st.pop_size*sizeof(T)*1.f/pop_success;
        // double pop_speed = pop_success_size_one / time_pop_total;

        // printf("        time(s)|try(%%)|gud(%%)|one(ns)    count(k)|err(%%)|miss(%%) speed(Mbs)\n");
        printf(" pop : %9.3f | %6.2f | %6.2f\n",
               pop_total, pop_error_rate, pop_miss_rate);
        // printf(" - Total time: %6.3f(s)\n", time_push_total + time_pop_total);
        // printf(" - push/pop speed: %.2f\n", push_speed / pop_speed);
    }

    inline void reset(size_t new_size=0)
    {
        cci_.reset(new_size);
    }

    inline void reserve(size_t size)
    {
        cci_.reserve(size);
        buffer_.resize(cci_.buffSize());
    }

    inline T *tryPop(size_t &cons, size_t &size)
    {
        size_t next_cons = cci_.tryPop(cons, size);
        if(size)
            return buffer_.data() + wrap(next_cons) * sizeof(T);
        return nullptr;
    }

    inline void completePop(size_t cons, size_t size)
    {
        cci_.completePop(cons, size);
    }

    inline T *tryPush(size_t &prod, size_t size)
    {
        size_t next_prod = cci_.tryPush(prod, size);
        if(size)
            return buffer_.data() + wrap(next_prod) * sizeof(T);
        else
            return nullptr;
    }

    inline void completePush(size_t prod, size_t size)
    {
        cci_.completePush(prod, size);
    }

    inline bool pop(T *dst, size_t &size)
    {
        return cci_.pop([this, dst](size_t id, size_t s){
            T *src = buffer_.data() + wrap(id) * sizeof(T);
            std::memcpy(dst, src, s);
        }, size);
    }

    inline bool push(const T *src, size_t size)
    {
        return cci_.push(
            [this, src](size_t id, size_t s){
                T *dst = buffer_.data() + wrap(id) * sizeof(T);
                std::memcpy(dst, src, s);
            }, size);
    }

    inline size_t dataSize() const
    {
        return cci_.dataSize();
    }

    inline size_t freeSize() const
    {
        return cci_.freeSize();
    }

    inline size_t buffSize() const
    {
        return cci_.buffSize();
    }

    inline size_t unused() const
    {
        return cci_.unused();
    }

    inline size_t wrap(size_t index) const
    {
        return cci_.wrap(index);
    }

    inline size_t next(size_t index) const
    {
        return cci_.next(index);
    }

    inline StatCCI stat() const
    {
        return cci_.stat();
    }

    CCI cci_;
    std::vector<T> buffer_;
};


} /// tll::lf
