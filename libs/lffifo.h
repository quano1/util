#pragma once

/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#include <mutex>
#include <thread>
#include <vector>
#include <mutex>
#include <bitset>

#include "util.h"

typedef std::chrono::steady_clock StatClock;
typedef std::chrono::duration<size_t, std::nano> StatDuration; /// ns

namespace tll::lf {

template <typename T>
using Callback = std::function<void(const T *o, size_t s)>;


namespace internal {
static thread_local tll::util::Counter<2, StatDuration, StatClock> counter;
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

// template <bool E=false>
struct Marker
{
    Marker() = default;
    inline auto &refIndexHead() {return index_head_;}
    inline auto &refIndexTail() {return index_tail_;}
    inline auto &refEntryIdHead() {return entry_id_head_;}
    inline auto &refEntryIdTail() {return entry_id_tail_;}
    inline auto &refExitId() {return exit_id_;}

    inline size_t getIndexHead(std::memory_order mo=std::memory_order_relaxed) const {return index_head_.load(mo);}
    inline size_t getIndexTail(std::memory_order mo=std::memory_order_relaxed) const {return index_tail_.load(mo);}
    inline size_t getEntryIdHead(std::memory_order mo=std::memory_order_relaxed) const {return entry_id_head_.load(mo);}
    inline size_t getEntryIdTail(std::memory_order mo=std::memory_order_relaxed) const {return entry_id_tail_.load(mo);}
    inline size_t getExitId(std::memory_order mo=std::memory_order_relaxed) const {return exit_id_.load(mo);}

    // inline auto &exit_id_list(size_t i) {return exit_id_list_[i];}
    inline auto &refNextIndexList(size_t i) {return next_index_list_[i];}
    // inline auto &curr_index_list(size_t i) {return curr_index_list_[i];}
    // inline auto &size_list(size_t i) {return size_list_[i];}

    ~Marker()
    {
        // if(exit_id_list_) delete exit_id_list_;
        if(next_index_list_) delete next_index_list_;
        // if(curr_index_list_) delete curr_index_list_;
        // if(size_list_) delete size_list_;
    }

    template <bool kIsDense=true>
    void reset(size_t capacity, size_t list_size)
    {
        index_head_.store(0, std::memory_order_relaxed);
        index_tail_.store(0, std::memory_order_relaxed);
        entry_id_head_.store(0, std::memory_order_relaxed);
        entry_id_tail_.store(0, std::memory_order_relaxed);
        exit_id_.store(0, std::memory_order_relaxed);

        if constexpr (kIsDense)
        {
            if(this->list_size != list_size)
            {
                this->list_size = list_size;
                // if(exit_id_list_) delete exit_id_list_;
                if(next_index_list_) delete next_index_list_;
                // if(curr_index_list_) delete curr_index_list_;
                // if(size_list_) delete size_list_;
                // exit_id_list_ = new std::atomic<size_t> [list_size];
                next_index_list_ = new std::atomic<size_t> [list_size];
                // curr_index_list_ = new std::atomic<size_t> [list_size];
                // size_list_ = new std::atomic<size_t> [list_size];
            }

            for(int i=0; i<list_size; i++)
            {
                // exit_id_list_[i].store(0, std::memory_order_relaxed);
                next_index_list_[i].store(0, std::memory_order_relaxed);
            }
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

    // std::atomic<size_t> *exit_id_list_{nullptr};
    std::atomic<size_t> *next_index_list_{nullptr};
    // std::atomic<size_t> *size_list_{nullptr};
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
    kMultiSparse,
    kMultiDense,
    kMax
};

namespace mode
{
    constexpr Mode multi_sparse = Mode::kMultiSparse;
    constexpr Mode multi_dense = Mode::kMultiDense;
}

/// Contiguous Circular Index
// template <size_t num_threads=0x400>
template <typename T, Mode kProdMode=Mode::kMultiSparse, Mode kConsMode=Mode::kMultiSparse, bool kIsContiguous=true, bool kProfiling=false>
struct Fifo
{
public:
    using Elem_t = T;
private:
    Marker producer_;
    Marker consumer_;
    std::atomic<size_t> water_mark_{0}, water_mark_head_{0};
    size_t capacity_{0}, num_threads_{0};
    std::vector<Elem_t> buffer_;

    void profTimerReset()
    {
        if constexpr (kProfiling)
            internal::counter.reset(-1);
    }

    size_t profTimerElapse(std::atomic<size_t> &atomic)
    {
        if constexpr (kProfiling)
        {
            size_t diff = internal::counter.elapse(0).count();
            atomic.fetch_add(diff, std::memory_order_relaxed);
            return diff;
        }
        return 0;
    }

    size_t profTimerElapsed(std::atomic<size_t> &atomic)
    {
        if constexpr (kProfiling)
        {
            size_t diff = internal::counter.elapsed(0).count();
            atomic.fetch_add(diff, std::memory_order_relaxed);
            return diff;
        }
        return 0;
    }

    size_t profTimerTotal(std::atomic<size_t> &atomic)
    {
        if constexpr (kProfiling)
        {
            size_t diff = internal::counter.elapse(1).count();
            atomic.fetch_add(diff, std::memory_order_relaxed);
            return diff;
        }
        return 0;
    }

    template <typename U>
    size_t profAdd(std::atomic<size_t> &atomic, const U &val)
    {
        if constexpr (kProfiling)
            return atomic.fetch_add(val, std::memory_order_relaxed);
        return 0;
    }

    inline void updateMinMax(Marker &mk, size_t time_cb, size_t time_try, size_t time_comp)
    {
        if constexpr (kProfiling)
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
    template <bool kIsProducer, typename F, typename ...Args>
    size_t doFifo(const F &callback, size_t size, Args &&...args)
    {
        auto &marker = kIsProducer ? producer_ : consumer_;
        size_t id, index;
        size_t time_cb{0}, time_try{0}, time_comp{0};
        profTimerReset();
        size_t next = kIsProducer ? tryPush(id, index, size) : tryPop(id, index, size);
        time_try = profTimerElapsed(marker.time_try);
        if(size)
        {
            if constexpr (!kIsProducer)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
            }

            if constexpr (kIsContiguous)
            {
                callback(elemAt(next), size, std::forward<Args>(args)...);
            }
            else
            {
                for(size_t i=0; i<size; i++)
                    callback(elemAt(next+i), size - i - 1, std::forward<Args>(args)...);
            }

            if constexpr (kIsProducer)
            {
                std::atomic_thread_fence(std::memory_order_release);
                // LOGD("%ld", size);
            }

            time_cb = profTimerElapsed(marker.time_cb);

            complete<kIsProducer>(id, index, next, size);

            time_comp = profTimerElapse(marker.time_comp);
            profTimerTotal(marker.time_total);
            profAdd(marker.total_size, size);
            updateMinMax(marker, time_cb, time_try, time_comp);
        }
        else
        {
            profTimerTotal(marker.time_total);
            profAdd(marker.error_count, 1);
        }
        profAdd(marker.try_count, 1);
        return size;
    } /// doFifo

    size_t tryPop(size_t &entry_id, size_t &cons_index_head, size_t &size)
    {
        auto &marker = consumer_;
        size_t cons_next_index;
        size_t entry_id_head;
        cons_index_head = marker.getIndexHead();
        size_t next_size = size;
        for(;;profAdd(marker.try_miss_count, 1))
        {
            next_size = size;
            size_t prod_index = producer_.getIndexTail();
            if constexpr (kIsContiguous && kConsMode == mode::multi_dense)
            {
                entry_id_head = marker.getEntryIdHead();
                while(entry_id_head != marker.getEntryIdTail())
                    entry_id_head = marker.getEntryIdHead();

                cons_index_head = marker.getIndexHead();
            }

            cons_next_index = cons_index_head;
            size_t wmark = water_mark_.load(std::memory_order_relaxed);
            if(cons_next_index < prod_index)
            {
                if constexpr (kIsContiguous)
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
                        {
                            // LOGD("%ld:%ld:%ld", next_size, prod_index, cons_next_index);
                            next_size = prod_index - cons_next_index;
                        }
                    }
                    else if(cons_next_index < wmark)
                    {
                        if(size > (wmark - cons_next_index))
                        {
                            // LOGD("%ld:%ld:%ld", next_size, wmark, cons_next_index);
                            next_size = wmark - cons_next_index;
                        }
                    }
                    else /// cons > wmark
                    {
                        if(size > (prod_index - cons_next_index))
                        {
                            // LOGD("%ld:%ld:%ld", next_size, prod_index, cons_next_index);
                            next_size = prod_index - cons_next_index;
                        }
                    }
                } /// if(kIsContiguous)
                else
                {
                    if(size > prod_index - cons_index_head)
                    {
                        next_size = prod_index - cons_index_head;
                    }
                }

                if constexpr (kIsContiguous && kConsMode == mode::multi_dense)
                {
                    if(!marker.refEntryIdHead().compare_exchange_strong(entry_id_head, entry_id_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        continue;
                    }
                    else
                    {
                        marker.refIndexHead().store(cons_next_index + next_size, std::memory_order_relaxed);
                        marker.refEntryIdTail().store(entry_id_head + 1, std::memory_order_relaxed);
                        entry_id = entry_id_head;
                        break;
                    }
                }
                else
                {
                    if(!marker.refIndexHead().compare_exchange_strong(cons_index_head, cons_next_index + next_size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                    else
                    {
                        entry_id = cons_index_head;
                        break;
                    }
                }
            } /// if(cons_next_index < prod_index)
            else
            {
                // LOGD("Underrun! %s", dump().data());
                next_size = 0;
                // profAdd(stat_pop_error, 1);
                break;
            }
        } /// for(;;)

        size = next_size;
        return cons_next_index;
    } /// tryPop

    size_t tryPush(size_t &entry_id, size_t &prod_index_head, size_t &size)
    {
        size_t prod_next_index;
        auto &marker = producer_;
        size_t entry_id_head;
        prod_index_head = marker.getIndexHead();

        for(;;profAdd(marker.try_miss_count, 1))
        {
            size_t cons_index = consumer_.getIndexTail();
            if constexpr (kIsContiguous && kProdMode == mode::multi_dense)
            {
                entry_id_head = marker.getEntryIdHead();
                while(entry_id_head != marker.getEntryIdTail())
                    entry_id_head = marker.getEntryIdHead();

                prod_index_head = marker.getIndexHead();
            }

            prod_next_index = prod_index_head;

            if(prod_next_index + size <= capacity() + cons_index)
            {
                if constexpr (kIsContiguous)
                {
                    /// prod leads
                    if(wrap(prod_next_index) >= wrap(cons_index))
                    {
                        /// not enough space ?
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
                } /// if(kIsContiguous)

                if constexpr (kIsContiguous && kProdMode == mode::multi_dense)
                {
                    if(!marker.refEntryIdHead().compare_exchange_strong(entry_id_head, entry_id_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        continue;
                    }
                    else
                    {
                        marker.refIndexHead().store(prod_next_index + size, std::memory_order_relaxed);
                        marker.refEntryIdTail().store(entry_id_head + 1, std::memory_order_relaxed);
                        entry_id = entry_id_head;
                        break;
                    }
                } /// kProdMode == mode::multi_dense
                else
                {
                    if(!marker.refIndexHead().compare_exchange_strong(prod_index_head, prod_next_index + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                    else
                    {
                        entry_id = prod_index_head;
                        break;
                    }
                }
            }
            else
            {
                // LOGE("OVERRUN");dump();
                size = 0;
                break;
            }
        } /// for(;;)
        return prod_next_index;
    } /// tryPush

    template <bool kIsProducer>
    void complete(size_t exit_id, size_t curr_index, size_t next_index, size_t size)
    {
        auto &marker = kIsProducer ? producer_ : consumer_;
        constexpr Mode kMode = kIsProducer ? kProdMode : kConsMode;
        if constexpr (kMode == mode::multi_dense)
        {
            while(exit_id >= (marker.getExitId() + num_threads_)){};
            size_t const kIdx = exit_id;

            if constexpr (kIsContiguous && kIsProducer)
                if(next_index >= this->next(curr_index))
                    water_mark_head_.store(curr_index, std::memory_order_relaxed);

            next_index += size;
            size_t old_next_index = marker.refNextIndexList(wrap(exit_id, num_threads_)).load(std::memory_order_relaxed);

            size_t new_exit_id = exit_id + (kIsContiguous? 1 : size);
            for(;;marker.comp_miss_count.fetch_add(1, std::memory_order_relaxed))
            {
                size_t curr_exit_id = marker.getExitId();

                if(exit_id == curr_exit_id)
                {
                    while(true)
                    {
                        old_next_index = marker.refNextIndexList(wrap(new_exit_id, num_threads_)).load(std::memory_order_relaxed);

                        if(old_next_index > next_index)
                        {
                            if constexpr (kIsContiguous)
                                new_exit_id++;
                            else
                                new_exit_id = old_next_index;
                            next_index = old_next_index;
                        }
                        else
                            break;
                    }

                    if constexpr (kIsContiguous && kIsProducer)
                    {
                        size_t wmh = water_mark_head_.load(std::memory_order_relaxed);
                        if((water_mark_.load(std::memory_order_relaxed) < wmh)
                           && (next_index > wmh))
                        {
                            marker.refIndexTail().store(wmh, std::memory_order_relaxed);
                            water_mark_.store(wmh, std::memory_order_relaxed);
                        }
                    }

                    marker.refIndexTail().store(next_index, std::memory_order_relaxed);
                    exit_id = new_exit_id;
                    marker.refExitId().store(exit_id, std::memory_order_relaxed);
                }
                else if(exit_id < curr_exit_id)
                { break; }

                if(marker.refNextIndexList(wrap(exit_id, num_threads_)).compare_exchange_strong(old_next_index, next_index, std::memory_order_relaxed, std::memory_order_relaxed)) break;
            }
        } /// if(mode == mode::multi_dense)
        else
        {
            while(marker.getIndexTail() != curr_index){}
            if constexpr (kIsContiguous && kIsProducer)
                if(next_index >= this->next(curr_index))
                    water_mark_.store(curr_index, std::memory_order_relaxed);
            marker.refIndexTail().store(next_index + size, std::memory_order_release);
        }
    }

public:
    Fifo() = default;
    ~Fifo() = default;

    Fifo(size_t size, size_t num_threads=0x1000)
    {
        reserve(size, num_threads);
    }

    // template <uint8_t type = 3>
    inline void dumpStat(double real_total_time, int type=3, bool title=true) const
    {
        if constexpr (!kProfiling) return;
        using namespace std::chrono;
        if(title) printf("        count(K) |err(%%) | miss(%%)tr:co|try(%%) |comp(%%)| cb(%%) | all(%%)| Mbs    | cb try comp (min:max:avg) (nano sec)\n");
        auto stats = statistics();
        /// producer
        if(type & 1)
        {
            auto &st = stats.first;
            // double time_total = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_total)).count();
            size_t comp_count = st.try_count - st.error_count;
            size_t time_avg_cb = st.time_cb / comp_count;
            size_t time_avg_try = st.time_try / st.try_count;
            size_t time_avg_comp = st.time_comp / comp_count;

            size_t time_real = st.time_cb + st.time_try + st.time_comp;
            double time_try_rate = st.time_try*100.f/ time_real;
            double time_complete_rate = st.time_comp*100.f/ time_real;
            double time_callback_rate = st.time_cb*100.f/ time_real;
            double time_real_rate = (time_real)*100.f/ st.time_total;

            double try_count = st.try_count * 1.f / 1000;
            double error_rate = (st.error_count*100.f)/st.try_count;
            double try_miss_rate = (st.try_miss_count*100.f)/st.try_count;
            double comp_miss_rate = (st.comp_miss_count*100.f)/st.try_count;

            // double opss = st.total_size * .001f / real_total_time;
            double speed = (st.total_size * 1.f / 0x100000) / real_total_time;


            printf(" push: %9.3f | %5.2f | %5.2f:%5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.3f | (%ld:%ld:%ld) (%ld:%ld:%ld) (%ld:%ld:%ld)\n",
                   try_count, error_rate, try_miss_rate, comp_miss_rate
                   , time_try_rate, time_complete_rate, time_callback_rate, time_real_rate
                   , speed,
                   st.time_min_cb, st.time_max_cb, time_avg_cb,
                   st.time_min_try, st.time_max_try, time_avg_try,
                   st.time_min_comp, st.time_max_comp, time_avg_comp
                   );
        }

        if(type & 2)
        {
            auto &st = stats.second;
            // double time_total = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_total)).count();
            size_t comp_count = st.try_count - st.error_count;
            size_t time_avg_cb = st.time_cb / comp_count;
            size_t time_avg_try = st.time_try / st.try_count;
            size_t time_avg_comp = st.time_comp / comp_count;

            size_t time_real = st.time_cb + st.time_try + st.time_comp;
            double time_try_rate = st.time_try*100.f/ time_real;
            double time_complete_rate = st.time_comp*100.f/ time_real;
            double time_callback_rate = st.time_cb*100.f/ time_real;
            double time_real_rate = (time_real)*100.f/ st.time_total;

            double try_count = st.try_count * 1.f / 1000;
            double error_rate = (st.error_count*100.f)/st.try_count;
            double try_miss_rate = (st.try_miss_count*100.f)/st.try_count;
            double comp_miss_rate = (st.comp_miss_count*100.f)/st.try_count;

            // double opss = st.total_size * .001f / real_total_time;
            double speed = (st.total_size * 1.f / 0x100000) / real_total_time;
            // LOGV(st.total_size, real_total_time, st.total_size * 1.f / 0x100000);
            printf(" pop : %9.3f | %5.2f | %5.2f:%5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.3f | (%ld:%ld:%ld) (%ld:%ld:%ld) (%ld:%ld:%ld)\n",
                   try_count, error_rate, try_miss_rate, comp_miss_rate
                   , time_try_rate, time_complete_rate, time_callback_rate, time_real_rate
                   , speed,
                   st.time_min_cb, st.time_max_cb, time_avg_cb,
                   st.time_min_try, st.time_max_try, time_avg_try,
                   st.time_min_comp, st.time_max_comp, time_avg_comp
                   );
        }
    }

    inline auto dump() const
    {
        size_t ph = producer_.getIndexHead();
        size_t pt = producer_.getIndexTail();
        size_t peh = producer_.getEntryIdHead();
        size_t pet = producer_.getEntryIdTail();
        size_t pex = producer_.getExitId();

        size_t ch = consumer_.getIndexHead();
        size_t ct = consumer_.getIndexTail();
        size_t ceh = consumer_.getEntryIdHead();
        size_t cet = consumer_.getEntryIdTail();
        size_t cex = consumer_.getExitId();

        size_t wm = water_mark_.load(std::memory_order_relaxed);
        size_t wmh = water_mark_head_.load(std::memory_order_relaxed);

        return util::stringFormat("(p/(%ld:%ld) (%ld:%ld)) "
                                  "(c/(%ld:%ld) (%ld:%ld)) "
                                  "(wm/%ld:%ld:%ld sz/%ld) "
                                  "(pe/%ld:%ld:%ld) "
                                  "(ce/%ld:%ld:%ld)",
            wrap(ph), wrap(pt), ph, pt,
            wrap(ch), wrap(ct), ch, ct,
            wrap(wm), wm, wmh, capacity_,
            peh, pet, pex,
            ceh, cet, cex);
    }

    inline void reset()
    {
        water_mark_.store(0,std::memory_order_relaxed);
        water_mark_head_.store(0,std::memory_order_relaxed);
        producer_.reset<kProdMode == mode::multi_dense>(capacity_, num_threads_);
        consumer_.reset<kConsMode == mode::multi_dense>(capacity_, num_threads_);
    }

    inline void reserve(size_t size)
    {
        reserve(size, num_threads_);
    }

    inline void reserve(size_t size, size_t num_threads)
    {
        capacity_ = util::isPowerOf2(size) ? size : util::nextPowerOf2(size);
        if constexpr (kProdMode == mode::multi_dense || kConsMode == mode::multi_dense)
        {
            if(num_threads > capacity_)
                num_threads_ = capacity_;
            else
                num_threads_ = tll::util::isPowerOf2(num_threads) ? num_threads : tll::util::nextPowerOf2(num_threads);
        }
#if (!defined NO_ALLOCATE)
        buffer_.resize(capacity_);
#endif
        reset();
    }

    inline size_t push(const Elem_t *ptr, size_t sz)
    {
        static auto cb = [](Elem_t *el, size_t sz, const Elem_t *ptr){ std::memcpy(el, ptr, sz); };
        return pushCb(cb, sz, ptr);
    }

    inline size_t pop(Elem_t *ptr, size_t sz)
    {
        static auto cb = [](Elem_t *el, size_t sz, Elem_t *ptr){ std::memcpy(ptr, el, sz); };
        return popCb(cb, sz, ptr);
    }

    template<typename U>
    inline size_t push(U &&val)
    {
        static auto cb = [](Elem_t *el, size_t, U &&val){ *el = std::forward<U>(val); };
        return pushCb(cb, 1, std::forward<U>(val));
    }

    inline size_t pop(Elem_t &val)
    {
        static auto cb = [](Elem_t *el, size_t, Elem_t &val){ val = std::move(*el); };
        return popCb(cb, 1, val);
    }


    template <typename F, typename ...Args>
    size_t pushCb(const F &callback, size_t size, Args &&...args)
    {
        return doFifo<true>(callback, size, std::forward<Args>(args)...);
    }

    template <typename F, typename ...Args>
    size_t popCb(const F &callback, size_t size, Args &&...args)
    {
        return doFifo<false>(callback, size, std::forward<Args>(args)...);
    }

/// utility functions
public:
    inline size_t capacity() const
    {
        return capacity_;
    }

    inline size_t size() const
    {
        return producer_.getIndexTail() - consumer_.getIndexHead();
    }

    inline bool empty() const
    {
        return producer_.getIndexTail() == consumer_.getIndexHead();
    }

    inline bool realEmpty() const
    {
        return producer_.getIndexHead() == consumer_.getIndexHead();
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

    inline std::pair<Statistic, Statistic> statistics() const
    {
        return {
            producer_.statistic(), consumer_.statistic()
        };
    }

    inline size_t ph() const {return producer_.getIndexHead();}
    inline size_t pt() const {return producer_.getIndexTail();}
    inline size_t ch() const {return consumer_.getIndexHead();}
    inline size_t ct() const {return consumer_.getIndexTail();}
    inline size_t wm() const {return water_mark_.load(std::memory_order_relaxed);}

    inline Elem_t *elemAt(size_t id)
    {
        return &buffer_[wrap(id)];
    }

    inline Elem_t &operator[](size_t id)
    {
        return buffer_[wrap(id)];
    }

    inline const Elem_t &operator[](size_t id) const
    {
        return buffer_[wrap(id)];
    }

    inline constexpr bool isProfilingEnabled() const
    {
        return kProfiling;
    }
}; /// Fifo

template <typename T, bool P=false>
using RingBufferDD = typename tll::lf::Fifo<T, mode::multi_dense, mode::multi_dense, true, P>;
template <typename T, bool P=false>
using RingBufferDS = typename tll::lf::Fifo<T, mode::multi_dense, mode::multi_sparse, true, P>;
template <typename T, bool P=false>
using RingBufferSS = typename tll::lf::Fifo<T, mode::multi_sparse, mode::multi_sparse, true, P>;
template <typename T, bool P=false>
using RingBufferSD = typename tll::lf::Fifo<T, mode::multi_sparse, mode::multi_dense, true, P>;

// template <typename T, bool P=false>
// using ring_buffer_mpmc = typename tll::lf::RingBufferDD<T, P>;
// template <typename T, bool P=false>
// using ring_buffer_mpsc = typename tll::lf::RingBufferDS<T, P>;
// template <typename T, bool P=false>
// using ring_buffer_spmc = typename tll::lf::RingBufferSD<T, P>;
// template <typename T, bool P=false>
// using ring_buffer_spsc = typename tll::lf::RingBufferSS<T, P>;


template <typename T, bool P=false>
using RingQueueDD = typename tll::lf::Fifo<T, mode::multi_dense, mode::multi_dense, false, P>;
template <typename T, bool P=false>
using RingQueueDS = typename tll::lf::Fifo<T, mode::multi_dense, mode::multi_sparse, false, P>;
template <typename T, bool P=false>
using RingQueueSS = typename tll::lf::Fifo<T, mode::multi_sparse, mode::multi_sparse, false, P>;
template <typename T, bool P=false>
using RingQueueSD = typename tll::lf::Fifo<T, mode::multi_sparse, mode::multi_dense, false, P>;

// template <typename T, bool P=false>
// using ring_queue_mpmc = typename tll::lf::RingQueueDD<T, P>;
// template <typename T, bool P=false>
// using ring_queue_mpsc = typename tll::lf::RingQueueDS<T, P>;
// template <typename T, bool P=false>
// using ring_queue_spmc = typename tll::lf::RingQueueSD<T, P>;
// template <typename T, bool P=false>
// using ring_queue_spsc = typename tll::lf::RingQueueSS<T, P>;

} /// tll::lf
