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

namespace st {
static thread_local tll::time::Counter<StatDuration, StatClock> counter;
}

struct Statistic {
    size_t total_size{0},
        try_count{0},
        error_count{0},
        try_miss_count{0},
        comp_miss_count{0},
        time_total{0},
        time_cb{0}, time_max_cb{0}, time_min_cb{0},
        time_try{0}, time_max_try{0}, time_min_try{0},
        time_comp{0}, time_max_comp{0}, time_min_comp{0};
};

typedef std::pair<Statistic, Statistic> Statistics;

// template <bool E=false>
struct Marker
{

    Marker() = default;
    inline auto &index_head() {return index_head_;}
    inline auto &index_tail() {return index_tail_;}
    inline auto &entry_id_head() {return entry_id_head_;}
    inline auto &entry_id_tail() {return entry_id_tail_;}
    inline auto &exit_id() {return exit_id_;}

    inline auto const &index_head() const {return index_head_;}
    inline auto const &index_tail() const {return index_tail_;}
    inline auto const &entry_id_head() const {return entry_id_head_;}
    inline auto const &entry_id_tail() const {return entry_id_tail_;}
    inline auto const &exit_id() const {return exit_id_;}

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
        entry_id_head_.store(0, std::memory_order_relaxed);
        entry_id_tail_.store(0, std::memory_order_relaxed);
        exit_id_.store(0, std::memory_order_relaxed);

        for(int i=0; i<list_size; i++)
        {
            exit_id_list_[i].store(0, std::memory_order_relaxed);
        }
    }

    Statistic statistic() const
    {
        return {
            .total_size = total_size.load(std::memory_order_relaxed),
            .try_count = try_count.load(std::memory_order_relaxed),
            .error_count = error_count.load(std::memory_order_relaxed),
            .try_miss_count = try_miss_count.load(std::memory_order_relaxed),
            .comp_miss_count = comp_miss_count.load(std::memory_order_relaxed),
            .time_total = time_total.load(std::memory_order_relaxed),
            .time_cb = time_cb.load(std::memory_order_relaxed),
            .time_max_cb = time_max_cb.load(std::memory_order_relaxed),
            .time_min_cb = time_min_cb.load(std::memory_order_relaxed),
            .time_try = time_try.load(std::memory_order_relaxed),
            .time_max_try = time_max_try.load(std::memory_order_relaxed),
            .time_min_try = time_min_try.load(std::memory_order_relaxed),
            .time_comp = time_comp.load(std::memory_order_relaxed),
            .time_max_comp = time_max_comp.load(std::memory_order_relaxed),
            .time_min_comp = time_min_comp.load(std::memory_order_relaxed),
        };
    }

    std::atomic<size_t> index_head_{0};
    std::atomic<size_t> index_tail_{0};

    std::atomic<size_t> entry_id_head_{0};
    std::atomic<size_t> entry_id_tail_{0};
    std::atomic<size_t> exit_id_{0};

    std::atomic<size_t> *exit_id_list_{nullptr};
    std::atomic<size_t> *next_index_list_{nullptr};
    std::atomic<size_t> *size_list_{nullptr};
    size_t list_size{0};

    std::atomic<size_t> total_size{0};
    std::atomic<size_t> try_count{0};
    std::atomic<size_t> error_count{0};
    std::atomic<size_t> try_miss_count{0};
    std::atomic<size_t> comp_miss_count{0};

    std::atomic<size_t> time_total{0};
    std::atomic<size_t> time_cb{0}, time_max_cb{0}, time_min_cb{-1U};
    std::atomic<size_t> time_try{0}, time_max_try{0}, time_min_try{-1U};
    std::atomic<size_t> time_comp{0}, time_max_comp{0}, time_min_comp{-1U};
};

enum class Mode
{
    kSparse,
    kDense,
    kMax
};

namespace mode
{
    constexpr Mode sparse = Mode::kSparse;
    constexpr Mode dense = Mode::kDense;
}

/// Contiguous Circular Index
// template <size_t num_threads=0x400>
template <typename T, Mode prod_mode=Mode::kDense, Mode cons_mode=Mode::kSparse, bool profiling=false>
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
        size_t peh = producer_.entry_id_head().load(std::memory_order_relaxed);
        size_t pet = producer_.entry_id_tail().load(std::memory_order_relaxed);
        size_t pex = producer_.exit_id().load(std::memory_order_relaxed);

        size_t ch = consumer_.index_head().load(std::memory_order_relaxed);
        size_t ct = consumer_.index_tail().load(std::memory_order_relaxed);
        size_t ceh = consumer_.entry_id_head().load(std::memory_order_relaxed);
        size_t cet = consumer_.entry_id_tail().load(std::memory_order_relaxed);
        size_t cex = consumer_.exit_id().load(std::memory_order_relaxed);

        size_t wm = water_mark_.load(std::memory_order_relaxed);

        return util::stringFormat("(p/(%ld:%ld) (%ld:%ld)) "
                                  "(c/(%ld:%ld) (%ld:%ld)) "
                                  "(wm/%ld:%ld sz/%ld) "
                                  "(pe/%ld:%ld:%ld) "
                                  "(ce/%ld:%ld:%ld) ",
            wrap(ph), wrap(pt), ph, pt,
            wrap(ch), wrap(ct), ch, ct,
            wrap(wm), wm, capacity_,
            peh, pet, pex,
            ceh, cet, cex);
    }

    inline void reset()
    {
        water_mark_.store(0,std::memory_order_relaxed);
        producer_.reset(capacity_, num_threads_);
        consumer_.reset(capacity_, num_threads_);
    }

    inline void reserve(size_t size)
    {
        reserve(size, num_threads_);
    }

    inline void reserve(size_t size, size_t num_threads)
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
        auto &marker = consumer_;
        size_t cons_next_index;
        size_t entry_id_head = marker.entry_id_head().load(std::memory_order_relaxed);
        cons_index_head = marker.index_head().load(std::memory_order_relaxed);
        size_t next_size = size;
        for(;;profAdd(marker.try_miss_count, 1))
        {
            if(mode == mode::dense)
            {
                while(entry_id_head != marker.entry_id_tail().load(std::memory_order_relaxed))
                    entry_id_head = marker.entry_id_head().load(std::memory_order_relaxed);

                cons_index_head = marker.index_head().load(std::memory_order_relaxed);
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
                        // LOGD("Underrun! %s", dump().data());
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

                if(mode == mode::dense)
                {
                    if(!marker.entry_id_head().compare_exchange_weak(entry_id_head, entry_id_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        continue;
                    }
                    else
                    {
                        marker.index_head().store(cons_next_index + next_size, std::memory_order_relaxed);
                        marker.entry_id_tail().store(entry_id_head + 1, std::memory_order_relaxed);
                        entry_id = entry_id_head;
                        break;
                    }
                }
                else
                {
                    if(!marker.index_head().compare_exchange_weak(cons_index_head, cons_next_index + next_size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                    else break;
                }
            }
            else
            {
                // LOGD("Underrun! %s", dump().data());
                next_size = 0;
                // profAdd(stat_pop_error, 1);
                break;
            }
        }
        size = next_size;
        // profAdd(stat_pop_total, 1);
        return cons_next_index;
    }

    template <Mode mode>
    bool completePop(size_t entry_id, size_t curr_index, size_t next_index, size_t size)
    {
        auto &marker = consumer_;
        if(mode == mode::dense)
        {
            // LOGD("%ld:%ld:%ld:%ld:%ld", entry_id, curr_index, next_index, marker.exit_id().load(std::memory_order_relaxed). num_threads_);
            if(entry_id >= (marker.exit_id().load(std::memory_order_relaxed) + num_threads_)) return false;

            size_t t_pos = wrap(entry_id, num_threads_);
            marker.size_list(wrap(entry_id, num_threads_)).store(size);
            // marker.curr_index_list(wrap(entry_id, num_threads_)).store(curr_index);
            marker.next_index_list(wrap(entry_id, num_threads_)).store(next_index);
            size_t const kIdx = entry_id;
            size_t next_exit_id = marker.exit_id_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
            // LOGD("%ld/%ld %ld/%ld %s", curr_index, next_index, next_exit_id, entry_id, dump().data());

            for(;;marker.comp_miss_count.fetch_add(1, std::memory_order_relaxed))
            {
                // size_t next_exit_id;
                size_t curr_exit_id = marker.exit_id().load(std::memory_order_relaxed);

                // LOGD(">(%ld/%ld)\t{%ld/%ld}", kIdx, entry_id, curr_exit_id, next_exit_id);
                if(entry_id < curr_exit_id)
                {
                    break;
                }
                else if (entry_id == curr_exit_id)
                {
                    next_exit_id = curr_exit_id + 1;
                    while((entry_id - curr_exit_id < num_threads_) && (next_exit_id > curr_exit_id))
                    {
                        size = marker.size_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
                        // curr_index = marker.curr_index_list(wrap(next_exit_id, num_threads_)).load(std::memory_order_relaxed);
                        next_index = marker.next_index_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);

                        marker.index_tail().store(next_index + size, std::memory_order_relaxed);
                        entry_id++;
                        next_exit_id = marker.exit_id_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
                        // LOGD(" -(%ld)\t%ld/%ld\t%ld/%ld/%ld\t%s", kIdx, 
                        //      next_exit_id, entry_id,
                        //      curr_index, next_index, size, 
                        //      dump().data());

                    }

                    marker.exit_id().store(entry_id, std::memory_order_relaxed);
                    // marker.index_tail().store(next_index + size, std::memory_order_relaxed);
                }

                // LOGD(" (%ld)\t%ld/%ld\t%s", 
                //          kIdx, 
                //          next_exit_id, entry_id, 
                //          dump().data());
                if(marker.exit_id_list(wrap(entry_id, num_threads_)).compare_exchange_strong(next_exit_id, entry_id, std::memory_order_relaxed, std::memory_order_relaxed)) break;

                // LOGD(" M(%ld/%ld) {%ld}", kIdx, entry_id, marker.exit_id().load(std::memory_order_relaxed));
            }
        }
        else
        {
            for(;marker.index_tail().load(std::memory_order_relaxed) != curr_index;){}
            marker.index_tail().store(next_index + size, std::memory_order_relaxed);
        }
        // LOGD(" <(%ld/%ld) {%ld}", kIdx, entry_id, marker.exit_id().load(std::memory_order_relaxed));

        return true;
    }

    template <Mode mode>
    size_t tryPush(size_t &entry_id, size_t &prod_index_head, size_t &size)
    {
        size_t prod_next_index;
        auto &marker = producer_;
        size_t entry_id_head = marker.entry_id_head().load(std::memory_order_relaxed);
        prod_index_head = marker.index_head().load(std::memory_order_relaxed);

        for(;;profAdd(marker.try_miss_count, 1))
        {
            if(mode == mode::dense)
            {
                while(entry_id_head != marker.entry_id_tail().load(std::memory_order_relaxed))
                    entry_id_head = marker.entry_id_head().load(std::memory_order_relaxed);

                prod_index_head = marker.index_head().load(std::memory_order_relaxed);
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

                if(mode == mode::dense)
                {
                    if(!marker.entry_id_head().compare_exchange_weak(entry_id_head, entry_id_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        continue;
                    }
                    else
                    {
                        marker.index_head().store(prod_next_index + size, std::memory_order_relaxed);
                        marker.entry_id_tail().store(entry_id_head + 1, std::memory_order_relaxed);
                        entry_id = entry_id_head;
                        break;
                    }
                }
                else
                {
                    if(!marker.index_head().compare_exchange_weak(prod_index_head, prod_next_index + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
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
        auto &marker = producer_;
        if(mode == mode::dense)
        {
            if(entry_id >= (marker.exit_id().load(std::memory_order_relaxed) + num_threads_)) return false;

            size_t t_pos = wrap(entry_id, num_threads_);
            marker.size_list(wrap(entry_id, num_threads_)).store(size);
            // marker.curr_index_list(wrap(exit_id, num_threads_)).store(curr_index);
            marker.next_index_list(wrap(entry_id, num_threads_)).store(next_index);
            size_t const kIdx = entry_id;
            size_t next_exit_id = marker.exit_id_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
            // LOGD("%ld/%ld %ld/%ld %s", curr_index, next_index, next_exit_id, entry_id, dump().data());

            for(;;marker.comp_miss_count.fetch_add(1, std::memory_order_relaxed))
            {
                // size_t next_exit_id;
                size_t curr_exit_id = marker.exit_id().load(std::memory_order_relaxed);

                // LOGD(">(%ld/%ld)\t{%ld/%ld}", kIdx, entry_id, curr_exit_id, next_exit_id);
                if(entry_id < curr_exit_id)
                {
                    break;
                }
                else if (entry_id == curr_exit_id)
                {
                    next_exit_id = curr_exit_id + 1;
                    while((entry_id - curr_exit_id < num_threads_) && (next_exit_id > curr_exit_id))
                    {
                        size = marker.size_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
                        curr_index = marker.index_tail().load(std::memory_order_relaxed);
                        next_index = marker.next_index_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);

                        if(next_index >= this->next(curr_index))
                        {
                            // marker.index_tail().store(curr_index, std::memory_order_relaxed);
                            water_mark_.store(curr_index, std::memory_order_relaxed);
                            // LOGD("%ld/%ld\t\t%s", curr_index, next_index, dump().data());
                        }
                        marker.index_tail().store(next_index + size, std::memory_order_relaxed);
                        entry_id++;
                        next_exit_id = marker.exit_id_list(wrap(entry_id, num_threads_)).load(std::memory_order_relaxed);
                        // LOGD(" -(%ld)\t%ld/%ld\t%ld/%ld/%ld\t%s", kIdx, 
                        //      next_exit_id, marker.exit_id().load(),
                        //      curr_index, next_index, size, 
                        //      dump().data());

                    }

                    // marker.index_tail().store(next_index + size, std::memory_order_relaxed);
                    // LOGD("store %ld", entry_id);
                    marker.exit_id().store(entry_id, std::memory_order_relaxed);
                }

                // LOGD(" (%ld)\t%ld/%ld\t%s", 
                //          kIdx, 
                //          next_exit_id, entry_id, 
                //          dump().data());
                if(marker.exit_id_list(wrap(entry_id, num_threads_)).compare_exchange_strong(next_exit_id, entry_id, std::memory_order_relaxed, std::memory_order_relaxed)) break;

                // LOGD(" M(%ld/%ld) {%ld}", kIdx, entry_id, marker.exit_id().load(std::memory_order_relaxed));
            }
        }
        else
        {
            for(;marker.index_tail().load(std::memory_order_relaxed) != curr_index;){}
            if(next_index >= this->next(curr_index))
            {
                water_mark_.store(curr_index, std::memory_order_relaxed);
            }
            marker.index_tail().store(next_index + size, std::memory_order_release);
        }
        // LOGD(" <(%ld/%ld) {%ld}", kIdx, exit_id, marker.exit_id().load(std::memory_order_relaxed));

        return true;
    }

    template <typename F, typename ...Args>
    size_t pop(F &&callback, size_t size, Args &&...args)
    {
        auto &marker = consumer_;
        size_t id, index;
        size_t time_cb{0}, time_try{0}, time_comp{0};
        profTimerReset();
        size_t next = tryPop<cons_mode>(id, index, size);
        time_try = profTimerElapse(marker.time_try);
        profTimerStart();
        if(size)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            callback(elemAt(next), size, std::forward<Args>(args)...);
            time_cb = profTimerElapse(marker.time_cb);
            profTimerStart();
            while(!completePop<cons_mode>(id, index, next, size)){};
            time_comp = profTimerElapse(marker.time_comp);
            profTimerAbsElapse(marker.time_total);
            profAdd(marker.total_size, size);
            updateMinMax(marker, time_cb, time_try, time_comp);
        }
        else
        {
            profTimerAbsElapse(marker.time_total);
            profAdd(marker.error_count, 1);
        }
        profAdd(marker.try_count, 1);
        return size;
    }

    template <typename F, typename ...Args>
    size_t push(const F &callback, size_t size, Args &&...args)
    {
        auto &marker = producer_;
        size_t id, index;
        size_t time_cb{0}, time_try{0}, time_comp{0};
        profTimerReset();
        size_t next = tryPush<prod_mode>(id, index, size);
        time_try = profTimerElapse(marker.time_try);
        profTimerStart();
        if(size)
        {
            callback(elemAt(next), size, std::forward<Args>(args)...);
            std::atomic_thread_fence(std::memory_order_release);
            time_cb = profTimerElapse(marker.time_cb);
            profTimerStart();
            while(!completePush<prod_mode>(id, index, next, size)){};
            time_comp = profTimerElapse(marker.time_comp);
            profTimerAbsElapse(marker.time_total);
            profAdd(marker.total_size, size);
            updateMinMax(marker, time_cb, time_try, time_comp);
        }
        else
        {
            profTimerAbsElapse(marker.time_total);
            profAdd(marker.error_count, 1);
        }
        profAdd(marker.try_count, 1);
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

            if(out_index[wrap(idx, num_threads_)].compare_exchange_weak(next_idx, idx, std::memory_order_relaxed, std::memory_order_relaxed)) break;

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
                profAdd(consumer_.error_count, 1);
                break;
            }

            if(consumer_.index_head().compare_exchange_weak(idx, idx + 1, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                ret = true;
                break;
            }
            profAdd(consumer_.try_miss_count, 1);
        }
        // profAdd(time_pop_try, timer.elapse().count());
        profTimerElapse(consumer_.time_try);
        profTimerStart();
        if(ret)
        {
            callback(elemAt(idx), std::forward<Args>(args)...);
            // profAdd(time_pop_cb, timer.elapse().count());
            profTimerElapse(consumer_.time_cb);
            profTimerStart();
            // while(!completeQueue(idx, cons_tail_, cons_out_)) {std::this_thread::yield();}
            while(!completeQueue(idx, 1)) {std::this_thread::yield();}
            // profAdd(time_pop_complete, timer.elapse().count());
            profTimerElapse(consumer_.time_comp);
            profAdd(consumer_.total_size, 1);
        }
        // profAdd(time_pop_total, timer.absElapse().count());
        profTimerAbsElapse(consumer_.time_total);
        profAdd(consumer_.try_count, 1);
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
                profAdd(producer_.error_count, 1);
                break;
            }

            if(producer_.index_head().compare_exchange_weak(idx, idx + 1, std::memory_order_acquire, std::memory_order_relaxed))
            {
                ret = true;
                break;
            }
            profAdd(producer_.try_miss_count, 1);
        }
        // profAdd(time_push_try, timer.elapse().count());
        profTimerElapse(producer_.time_try);
        profTimerStart();
        if(ret)
        {
            callback(elemAt(idx), std::forward<Args>(args)...);
            // profAdd(time_push_cb, timer.elapse().count());
            profTimerElapse(producer_.time_cb);
            profTimerStart();
            // while(!completeQueue(idx, prod_tail_, prod_out_)) {std::this_thread::yield();}
            while(!completeQueue(idx, 0)) {std::this_thread::yield();}
            // profAdd(time_push_complete, timer.elapse().count());
            profTimerElapse(producer_.time_comp);
            profAdd(producer_.total_size, 1);
        }
        // profAdd(time_push_total, timer.absElapse().count());
        profTimerAbsElapse(producer_.time_total);
        profAdd(producer_.try_count, 1);
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

    inline bool real_empty() const
    {
        return producer_.index_head().load(std::memory_order_relaxed) == consumer_.index_head().load(std::memory_order_relaxed);
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

    inline Statistics statistics() const
    {
        return Statistics {
            producer_.statistic(), consumer_.statistic()
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

    inline bool isProfilingEnabled() const
    {
        return profiling;
    }

private:

    void profTimerReset()
    {
        if(profiling)
            st::counter.reset();
    }

    void profTimerStart()
    {
        if(profiling)
            st::counter.start();
    }

    size_t profTimerElapse(std::atomic<size_t> &atomic)
    {
        if(profiling) 
        {
            size_t diff = st::counter.elapse().count();
            atomic.fetch_add(diff, std::memory_order_relaxed);
            return diff;
        }
        return 0;
    }

    size_t profTimerAbsElapse(std::atomic<size_t> &atomic)
    {
        if(profiling)
        {
            size_t diff = st::counter.absElapse().count();
            atomic.fetch_add(diff, std::memory_order_relaxed);
            return diff;
        }
        return 0;
    }

    template <typename U>
    size_t profAdd(std::atomic<size_t> &atomic, const U &val)
    {
        if(profiling)
            return atomic.fetch_add(val, std::memory_order_relaxed);
        return 0;
    }

    inline void updateMinMax(Marker &mk, size_t time_cb, size_t time_try, size_t time_comp)
    {
        if(profiling)
        {
            size_t time_min_cb = mk.time_min_cb.load(std::memory_order_relaxed);
            while(time_min_cb > time_cb) mk.time_min_cb.compare_exchange_weak(time_min_cb, time_cb, std::memory_order_relaxed, std::memory_order_relaxed);

            size_t time_max_cb = mk.time_max_cb.load(std::memory_order_relaxed);
            while(time_max_cb < time_cb) mk.time_max_cb.compare_exchange_weak(time_max_cb, time_cb, std::memory_order_relaxed, std::memory_order_relaxed);

            size_t time_min_try = mk.time_min_try.load(std::memory_order_relaxed);
            while(time_min_try > time_try) mk.time_min_try.compare_exchange_weak(time_min_try, time_try, std::memory_order_relaxed, std::memory_order_relaxed);

            size_t time_max_try = mk.time_max_try.load(std::memory_order_relaxed);
            while(time_max_try < time_try) mk.time_max_try.compare_exchange_weak(time_max_try, time_try, std::memory_order_relaxed, std::memory_order_relaxed);

            size_t time_min_comp = mk.time_min_comp.load(std::memory_order_relaxed);
            while(time_min_comp > time_comp) mk.time_min_comp.compare_exchange_weak(time_min_comp, time_comp, std::memory_order_relaxed, std::memory_order_relaxed);

            size_t time_max_comp = mk.time_max_comp.load(std::memory_order_relaxed);
            while(time_max_comp < time_comp) mk.time_max_comp.compare_exchange_weak(time_max_comp, time_comp, std::memory_order_relaxed, std::memory_order_relaxed);
        }
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

    // /// Statistic
    // std::atomic<size_t> stat_push_size{0};
    // std::atomic<size_t> stat_push_total{0};
    // std::atomic<size_t> stat_push_error{0};
    // std::atomic<size_t> stat_push_miss{0};
    // std::atomic<size_t> stat_push_exit_miss{0};
    // std::atomic<size_t> time_push_total{0};
    // std::atomic<size_t> time_push_cb{0};
    // std::atomic<size_t> time_push_try{0};
    // std::atomic<size_t> time_push_complete{0};

    // std::atomic<size_t> stat_pop_size{0};
    // std::atomic<size_t> stat_pop_total{0};
    // std::atomic<size_t> stat_pop_error{0};
    // std::atomic<size_t> stat_pop_miss{0};
    // std::atomic<size_t> stat_pop_exit_miss{0};
    // std::atomic<size_t> time_pop_total{0};
    // std::atomic<size_t> time_pop_cb{0};
    // std::atomic<size_t> time_pop_try{0};
    // std::atomic<size_t> time_pop_complete{0};

    std::vector<T> buffer_;
};

template <typename T, bool P=false>
using ring_fifo_dd = typename tll::lf2::ccfifo<T, mode::dense, mode::dense, P>;
template <typename T, bool P=false>
using ring_fifo_ds = typename tll::lf2::ccfifo<T, mode::dense, mode::sparse, P>;
template <typename T, bool P=false>
using ring_fifo_ss = typename tll::lf2::ccfifo<T, mode::sparse, mode::sparse, P>;
template <typename T, bool P=false>
using ring_fifo_sd = typename tll::lf2::ccfifo<T, mode::sparse, mode::dense, P>;



template <typename T, bool P=false>
using ring_fifo_mpmc = typename tll::lf2::ring_fifo_dd<T, P>;

template <typename T, bool P=false>
using ring_fifo_mpsc = typename tll::lf2::ring_fifo_ds<T, P>;

template <typename T, bool P=false>
using ring_fifo_spmc = typename tll::lf2::ring_fifo_sd<T, P>;

template <typename T, bool P=false>
using ring_fifo_spsc = typename tll::lf2::ring_fifo_ss<T, P>;

} /// tll::lf
