/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include "../util.h"

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
        std::scoped_lock lock(pop_mtx_);
        cons = ch_;
        size_t next_cons = cons;
        size_t prod = pt_;
        size_t wmark = wm_;
        pop_count++;
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

        ch_ = next_cons + size;
        return buffer_.data() + wrap(next_cons);
    }

    inline void completePop(size_t cons, size_t size)
    {
        std::scoped_lock lock(pop_mtx_);
        if(cons == wm_)
            ct_ = next(cons) + size;
        else
            ct_ = cons + size;
        pop_hit_count++;
        pop_size+=size;
    }

    inline bool pop(char *dst, size_t &size)
    {
        std::scoped_lock lock(pop_mtx_);
        size_t cons;
        char *src = tryPop(cons, size);
        if(src != nullptr)
        {
            memcpy(dst, src, size);
            completePop(cons, size);

            return true;
        }
        // dump();
        return false;
    }

    inline char *tryPush(size_t &prod, size_t size)
    {
        std::scoped_lock lock(push_mtx_);
        prod = ph_;
        size_t cons = ct_;
        push_count++;
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
                        ph_ = next(prod) + size;
                        // wm_ = prod;
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
                    ph_ = prod + size;
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
                ph_ = prod + size;
            }
        }
        else
        {
            // LOGE("OVERRUN");dump();
            return nullptr;
        }
        return buffer_.data() + wrap(prod);
    }

    inline void completePush(size_t prod, size_t size)
    {
        std::scoped_lock lock(push_mtx_);
        // if(wrap(prod) == 0) wm_ = prod;
        if(prod + size > next(prod))
        {
        // if(prod == wm_)
            wm_ = prod;
            pt_ = next(prod) + size;
        }
        else
            pt_ = prod + size;
        push_hit_count++;
        push_size += size;
    }

    inline bool push(const char *src, size_t size)
    {
        std::scoped_lock lock(push_mtx_);
        size_t prod;
        char *dst = tryPush(prod, size);
        if(dst)
        {
            memcpy(dst, src, size);
            completePush(prod, size);
            return true;
        }
        // dump();
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
            .push_size = push_size, .pop_size = pop_size,
            .push_count = push_count, .pop_count = pop_count,
            .push_hit_count = push_hit_count, .pop_hit_count = pop_hit_count,
            .push_miss_count = 0, .pop_miss_count = 0,
        };
    }

    size_t push_size=0, pop_size=0;
    size_t push_count=0, pop_count=0;
    size_t push_hit_count=0, pop_hit_count=0;

    size_t ph_=0, pt_=0, ch_=0, ct_=0, wm_=0;
    std::vector<char> buffer_{}; /// 1Kb
    std::recursive_mutex push_mtx_, pop_mtx_;
};
} /// tll::lk
