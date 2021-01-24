/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#pragma once

#include "../util.h"

namespace tll::mt {
/// Circular Queue
template <typename T, size_t const kELemSize=sizeof(T)>
class CQueue
{
public:
    CQueue(uint32_t num_of_elem)
    {
        capacity_ = isPowerOf2(num_of_elem) ? num_of_elem : nextPowerOf2(num_of_elem);
        buffer_.resize(capacity_ * kELemSize);
    }

    TLL_INLINE bool tryPop(uint32_t &cons)
    {
        cons = ch_.load(std::memory_order_relaxed);

        for(;!isEmpty();)
        {
            if (cons == pt_.load(std::memory_order_relaxed))
                return false;

            if(ch_.compare_exchange_weak(cons, cons + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }

        return false;
    }

    TLL_INLINE bool completePop(uint32_t cons)
    {
        if (ct_.load(std::memory_order_relaxed) != cons)
            return false;

        ct_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template </*typename D,*/ typename F, typename ...Args>
    bool pop(F &&doPop, Args &&...args)
    {
        uint32_t cons;
        while(!tryPop(cons)) 
        {
            if (isEmpty()) 
                return false;

            std::this_thread::yield();
        }

        std::forward<F>(doPop)(elemAt(cons), kELemSize, std::forward<Args>(args)...);

        while(!completePop(cons)) 
            std::this_thread::yield();

        return true;
    }

    /// ONE consumer only
    template </*typename D,*/ typename F, typename ...Args>
    bool popBatch(uint32_t max_num, F &&onPopBatch, Args &&...args)
    {
        if (isEmpty())
            return false;

        uint32_t cons = ch_.load(std::memory_order_relaxed);
        uint32_t elem_num = size();

        if(elem_num > max_num)
            elem_num = max_num;

        while(cons + elem_num > pt_.load(std::memory_order_relaxed))
            std::this_thread::yield();

        ch_.fetch_add(elem_num, std::memory_order_acquire);

        std::forward<F>(onPopBatch)(cons, elem_num, kELemSize, std::forward<Args>(args)...);

        ct_.fetch_add(elem_num, std::memory_order_release);

        return true;
    }

    TLL_INLINE bool tryPush(uint32_t &prod)
    {
        prod = ph_.load(std::memory_order_relaxed);

        for(;!isFull();)
        {
            if (prod == (ct_.load(std::memory_order_relaxed) + capacity_))
                return false;

            if(ph_.compare_exchange_weak(prod, prod + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }
        return false;
    }

    TLL_INLINE bool completePush(uint32_t prod)
    {
        if (pt_.load(std::memory_order_relaxed) != prod)
            return false;

        pt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template </*typename D, */typename F, typename ...Args>
    bool push(F &&doPush, Args&&...args)
    {
        uint32_t prod;
        prod = ph_.load(std::memory_order_relaxed);
        while(!tryPush(prod))
        {
            if(isFull()) return false;
            std::this_thread::yield();
        }

        std::forward<F>(doPush)(elemAt(prod), kELemSize, std::forward<Args>(args)...);
        while(!completePush(prod)) 
            std::this_thread::yield();

        return true;
    }

    TLL_INLINE uint32_t size() const
    {
        return ph_.load(std::memory_order_relaxed) - ch_.load(std::memory_order_relaxed);
    }

    TLL_INLINE uint32_t wrap(uint32_t index) const
    {
        return index & (capacity_ - 1);
    }

    TLL_INLINE uint32_t capacity() const {
        return capacity_;
    }

    TLL_INLINE bool isEmpty() const {
        return size() == 0;
    }

    TLL_INLINE bool isFull() const {
        return size() == capacity();
    }

    TLL_INLINE T &elemAt(uint32_t index)
    {
        return buffer_[kELemSize * wrap(index)];
    }

    TLL_INLINE T const &elemAt(uint32_t index) const
    {
        return buffer_[kELemSize * wrap(index)];
    }

    TLL_INLINE size_t elemSize() const
    {
        return kELemSize;
    }

private:

    std::atomic<uint32_t> pt_{0}, ph_{0}, ct_{0}, ch_{0};
    uint32_t capacity_{0};
    std::vector<T> buffer_{};
}; /// CQueue
} /// tll::lk