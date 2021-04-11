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

namespace tll::lf2 {

static thread_local tll::time::Counter<StatDuration, StatClock> counter;

struct Marker
{
    Marker() = default;
    inline auto &index_head() {return index_head_;}
    inline auto const &index_head() const {return index_head_;}
    inline auto &index_tail() {return index_tail_;}
    inline auto const &index_tail() const {return index_tail_;}

    inline auto &entry_id_head() {return entry_id_head_;}
    inline auto &entry_id_tail() {return entry_id_tail_;}
    inline auto &exit_id() {return exit_id_;}
    inline auto &exit_id_list(size_t i) {return exit_id_list_[i];}
    inline auto &next_index_list(size_t i) {return next_index_list_[i];}
    // inline auto &curr_index_list(size_t i) {return curr_index_list_[i];}
    inline auto &size_list(size_t i) {return size_list_[i];}

    ~Marker()
    {
        if(exit_id_list_) delete exit_id_list_;
        if(next_index_list_) delete next_index_list_;
        // if(curr_index_list_) delete curr_index_list_;
        if(size_list_) delete size_list_;
    }

    inline void reset(size_t capacity, size_t list_size)
    {
        if(this->list_size != list_size)
        {
            this->list_size = list_size;
            if(exit_id_list_) delete exit_id_list_;
            if(next_index_list_) delete next_index_list_;
            // if(curr_index_list_) delete curr_index_list_;
            if(size_list_) delete size_list_;
            exit_id_list_ = new std::atomic<size_t> [list_size];
            next_index_list_ = new std::atomic<size_t> [list_size];
            // curr_index_list_ = new std::atomic<size_t> [list_size];
            size_list_ = new std::atomic<size_t> [list_size];
        }

        index_head_.store(0, std::memory_order_relaxed);
        index_tail_.store(0, std::memory_order_relaxed);
        entry_id_head_.store(capacity, std::memory_order_relaxed);
        entry_id_tail_.store(capacity, std::memory_order_relaxed);
        exit_id_.store(capacity, std::memory_order_relaxed);

        for(int i=0; i<list_size; i++)
        {
            exit_id_list_[i].store(0, std::memory_order_relaxed);
        }
    }
    std::atomic<size_t> index_head_{0};
    std::atomic<size_t> index_tail_{0};

    std::atomic<size_t> entry_id_head_{0};
    std::atomic<size_t> entry_id_tail_{0};
    std::atomic<size_t> exit_id_{0};

    std::atomic<size_t> *exit_id_list_{nullptr};
    std::atomic<size_t> *next_index_list_{nullptr};
    // std::atomic<size_t> *curr_index_list_{nullptr};
    std::atomic<size_t> *size_list_{nullptr};

    size_t list_size{0};
};

enum class Mode
{
    kLL,
    kHL,
    kMax
};

namespace mode
{
    constexpr Mode low_load = Mode::kLL;
    constexpr Mode high_load = Mode::kHL;
}

/// Contiguous Circular Index
// template <size_t num_threads=0x400>
template <typename T, Mode prod_mode=Mode::kHL, Mode cons_mode=Mode::kLL, bool profiling=false>
struct ccfifo
{
public:
    ccfifo() = default;
    ~ccfifo() = default;

    ccfifo(size_t size, size_t num_threads=0x400)
    {
        reserve(size, num_threads);
    }

    inline auto dump() const
    {
        size_t ph = producer_.index_head().load(std::memory_order_relaxed);
        size_t pt = producer_.index_tail().load(std::memory_order_relaxed);

        size_t ch = consumer_.index_head().load(std::memory_order_relaxed);
        size_t ct = consumer_.index_tail().load(std::memory_order_relaxed);

        size_t wm = water_mark_.load(std::memory_order_relaxed);

        return util::stringFormat("ph:%ld/%ld pt:%ld/%ld ch:%ld/%ld ct:%ld/%ld wm:%ld/%ld sz:%ld", 
            wrap(ph), ph, wrap(pt), pt, wrap(ch), ch, wrap(ct), ct, wrap(wm), wm, capacity_);
    }

    inline void reset()
    {
        water_mark_.store(0,std::memory_order_relaxed);
        producer_.reset(capacity_, num_threads_);
        consumer_.reset(capacity_, num_threads_);
    }

    inline void reserve(size_t size, size_t num_threads=0x400)
    {
        capacity_ = util::isPowerOf2(size) ? size : util::nextPowerOf2(size);
        if(num_threads > capacity_)
            num_threads_ = capacity_;
        else
            num_threads_ = tll::util::isPowerOf2(num_threads) ? num_threads : tll::util::nextPowerOf2(num_threads);
#if (!defined NO_ALLOCATE)
        buffer_.resize(capacity_);
#endif
        reset();
    }

    template <Mode mode>
    size_t tryPop(size_t &entry_id, size_t &cons_index_head, size_t &size)
    {
        size_t cons_next_index;
        size_t entry_id_head = consumer_.entry_id_head().load(std::memory_order_relaxed);
        cons_index_head = consumer_.index_head().load(std::memory_order_relaxed);
        size_t next_size = size;
        for(;;profAdd(stat_push_miss, 1))
        {
            if(mode == mode::high_load)
            {
                while(entry_id_head != consumer_.entry_id_tail().load(std::memory_order_relaxed))
                    entry_id_head = consumer_.entry_id_head().load(std::memory_order_relaxed);

                cons_index_head = consumer_.index_head().load(std::memory_order_relaxed);
            }

            cons_next_index = cons_index_head;
            size_t prod_index = producer_.index_tail().load(std::memory_order_relaxed);
            size_t wmark = water_mark_.load(std::memory_order_relaxed);
            if(cons_next_index < prod_index)
            {
                if(cons_next_index == wmark)
                {
                    cons_next_index = next(cons_next_index);
                    if(prod_index <= cons_next_index)
                    {
                        // LOGE("Underrun!");dump();
                        next_size = 0;
                        // profAdd(stat_pop_error, 1);
                        break;
                    }
                    if(size > (prod_index - cons_next_index))
                        next_size = prod_index - cons_next_index;
                }
                else if(cons_next_index < wmark)
                {
                    if(size > (wmark - cons_next_index))
                        next_size = wmark - cons_next_index;
                }
                else /// cons > wmark
                {
                    if(size > (prod_index - cons_next_index))
                    {
                        next_size = prod_index - cons_next_index;
                    }
                }

                if(mode == mode::high_load)
                {
                    if(!consumer_.entry_id_head().compare_exchange_weak(entry_id_head, entry_id_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        continue;
                    }
                    else
                    {
                        consumer_.index_head().store(cons_next_index + next_size, std::memory_order_relaxed);
                        consumer_.entry_id_tail().store(entry_id_head + 1, std::memory_order_relaxed);
                        entry_id = entry_id_head;
                        break;
                    }
                }
                else
                {
                    if(!consumer_.index_head().compare_exchange_weak(cons_index_head, cons_next_index + next_size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                    else break;
                }
            }
            else
            {
                // LOGE("Underrun!");dump();
                next_size = 0;
                // profAdd(stat_pop_error, 1);
                break;
            }
        }
        size = next_size;
        profAdd(stat_pop_total, 1);
        return cons_next_index;
    }

    template <Mode mode>
    bool completePop(size_t entry_id, size_t curr_index, size_t next_index, size_t size)
    {
        if(mode == mode::high_load)
        {
            if(entry_id >= (consumer_.exit_id().load(std::memory_order_relaxed) + num_threads_)) return false;

            size_t t_pos = wrap(entry_id, num_threads_);
            consumer_.size_list(wrap(entry_id, num_threads_)).store(size);
            // consumer_.curr_index_list(wrap(entry_id, num_threads_)).store(curr_index);
            consumer_.next_index_list(wrap(entry_id, num_threads_)).store(next_index);
            size_t const kIdx = entry_id;
            size_t next_exit_id = consumer_.exit_id_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
            // LOGD("%ld/%ld %ld/%ld %s", curr_index, next_index, next_exit_id, entry_id, dump().data());

            for(;;stat_pop_exit_miss.fetch_add(1, std::memory_order_relaxed))
            {
                // size_t next_exit_id;
                size_t curr_exit_id = consumer_.exit_id().load(std::memory_order_relaxed);

                // LOGD(">(%ld/%ld)\t{%ld/%ld}", kIdx, entry_id, curr_exit_id, next_exit_id);
                if(entry_id < curr_exit_id)
                {
                    break;
                }
                else if (entry_id == curr_exit_id)
                {
                    next_exit_id = entry_id;
                    while((entry_id - curr_exit_id < num_threads_) && (next_exit_id >= curr_exit_id))
                    {
                        size = consumer_.size_list(wrap(next_exit_id, num_threads_)).load(std::memory_order_relaxed);
                        // curr_index = consumer_.curr_index_list(wrap(next_exit_id, num_threads_)).load(std::memory_order_relaxed);
                        next_index = consumer_.next_index_list(wrap(next_exit_id, num_threads_)).load(std::memory_order_relaxed);

                        consumer_.index_tail().store(next_index + size, std::memory_order_relaxed);
                        entry_id++;
                        next_exit_id = consumer_.exit_id_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
                        // LOGD(" -(%ld)\t%ld/%ld\t%ld/%ld/%ld\t%s", kIdx, 
                        //      next_exit_id, entry_id,
                        //      curr_index, next_index, size, 
                        //      dump().data());

                    }

                    consumer_.exit_id().store(entry_id, std::memory_order_relaxed);
                    // consumer_.index_tail().store(next_index + size, std::memory_order_relaxed);
                }

                // LOGD(" (%ld)\t%ld/%ld\t%s", 
                //          kIdx, 
                //          next_exit_id, entry_id, 
                //          dump().data());
                if(consumer_.exit_id_list(wrap(entry_id, num_threads_)).compare_exchange_strong(next_exit_id, kIdx, std::memory_order_relaxed, std::memory_order_relaxed)) break;

                // LOGD(" M(%ld/%ld) {%ld}", kIdx, entry_id, consumer_.exit_id().load(std::memory_order_relaxed));
            }
        }
        else
        {
            for(;consumer_.index_tail().load(std::memory_order_relaxed) != curr_index;){}
            consumer_.index_tail().store(next_index + size, std::memory_order_relaxed);
        }
        // LOGD(" <(%ld/%ld) {%ld}", kIdx, entry_id, consumer_.exit_id().load(std::memory_order_relaxed));

        return true;
    }

    template <Mode mode>
    size_t tryPush(size_t &entry_id, size_t &prod_index_head, size_t &size)
    {
        size_t prod_next_index;
        size_t entry_id_head = producer_.entry_id_head().load(std::memory_order_relaxed);
        prod_index_head = producer_.index_head().load(std::memory_order_relaxed);

        for(;;profAdd(stat_push_miss, 1))
        {
            if(mode == mode::high_load)
            {
                while(entry_id_head != producer_.entry_id_tail().load(std::memory_order_relaxed))
                    entry_id_head = producer_.entry_id_head().load(std::memory_order_relaxed);

                prod_index_head = producer_.index_head().load(std::memory_order_relaxed);
            }

            prod_next_index = prod_index_head;
            size_t cons_index = consumer_.index_tail().load(std::memory_order_relaxed);
            if(size <= capacity() - (prod_next_index - cons_index))
            {
                /// prod leads
                if(wrap(prod_next_index) >= wrap(cons_index))
                {
                    /// not enough space    ?
                    if(size > capacity() - wrap(prod_next_index))
                    {
                        if(size <= wrap(cons_index))
                        {
                            prod_next_index = next(prod_next_index);
                        }
                        else
                        {
                            // LOGE("OVERRUN");dump();
                            size = 0;
                            break;
                        }
                    }
                }
                /// cons leads
                else
                {
                    if(size > wrap(cons_index) - wrap(prod_next_index))
                    {
                        // LOGE("OVERRUN");dump();
                        size = 0;
                        break;
                    }
                }

                if(mode == mode::high_load)
                {
                    if(!producer_.entry_id_head().compare_exchange_weak(entry_id_head, entry_id_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        continue;
                    }
                    else
                    {
                        producer_.index_head().store(prod_next_index + size, std::memory_order_relaxed);
                        producer_.entry_id_tail().store(entry_id_head + 1, std::memory_order_relaxed);
                        entry_id = entry_id_head;
                        break;
                    }
                }
                else
                {
                    if(!producer_.index_head().compare_exchange_weak(prod_index_head, prod_next_index + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
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
        return prod_next_index;
    }

    template <Mode mode>
    bool completePush(size_t entry_id, size_t curr_index, size_t next_index, size_t size)
    {
        if(mode == mode::high_load)
        {
            if(entry_id >= (producer_.exit_id().load(std::memory_order_relaxed) + num_threads_)) return false;

            size_t t_pos = wrap(entry_id, num_threads_);
            producer_.size_list(wrap(entry_id, num_threads_)).store(size);
            // producer_.curr_index_list(wrap(exit_id, num_threads_)).store(curr_index);
            producer_.next_index_list(wrap(entry_id, num_threads_)).store(next_index);
            size_t const kIdx = entry_id;
            size_t next_exit_id = producer_.exit_id_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
            // LOGD("%ld/%ld %ld/%ld %s", curr_index, next_index, next_exit_id, entry_id, dump().data());

            for(;;stat_push_exit_miss.fetch_add(1, std::memory_order_relaxed))
            {
                // size_t next_exit_id;
                size_t curr_exit_id = producer_.exit_id().load(std::memory_order_relaxed);

                // LOGD(">(%ld/%ld)\t{%ld/%ld}", kIdx, entry_id, curr_exit_id, next_exit_id);
                if(entry_id < curr_exit_id)
                {
                    break;
                }
                else if (entry_id == curr_exit_id)
                {
                    next_exit_id = entry_id;
                    while((entry_id - curr_exit_id < num_threads_) && (next_exit_id >= curr_exit_id))
                    {
                        size = producer_.size_list(wrap(next_exit_id, num_threads_)).load(std::memory_order_relaxed);
                        curr_index = producer_.index_tail().load(std::memory_order_relaxed);
                        next_index = producer_.next_index_list(wrap(next_exit_id, num_threads_)).load(std::memory_order_relaxed);

                        if(next_index >= this->next(curr_index))
                        {
                            // producer_.index_tail().store(curr_index, std::memory_order_relaxed);
                            water_mark_.store(curr_index, std::memory_order_relaxed);
                            // LOGD("%ld/%ld\t\t%s", curr_index, next_index, dump().data());
                        }
                        producer_.index_tail().store(next_index + size, std::memory_order_relaxed);
                        entry_id++;
                        next_exit_id = producer_.exit_id_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
                        // LOGD(" -(%ld)\t%ld/%ld\t%ld/%ld/%ld\t%s", kIdx, 
                        //      next_exit_id, exit_id,
                        //      curr_index, next_index, size, 
                        //      dump().data());

                    }

                    producer_.exit_id().store(entry_id, std::memory_order_relaxed);
                    // producer_.index_tail().store(next_index + size, std::memory_order_relaxed);
                }

                // LOGD(" (%ld)\t%ld/%ld\t%s", 
                //          kIdx, 
                //          next_exit_id, entry_id, 
                //          dump().data());
                if(producer_.exit_id_list(wrap(entry_id, num_threads_)).compare_exchange_strong(next_exit_id, entry_id, std::memory_order_relaxed, std::memory_order_relaxed)) break;

                // LOGD(" M(%ld/%ld) {%ld}", kIdx, entry_id, producer_.exit_id().load(std::memory_order_relaxed));
            }
        }
        else
        {
            for(;producer_.index_tail().load(std::memory_order_relaxed) != curr_index;){}
            if(next_index >= this->next(curr_index))
            {
                water_mark_.store(curr_index, std::memory_order_relaxed);
            }
            producer_.index_tail().store(next_index + size, std::memory_order_release);
        }
        // LOGD(" <(%ld/%ld) {%ld}", kIdx, exit_id, producer_.exit_id().load(std::memory_order_relaxed));

        return true;
    }

    template <typename F, typename ...Args>
    size_t pop(F &&callback, size_t size, Args &&...args)
    {
        size_t id, index;
        profTimerReset();
        size_t next = tryPop<cons_mode>(id, index, size);
        profTimerElapse(time_pop_try);
        profTimerStart();
        if(size)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            callback(elemAt(next), size, std::forward<Args>(args)...);
            profTimerElapse(time_pop_cb);
            profTimerStart();
            while(!completePop<cons_mode>(id, index, next, size)){};
            profTimerElapse(time_pop_complete);
            profAdd(stat_pop_total, 1);
            profAdd(stat_pop_size, size);
        }
        else
        {
            profAdd(stat_pop_error, 1);
        }
        profTimerAbsElapse(time_pop_total);
        return size;
    }

    template <typename F, typename ...Args>
    size_t push(const F &callback, size_t size, Args &&...args)
    {
        size_t id, index;
        profTimerReset();
        size_t next = tryPush<prod_mode>(id, index, size);
        profTimerElapse(time_push_try);
        profTimerStart();
        if(size)
        {
            callback(elemAt(next), size, std::forward<Args>(args)...);
            std::atomic_thread_fence(std::memory_order_release);
            profTimerElapse(time_push_cb);
            profTimerStart();
            while(!completePush<prod_mode>(id, index, next, size)){};
            profTimerElapse(time_push_complete);
            profAdd(stat_push_total, 1);
            profAdd(stat_push_size, size);
        }
        else
        {
            profAdd(stat_push_error, 1);
        }
        profTimerAbsElapse(time_push_total);
        return size;
    }

    template<typename U>
    size_t push(U &&val)
    {
        return push([&val](T *el, size_t){ *el = std::forward<U>(val); }, 1);
    }

    size_t pop(T &val)
    {
        return pop([&val](T *el, size_t){ val = std::move(*el); }, 1);
    }

    inline bool completeQueue(size_t idx, int type)
    {
        size_t t_pos = wrap(idx, num_threads_);
        auto &tail = (type == 0) ? producer_.index_tail() : consumer_.index_tail();
        auto &out_index = (type == 0) ? producer_.exit_id_list_ : consumer_.exit_id_list_;

        if(idx >= (tail.load(std::memory_order_relaxed) + num_threads_)) return false;
        
        size_t const kIdx = idx;

        for(;;)
        {
            size_t next_idx = out_index[wrap(idx, num_threads_)].load(std::memory_order_relaxed);
            size_t crr_tail = tail.load(std::memory_order_relaxed);
            // LOGD(">(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads_), crr_tail, next_idx, this->to_string(out_index).data());
            if(crr_tail > idx)
            {
                // LOGD("E-(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads_), crr_tail, next_idx, this->to_string(out_index).data());
                break;
            }
            else if (crr_tail == idx)
            {
                size_t cnt = 1;
                next_idx = out_index[wrap(idx + cnt, num_threads_)].load(std::memory_order_relaxed);
                for(; (cnt<num_threads_) && (next_idx>crr_tail); cnt++)
                {
                    next_idx = out_index[wrap(idx + cnt + 1, num_threads_)].load(std::memory_order_relaxed);
                }

                idx += cnt;
                // LOGD("=-(%ld:%ld:%ld:%ld) [%ld] +%ld {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads_), crr_tail, cnt, next_idx, this->to_string(out_index).data());
                // tail.store(crr_tail + cnt);
                tail.fetch_add(cnt, std::memory_order_relaxed);
            }

            if(out_index[wrap(idx, num_threads_)].compare_exchange_weak(next_idx, kIdx, std::memory_order_relaxed, std::memory_order_relaxed)) break;

            // LOGD("M-(%ld:%ld:%ld:%ld) [%ld] {%ld %s}", kIdx, idx, t_pos, wrap(idx, num_threads_), crr_tail, next_idx, this->to_string(out_index).data());
        }

        return true;
    }

    template <typename F, typename ...Args>
    bool deQueue(const F &callback, Args &&...args)
    {
        profTimerReset();
        bool ret = false;
        size_t idx = consumer_.index_head().load(std::memory_order_relaxed);
        for(;;)
        {
            if (idx == producer_.index_tail().load(std::memory_order_relaxed))
            {
                profAdd(stat_pop_error, 1);
                break;
            }

            if(consumer_.index_head().compare_exchange_weak(idx, idx + 1, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                ret = true;
                break;
            }
            profAdd(stat_pop_miss, 1);
        }
        // profAdd(time_pop_try, timer.elapse().count());
        profTimerElapse(time_pop_try);
        profTimerStart();
        if(ret)
        {
            callback(elemAt(idx), std::forward<Args>(args)...);
            // profAdd(time_pop_cb, timer.elapse().count());
            profTimerElapse(time_pop_cb);
            profTimerStart();
            // while(!completeQueue(idx, cons_tail_, cons_out_)) {std::this_thread::yield();}
            while(!completeQueue(idx, 1)) {std::this_thread::yield();}
            // profAdd(time_pop_complete, timer.elapse().count());
            profTimerElapse(time_pop_complete);
            profAdd(stat_pop_size, 1);
        }
        // profAdd(time_pop_total, timer.absElapse().count());
        profTimerAbsElapse(time_pop_total);
        profAdd(stat_pop_total, 1);
        return ret;
    }

    template <typename F, typename ...Args>
    bool enQueue(const F &callback, Args &&...args)
    {
        profTimerReset();
        bool ret = false;
        size_t idx = producer_.index_head().load(std::memory_order_relaxed);
        for(;;)
        {
            if (idx == (consumer_.index_tail().load(std::memory_order_relaxed) + capacity_))
            {
                profAdd(stat_push_error, 1);
                break;
            }

            if(producer_.index_head().compare_exchange_weak(idx, idx + 1, std::memory_order_acquire, std::memory_order_relaxed))
            {
                ret = true;
                break;
            }
            profAdd(stat_push_miss, 1);
        }
        // profAdd(time_push_try, timer.elapse().count());
        profTimerElapse(time_push_try);
        profTimerStart();
        if(ret)
        {
            callback(elemAt(idx), std::forward<Args>(args)...);
            // profAdd(time_push_cb, timer.elapse().count());
            profTimerElapse(time_push_cb);
            profTimerStart();
            // while(!completeQueue(idx, prod_tail_, prod_out_)) {std::this_thread::yield();}
            while(!completeQueue(idx, 0)) {std::this_thread::yield();}
            // profAdd(time_push_complete, timer.elapse().count());
            profTimerElapse(time_push_complete);
            profAdd(stat_push_size, 1);
        }
        // profAdd(time_push_total, timer.absElapse().count());
        profTimerAbsElapse(time_push_total);
        profAdd(stat_push_total, 1);
        return ret;
    }

    // template<typename U>
    bool enQueue(const T &val)
    {
        return enQueue([&val](T *el){ *el = val;});
    }

    bool enQueue(T &&val)
    {
        return enQueue([&val](T *el){ *el = std::move(val);});
    }

    bool deQueue(T &val)
    {
        return deQueue([&val](T *el){ val = std::move(*el);});
    }


/// utility functions
    inline size_t capacity() const
    {
        return capacity_;
    }

    inline size_t size() const
    {
        return producer_.index_tail().load(std::memory_order_relaxed) - consumer_.index_head().load(std::memory_order_relaxed);
    }

    inline bool empty() const
    {
        return producer_.index_tail().load(std::memory_order_relaxed) == consumer_.index_head().load(std::memory_order_relaxed);
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

    inline tll::cc::Statistic statistic() const
    {
        return tll::cc::Statistic {
            .time_push_total = time_push_total.load(std::memory_order_relaxed), .time_pop_total = time_pop_total.load(std::memory_order_relaxed),
            .time_push_cb = time_push_cb.load(std::memory_order_relaxed), .time_pop_cb = time_pop_cb.load(std::memory_order_relaxed),
            .time_push_try = time_push_try.load(std::memory_order_relaxed), .time_pop_try = time_pop_try.load(std::memory_order_relaxed),
            .time_push_complete = time_push_complete.load(std::memory_order_relaxed), .time_pop_complete = time_pop_complete.load(std::memory_order_relaxed),
            .push_size = stat_push_size.load(std::memory_order_relaxed), .pop_size = stat_pop_size.load(std::memory_order_relaxed),
            .push_total = stat_push_total.load(std::memory_order_relaxed), .pop_total = stat_pop_total.load(std::memory_order_relaxed),
            .push_error = stat_push_error.load(std::memory_order_relaxed), .pop_error = stat_pop_error.load(std::memory_order_relaxed),
            .push_miss = stat_push_miss.load(std::memory_order_relaxed), .pop_miss = stat_pop_miss.load(std::memory_order_relaxed),
            .push_exit_miss = stat_push_exit_miss.load(std::memory_order_relaxed), .pop_exit_miss = stat_pop_exit_miss.load(std::memory_order_relaxed),
        };
    }

    inline size_t ph() const {return producer_.index_head().load(std::memory_order_relaxed);}
    inline size_t pt() const {return producer_.index_tail().load(std::memory_order_relaxed);}
    inline size_t ch() const {return consumer_.index_head().load(std::memory_order_relaxed);}
    inline size_t ct() const {return consumer_.index_tail().load(std::memory_order_relaxed);}
    inline size_t wm() const {return water_mark_.load(std::memory_order_relaxed);}
    // inline size_t pef() const {return prod_exit_flag_.load(std::memory_order_relaxed);}
    // inline size_t cef() const {return cons_exit_flag_.load(std::memory_order_relaxed);}
    // inline std::string to_string(int type)
    // {
    //     std::string ret;
    //     ret.resize(num_threads_);
    //     size_t ct = this->ct();
    //     auto &idxs = type == 0 ? prod_exit_id_list_ : cons_exit_id_list_;
    //     for(int i=0; i<num_threads_; i++)
    //     {
    //         if(idxs[i].load(std::memory_order_relaxed) > ct)
    //             ret[i] = '^';
    //         else
    //             ret[i] = '.';
    //     }
    //     return ret;
    // }

    inline T *elemAt(size_t id)
    {
        return &buffer_[wrap(id)];
    }

    inline T &operator[](size_t id)
    {
        return buffer_[wrap(id)];
    }

    inline const T &operator[](size_t id) const
    {
        return buffer_[wrap(id)];
    }

private:

    inline ccfifo<T, prod_mode, cons_mode, profiling> &profTimerReset()
    {
        if(profiling)
            counter.reset();
        return *this;
    }

    inline ccfifo<T, prod_mode, cons_mode, profiling> &profTimerStart()
    {
        if(profiling)
            counter.start();
        return *this;
    }

    inline ccfifo<T, prod_mode, cons_mode, profiling> &profTimerElapse(auto &atomic)
    {
        if(profiling) 
            atomic.fetch_add(counter.elapse().count(), std::memory_order_relaxed);
        return *this;
    }

    inline ccfifo<T, prod_mode, cons_mode, profiling> &profTimerAbsElapse(auto &atomic)
    {
        if(profiling) 
            atomic.fetch_add(counter.absElapse().count(), std::memory_order_relaxed);
        return *this;
    }

    template <typename U>
    inline ccfifo<T, prod_mode, cons_mode, profiling> &profAdd(auto &atomic, const U &val)
    {
        if(profiling)
            atomic.fetch_add(val, std::memory_order_relaxed);
        return *this;
    }

    inline size_t next(size_t index) const
    {
        size_t tmp = capacity() - 1;
        return (index + tmp) & (~tmp);
    }

private:
    Marker producer_;
    Marker consumer_;
    std::atomic<size_t> water_mark_{0};
    size_t capacity_{0}, num_threads_{0};

    /// Statistic
    std::atomic<size_t> stat_push_size{0};
    std::atomic<size_t> stat_push_total{0};
    std::atomic<size_t> stat_push_error{0};
    std::atomic<size_t> stat_push_miss{0};
    std::atomic<size_t> stat_push_exit_miss{0};
    std::atomic<size_t> time_push_total{0};
    std::atomic<size_t> time_push_cb{0};
    std::atomic<size_t> time_push_try{0};
    std::atomic<size_t> time_push_complete{0};

    std::atomic<size_t> stat_pop_size{0};
    std::atomic<size_t> stat_pop_total{0};
    std::atomic<size_t> stat_pop_error{0};
    std::atomic<size_t> stat_pop_miss{0};
    std::atomic<size_t> stat_pop_exit_miss{0};
    std::atomic<size_t> time_pop_total{0};
    std::atomic<size_t> time_pop_cb{0};
    std::atomic<size_t> time_pop_try{0};
    std::atomic<size_t> time_pop_complete{0};

    std::vector<T> buffer_;
};

} /// tll::lf
