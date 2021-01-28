/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include "../util.h"

namespace tll::lf {
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
        LOGD("sz:%ld ph:%ld pt:%ld wm:%ld ch:%ld ct:%ld", buffer_.size(), 
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
        buffer_.resize(size);
    }

    inline char *tryPop(size_t &cons, size_t &size)
    {
        cons = ch_.load(std::memory_order_relaxed);
        size_t next_cons = cons;
        for(;;)
        {
            size_t prod = pt_.load(std::memory_order_relaxed);
            size_t wmark = wm_.load(std::memory_order_acquire);
            next_cons = cons;
            if(cons == prod)
            {
                // LOGE("Underrun!");dump();
                size = 0;
                return nullptr;
            }

            if(cons == wmark)
            {
                next_cons = next(cons);
                if(prod <= next_cons)
                {
                    // LOGE("Underrun!");dump();
                    size = 0;
                    return nullptr;
                }
                if(size > (prod - next_cons))
                    size = prod - next_cons;
            }
            else if(cons < wmark)
            {
                if(size > (wmark - cons))
                    size = wmark - cons;
            }
            else /// cons > wmark
            {
                if(size > (prod - cons))
                    size = prod - cons;
            }

            if(ch_.compare_exchange_weak(cons, next_cons + size, std::memory_order_relaxed, std::memory_order_relaxed)) break;
        }
        return buffer_.data() + wrap(next_cons);
    }

    inline void completePop(size_t cons, size_t size)
    {
        for(;ct_.load(std::memory_order_relaxed) != cons;)
        {}

        if(cons == wm_.load(std::memory_order_relaxed))
            ct_.store(next(cons) + size, std::memory_order_release);
        else
            ct_.store(cons + size, std::memory_order_release);
    }

    inline bool pop(char *dst, size_t &size)
    {
        // size_t offset;
        size_t cons;
        char *src = tryPop(cons, size);
        if(src != nullptr)
        {
            memcpy(dst, src, size);
            completePop(cons, size);
            
            return true;
        }

        return false;
    }

    inline char *tryPush(size_t &prod, size_t size)
    {
        prod = ph_.load(std::memory_order_relaxed);
        for(;;)
        {
            size_t wmark = wm_.load(std::memory_order_relaxed);
            size_t cons = ct_.load(std::memory_order_acquire);
            // if(size <= buffer_.size() - (prod - cons - unused()))
            if(size <= buffer_.size() - (prod - cons))
            {
                /// prod leads
                if(wrap(prod) >= wrap(cons))
                {
                    /// not enough space    ?
                    if(size > buffer_.size() - wrap(prod))
                    {
                        if(size <= wrap(cons))
                        {
                            if(!ph_.compare_exchange_weak(prod, next(prod) + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                            if(!wm_.compare_exchange_weak(wmark, prod, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                            return buffer_.data() + wrap(next(prod));
                        }
                        else
                        {
                            // LOGE("OVERRUN");dump();
                            return nullptr;
                        }
                    }
                    else
                    {
                        if(!ph_.compare_exchange_weak(prod, prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                        else break;
                    }
                }
                /// cons leads
                else
                {
                    if(size > wrap(cons) - wrap(prod))
                    {
                        // LOGE("OVERRUN");dump();
                        return nullptr;
                    }
                    if(!ph_.compare_exchange_weak(prod, prod + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                    else break;
                }
            }
            else
            {
                // LOGE("OVERRUN");dump();
                return nullptr;
            }
        }
        // wsize_ = size;
        return buffer_.data() + wrap(prod);
    }

    inline void completePush(size_t prod, size_t size)
    {
        for(;pt_.load(std::memory_order_relaxed) != prod;)
        {}

        if(wrap(prod) == 0) wm_.store(prod, std::memory_order_relaxed);

        if(prod == wm_.load(std::memory_order_relaxed))
            pt_.store(next(prod) + size, std::memory_order_release);
        else
            pt_.store(prod + size, std::memory_order_release);
    }

    inline bool push(const char *src, size_t size)
    {
        // size_t offset;
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


    inline size_t wrap(size_t index) const
    {
        return index & (buffer_.size() - 1);
    }

    inline size_t size() const
    {
        return pt_.load(std::memory_order_relaxed) - ch_.load(std::memory_order_relaxed) - unused();
    }

    inline size_t freeSize() const
    {
        return buffer_.size() - (ph_.load(std::memory_order_relaxed) - ct_.load(std::memory_order_relaxed) - unused());
    }

    inline size_t unused() const
    {
        size_t cons = ct_.load(std::memory_order_relaxed);
        size_t wmark = wm_.load(std::memory_order_relaxed);
        return cons < wmark ? buffer_.size() - wrap(wmark) : 0;
    }

    inline size_t next(size_t index) const
    {
        size_t tmp = buffer_.size() - 1;
        return (index + tmp) & (~tmp);
        // return wrap(index) ? index + (buffer_.size() - wrap(index)) : index;
    }

    std::atomic<size_t> ph_{0}, pt_{0}, wm_{0}, ch_{0}, ct_{0};
    std::vector<char> buffer_; /// 1Kb
};

} /// tll::lf
