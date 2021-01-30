/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include "../util.h"

#ifdef ENABLE_STAT
#define STAT_FETCH_ADD(atomic, val)  atomic.fetch_add(val, std::memory_order_relaxed)
#else
#define STAT_FETCH_ADD(...)
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
        cons = ch_.load(std::memory_order_relaxed);
        size_t next_cons;
        size_t prod;
        size_t wmark;
        size_t tmp_size = size;
        for(;;STAT_FETCH_ADD(pop_miss_count, 1))
        {
            next_cons = cons;
            size = tmp_size;
            prod = pt_.load(std::memory_order_relaxed);
            wmark = wm_.load(std::memory_order_relaxed);
            if(next_cons == prod)
            {
                // LOGE("Underrun!");dump();
                size = 0;
                break;
            }

            if(next_cons == wmark)
            {
                next_cons = next(next_cons);
                if(prod <= next_cons)
                {
                    // LOGE("Underrun!");dump();
                    size = 0;
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

        STAT_FETCH_ADD(pop_count, 1);
        return next_cons;
    }

    inline void completePop(size_t cons, size_t size)
    {
        for(;ct_.load(std::memory_order_relaxed) != cons;)
        {std::this_thread::yield();}

        if(cons == wm_.load(std::memory_order_relaxed))
            ct_.store(next(cons) + size, std::memory_order_relaxed);
        else
            ct_.store(cons + size, std::memory_order_relaxed);
        STAT_FETCH_ADD(pop_hit_count, 1);
        STAT_FETCH_ADD(pop_size, size);
    }

    inline size_t tryPush(size_t &prod, size_t &size)
    {
        prod = ph_.load(std::memory_order_relaxed);
        size_t next_prod;
        for(;;STAT_FETCH_ADD(push_miss_count, 1))
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
                break;
            }
        }
        STAT_FETCH_ADD(push_count, 1);
        return next_prod;
    }

    inline void completePush(size_t prod, size_t size)
    {
        for(;pt_.load(std::memory_order_relaxed) != prod;)
        {std::this_thread::yield();}

        if(prod + size > next(prod))
        {
            wm_.store(prod, std::memory_order_relaxed);
            pt_.store(next(prod) + size, std::memory_order_release);
        }
        else
            pt_.store(prod + size, std::memory_order_release);
        STAT_FETCH_ADD(push_hit_count, 1);
        STAT_FETCH_ADD(push_size, size);
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
            .push_size = push_size.load(std::memory_order_relaxed), .pop_size = pop_size.load(std::memory_order_relaxed),
            .push_count = push_count.load(std::memory_order_relaxed), .pop_count = pop_count.load(std::memory_order_relaxed),
            .push_hit_count = push_hit_count.load(std::memory_order_relaxed), .pop_hit_count = pop_hit_count.load(std::memory_order_relaxed),
            .push_miss_count = push_miss_count.load(std::memory_order_relaxed), .pop_miss_count = pop_miss_count.load(std::memory_order_relaxed),
        };
    }
private:
    std::atomic<size_t> ph_{0}, pt_{0}, wm_{0}, ch_{0}, ct_{0};
    size_t size_;
    std::atomic<size_t> push_size{0}, pop_size{0};
    std::atomic<size_t> push_count{0}, pop_count{0};
    std::atomic<size_t> push_hit_count{0}, pop_hit_count{0};
    std::atomic<size_t> push_miss_count{0}, pop_miss_count{0};
};


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
        cci_.dump();
    }

    inline void dumpStat() const
    {
        StatCCI statistic = stat();
        double push_hit_rate = static_cast<double>(statistic.push_hit_count)/statistic.push_count*100;
        double push_miss_rate = static_cast<double>(statistic.push_miss_count)/statistic.push_count*100;
        printf("push: (%9ld|%6.2f%%|%6.2f%%) %ld\n",
               statistic.push_count, push_hit_rate, push_miss_rate,
               statistic.push_size);
        double pop_hit_rate = static_cast<double>(statistic.pop_hit_count)/statistic.pop_count*100;
        double pop_miss_rate = static_cast<double>(statistic.pop_miss_count)/statistic.pop_count*100;
        printf("pop : (%9ld|%6.2f%%|%6.2f%%) %ld\n",
               statistic.pop_count, pop_hit_rate, pop_miss_rate,
               statistic.pop_size);
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

    inline char *tryPop(size_t &cons, size_t &size)
    {
        size_t next_cons = cci_.tryPop(cons, size);
        if(size)
            return buffer_.data() + wrap(next_cons);
        return nullptr;
    }

    inline void completePop(size_t cons, size_t size)
    {
        cci_.completePop(cons, size);
    }

    inline bool pop(char *dst, size_t &size)
    {
        size_t cons;
        char *src = tryPop(cons, size);
        if(src)
        {
            memcpy(dst, src, size);
            completePop(cons, size);
            return true;
        }

        return false;
    }

    inline char *tryPush(size_t &prod, size_t size)
    {
        size_t next_prod = cci_.tryPush(prod, size);
        if(size)
            return buffer_.data() + wrap(next_prod);
        else
            return nullptr;
    }

    inline void completePush(size_t prod, size_t size)
    {
        cci_.completePush(prod, size);
    }

    inline bool push(const char *src, size_t size)
    {
        size_t prod;
        char *dst = tryPush(prod, size);
        if(dst)
        {
            memcpy(dst, src, size);
            completePush(prod, size);
            return true;
        }

        return false;
    }

    inline size_t buffSize() const
    {
        return cci_.buffSize();
    }

    inline size_t dataSize() const
    {
        return cci_.dataSize();
    }

    inline size_t freeSize() const
    {
        return cci_.freeSize();
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
    std::vector<char> buffer_;
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
        StatCCI statistic = stat();
        double push_hit_rate = static_cast<double>(statistic.push_hit_count)/statistic.push_count*100;
        double push_miss_rate = static_cast<double>(statistic.push_miss_count)/statistic.push_count*100;
        printf("push: (%9ld|%6.2f%%|%6.2f%%) %ld\n",
               statistic.push_count, push_hit_rate, push_miss_rate,
               statistic.push_size*sizeof(T));
        double pop_hit_rate = static_cast<double>(statistic.pop_hit_count)/statistic.pop_count*100;
        double pop_miss_rate = static_cast<double>(statistic.pop_miss_count)/statistic.pop_count*100;
        printf("pop : (%9ld|%6.2f%%|%6.2f%%) %ld\n",
               statistic.pop_count, pop_hit_rate, pop_miss_rate,
               statistic.pop_size*sizeof(T));
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

    inline bool pop_(const std::function<void(T *, size_t)> &func, size_t &size)
    {
        size_t cons;
        T *src = tryPop(cons, size);
        if(src)
        {
            func(src, size);
            completePop(cons, size);
            return true;
        }

        return false;
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

    inline bool push_(const std::function<void(T *, size_t)> &func, size_t size)
    {
        size_t prod;
        T *dst = tryPush(prod, size);
        if(dst)
        {
            func(dst, size);
            completePush(prod, size);
            return true;
        }

        return false;
    }

    inline bool pop(T *dst, size_t &size)
    {
        return pop_([dst](T *src, size_t s){
            std::memcpy(dst, src, s);
        }, size);
    }

    inline bool push(const T *src, size_t size)
    {
        return push_([src](T *dst, size_t s){
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
