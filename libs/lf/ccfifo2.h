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
    inline auto &ref_index_head() {return index_head_;}
    inline auto &ref_index_tail() {return index_tail_;}
    inline auto &ref_entry_id_head() {return entry_id_head_;}
    inline auto &ref_entry_id_tail() {return entry_id_tail_;}
    inline auto &ref_exit_id() {return exit_id_;}

    inline size_t get_index_head(std::memory_order mo=std::memory_order_relaxed) const {return index_head_.load(mo);}
    inline size_t get_index_tail(std::memory_order mo=std::memory_order_relaxed) const {return index_tail_.load(mo);}
    inline size_t get_entry_id_head(std::memory_order mo=std::memory_order_relaxed) const {return entry_id_head_.load(mo);}
    inline size_t get_entry_id_tail(std::memory_order mo=std::memory_order_relaxed) const {return entry_id_tail_.load(mo);}
    inline size_t get_exit_id(std::memory_order mo=std::memory_order_relaxed) const {return exit_id_.load(mo);}

    // inline auto &exit_id_list(size_t i) {return exit_id_list_[i];}
    inline auto &ref_next_index_list(size_t i) {return next_index_list_[i];}
    // inline auto &curr_index_list(size_t i) {return curr_index_list_[i];}
    // inline auto &size_list(size_t i) {return size_list_[i];}

    ~Marker()
    {
        // if(exit_id_list_) delete exit_id_list_;
        if(next_index_list_) delete next_index_list_;
        // if(curr_index_list_) delete curr_index_list_;
        // if(size_list_) delete size_list_;
    }

    inline void reset(size_t capacity, size_t list_size)
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

        index_head_.store(0, std::memory_order_relaxed);
        index_tail_.store(0, std::memory_order_relaxed);
        entry_id_head_.store(0, std::memory_order_relaxed);
        entry_id_tail_.store(0, std::memory_order_relaxed);
        exit_id_.store(0, std::memory_order_relaxed);

        for(int i=0; i<list_size; i++)
        {
            // exit_id_list_[i].store(0, std::memory_order_relaxed);
            next_index_list_[i].store(0, std::memory_order_relaxed);
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
    kSparse,
    kDense,
    kMax
};

namespace mode
{
    constexpr Mode sparse = Mode::kSparse;
    constexpr Mode dense = Mode::kDense;
}

enum class Type
{
    kBuffer,
    kQueue,
    kMax
};

namespace type
{
    constexpr Type buffer = Type::kBuffer;
    constexpr Type queue = Type::kQueue;
}

/// Contiguous Circular Index
// template <size_t num_threads=0x400>
template <typename T, Type fifo_type=Type::kBuffer, Mode prod_mode=Mode::kDense, Mode cons_mode=Mode::kSparse, bool profiling=false>
struct ccfifo
{
private:
    Marker producer_;
    Marker consumer_;
    std::atomic<size_t> water_mark_{0}, water_mark_head_{0};
    size_t capacity_{0}, num_threads_{0};
    std::vector<T> buffer_;

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
    template <bool is_producer, typename F, typename ...Args>
    size_t doFifing(F &&callback, size_t size, Args &&...args)
    {
        auto &marker = is_producer ? producer_ : consumer_;
        size_t id, index;
        size_t time_cb{0}, time_try{0}, time_comp{0};
        profTimerReset();
        size_t next = is_producer ? tryPush(id, index, size) : tryPop(id, index, size);
        time_try = profTimerElapse(marker.time_try);
        profTimerStart();
        if(size)
        {
            if(is_producer)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
            }

            if(fifo_type == type::buffer)
            {
                callback(elemAt(next), size, std::forward<Args>(args)...);
            }
            else
            {
                for(size_t i=0; i<size; i++)
                    callback(elemAt(next+i), size, std::forward<Args>(args)...);
            }

            if(!is_producer)
            {
                std::atomic_thread_fence(std::memory_order_release);
                // LOGD("%ld", size);
            }

            time_cb = profTimerElapse(marker.time_cb);
            profTimerStart();
            
            complete<is_producer>(id, index, next, size);
            
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
    } /// doFifing

    size_t tryPop(size_t &entry_id, size_t &cons_index_head, size_t &size)
    {
        auto &marker = consumer_;
        size_t cons_next_index;
        size_t entry_id_head = marker.get_entry_id_head();
        cons_index_head = marker.get_index_head();
        size_t next_size = size;
        for(;;profAdd(marker.try_miss_count, 1))
        {
            if(fifo_type == type::buffer)
            {
                if(cons_mode == mode::dense)
                {
                    while(entry_id_head != marker.get_entry_id_tail())
                        entry_id_head = marker.get_entry_id_head();

                    cons_index_head = marker.get_index_head();
                }

                cons_next_index = cons_index_head;
                size_t prod_index = producer_.get_index_tail();
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

                    if(cons_mode == mode::dense)
                    {
                        if(!marker.ref_entry_id_head().compare_exchange_weak(entry_id_head, entry_id_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                        {
                            continue;
                        }
                        else
                        {
                            marker.ref_index_head().store(cons_next_index + next_size, std::memory_order_relaxed);
                            marker.ref_entry_id_tail().store(entry_id_head + 1, std::memory_order_relaxed);
                            entry_id = entry_id_head;
                            break;
                        }
                    }
                    else
                    {
                        if(!marker.ref_index_head().compare_exchange_weak(cons_index_head, cons_next_index + next_size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                        else break;
                    }
                } /// if(cons_next_index < prod_index)
                else
                {
                    // LOGD("Underrun! %s", dump().data());
                    next_size = 0;
                    // profAdd(stat_pop_error, 1);
                    break;
                }
            } /// if(fifo_type == type::buffer)
            else /// fifo_type == type::queue
            {
                for(;;)
                {
                    size_t prod_index = producer_.get_index_tail();
                    if (cons_index_head == marker.get_index_tail())
                    {
                        next_size = 0;
                        break;
                    }

                    if(size > prod_index - cons_index_head)
                    {
                        next_size = prod_index - cons_index_head;
                    }

                    if(marker.ref_index_head().compare_exchange_weak(cons_index_head, cons_index_head + next_size, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        entry_id = cons_index_head;
                        cons_next_index = cons_index_head;
                        break;
                    }
                }
            }
        } /// /// for(;;)

        size = next_size;
        return cons_next_index;
    } /// tryPop

    size_t tryPush(size_t &entry_id, size_t &prod_index_head, size_t &size)
    {
        size_t prod_next_index;
        auto &marker = producer_;
        size_t entry_id_head = marker.get_entry_id_head();
        prod_index_head = marker.get_index_head();

        for(;;profAdd(marker.try_miss_count, 1))
        {
            if(fifo_type == type::buffer)
            {
                if(prod_mode == mode::dense)
                {
                    while(entry_id_head != marker.get_entry_id_tail())
                        entry_id_head = marker.get_entry_id_head();

                    prod_index_head = marker.get_index_head();
                }

                prod_next_index = prod_index_head;
                size_t cons_index = consumer_.get_index_tail();
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

                    if(prod_mode == mode::dense)
                    {
                        if(!marker.ref_entry_id_head().compare_exchange_weak(entry_id_head, entry_id_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                        {
                            continue;
                        }
                        else
                        {
                            marker.ref_index_head().store(prod_next_index + size, std::memory_order_relaxed);
                            marker.ref_entry_id_tail().store(entry_id_head + 1, std::memory_order_relaxed);
                            entry_id = entry_id_head;
                            break;
                        }
                    }
                    else
                    {
                        if(!marker.ref_index_head().compare_exchange_weak(prod_index_head, prod_next_index + size, std::memory_order_relaxed, std::memory_order_relaxed)) continue;
                        else break;
                    }
                }
                else
                {
                    // LOGE("OVERRUN");dump();
                    size = 0;
                    break;
                }
            } /// if(fifo_type == type::buffer)
            else /// fifo_type == type::queue
            {
                for(;;)
                {
                    size_t cons_index = consumer_.get_index_tail();

                    if(prod_index_head + size > cons_index + capacity())
                    {
                        size = 0;
                        break;
                    }

                    if(marker.ref_index_head().compare_exchange_weak(prod_index_head, prod_index_head + size, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        entry_id = prod_index_head;
                        prod_next_index = prod_index_head;
                        break;
                    }
                }
            }
        } /// for(;;)
        return prod_next_index;
    } /// tryPush

    template <bool is_producer>
    void complete(size_t exit_id, size_t curr_index, size_t next_index, size_t size)
    {
        auto &marker = is_producer ? producer_ : consumer_;
        constexpr Mode mode = is_producer ? prod_mode : cons_mode;
        if(mode == mode::dense)
        {
            while(exit_id >= (marker.get_exit_id() + num_threads_)){};
            size_t const kIdx = exit_id;

            // size_t old_exit_id = marker.exit_id_list(wrap(kIdx, num_threads_)).load(std::memory_order_relaxed);
            // size_t new_exit_id=exit_id + 1;

            // LOGD("%d\t%ld:%ld", is_producer, next_index, this->next(curr_index));
            if(is_producer && next_index >= this->next(curr_index))
            {
                water_mark_head_.store(curr_index, std::memory_order_relaxed);
            }

            next_index+=size;
            // LOGD("%ld/%ld %ld/%ld %s", curr_index, next_index, old_exit_id, exit_id, dump().data());
            size_t old_next_index = marker.ref_next_index_list(wrap(exit_id, num_threads_)).load(std::memory_order_relaxed);
            // marker.ref_next_index_list(wrap(exit_id, num_threads_)).store(next_index);

            size_t new_exit_id=exit_id + 1;
            for(;;marker.comp_miss_count.fetch_add(1, std::memory_order_relaxed))
            {
                size_t curr_exit_id = marker.get_exit_id();

                // LOGD(">%ld:%ld\t%ld:%ld", kIdx, curr_exit_id, exit_id, old_exit_id);
                if(exit_id < curr_exit_id)
                { break; }

                // new_exit_id = exit_id + 1;
                if(exit_id > curr_exit_id)
                {
                    // LOGD(" !%ld:%ld\t%ld:%ld:%ld\t%s", kIdx, curr_exit_id, exit_id, old_exit_id, new_exit_id, dump().data());
                    // next_index = marker.ref_next_index_list(wrap(new_exit_id - 1, num_threads_)).load(std::memory_order_relaxed);
                    // marker.ref_next_index_list(wrap(exit_id, num_threads_)).store(next_index);
                }
                else
                {
                    while(true)
                    {
                        old_next_index = marker.ref_next_index_list(wrap(new_exit_id, num_threads_)).load(std::memory_order_relaxed);

                        if(old_next_index > next_index)
                        {
                            new_exit_id++;
                            next_index = old_next_index;
                            // new_exit_id = old_exit_id;
                        }
                        else
                            break;
                    }

                    // next_index = marker.ref_next_index_list(wrap(new_exit_id - 1, num_threads_)).load(std::memory_order_relaxed);

                    if(is_producer)
                    {
                        size_t wmh = water_mark_head_.load(std::memory_order_relaxed);
                        if((water_mark_.load(std::memory_order_relaxed) < wmh) 
                           && (next_index > wmh))
                        {
                            marker.ref_index_tail().store(wmh, std::memory_order_relaxed);
                            water_mark_.store(wmh, std::memory_order_relaxed);
                        }
                    }

                    marker.ref_index_tail().store(next_index, std::memory_order_relaxed);
                    exit_id = new_exit_id;
                    marker.ref_exit_id().store(exit_id, std::memory_order_relaxed);
                    // LOGD(" =%ld:%ld\t%ld:%ld:%ld\t%s", kIdx, curr_exit_id, exit_id, old_exit_id, new_exit_id, dump().data());
                }

                if(marker.ref_next_index_list(wrap(exit_id, num_threads_)).compare_exchange_strong(old_next_index, next_index, std::memory_order_relaxed, std::memory_order_relaxed)) break;
                // if(marker.exit_id_list(wrap(exit_id, num_threads_)).compare_exchange_strong(old_exit_id, new_exit_id, std::memory_order_relaxed, std::memory_order_relaxed)) break;
                // LOGD(" M%ld:%ld:%ld\t%ld:%ld\t%s", kIdx, curr_exit_id, exit_id, old_next_index, next_index, dump().data());
            }
        }
        else
        {

            for(;marker.get_index_tail() != curr_index;){}
            if(is_producer && next_index >= this->next(curr_index))
            {
                water_mark_.store(curr_index, std::memory_order_relaxed);
            }
            marker.ref_index_tail().store(next_index + size, std::memory_order_release);
        }
        // LOGD(" <(%ld/%ld) {%ld}", kIdx, entry_id, marker.get_exit_id());

        // return true;
    }

public:
    ccfifo() = default;
    ~ccfifo() = default;

    ccfifo(size_t size, size_t num_threads=0x400)
    {
        reserve(size, num_threads);
    }

    // template <uint8_t type = 3>
    inline void dumpStat(double real_total_time, int type=3) const
    {
        if(!profiling) return;
        using namespace std::chrono;
        if(type) printf("        count(K) |err(%%) | miss(%%) t:c |try(%%) |comp(%%)| cb(%%) | all(%%)| Mbs   | cb try comp (min:max:avg) (nano sec)\n");
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
            double speed = st.total_size * 1.f / 0x100000 / real_total_time;


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
            double speed = st.total_size * 1.f / 0x100000 / real_total_time;

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
        size_t ph = producer_.get_index_head();
        size_t pt = producer_.get_index_tail();
        size_t peh = producer_.get_entry_id_head();
        size_t pet = producer_.get_entry_id_tail();
        size_t pex = producer_.get_exit_id();

        size_t ch = consumer_.get_index_head();
        size_t ct = consumer_.get_index_tail();
        size_t ceh = consumer_.get_entry_id_head();
        size_t cet = consumer_.get_entry_id_tail();
        size_t cex = consumer_.get_exit_id();

        size_t wm = water_mark_.load(std::memory_order_relaxed);
        size_t wmh = water_mark_head_.load(std::memory_order_relaxed);

        return util::stringFormat("(p/(%ld:%ld) (%ld:%ld)) "
                                  "(c/(%ld:%ld) (%ld:%ld)) "
                                  "(wm/%ld:%ld:%ld sz/%ld) "
                                  "(pe/%ld:%ld:%ld) "
                                  "(ce/%ld:%ld:%ld) ",
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

    template <typename F, typename ...Args>
    size_t push(const F &callback, size_t size, Args &&...args)
    {
        return doFifing<true>(callback, size, std::forward<Args>(args)...);
    }

    template<typename U>
    size_t push(U &&val)
    {
        return push([&val](T *el, size_t){ *el = std::forward<U>(val); }, 1);
    }

    template <typename F, typename ...Args>
    size_t pop(F &&callback, size_t size, Args &&...args)
    {
        return doFifing<false>(std::forward<F>(callback), size, std::forward<Args>(args)...);
    }

    size_t pop(T &val)
    {
        return pop([&val](T *el, size_t){ val = std::move(*el); }, 1);
    }

/// utility functions
public:
    inline size_t capacity() const
    {
        return capacity_;
    }

    inline size_t size() const
    {
        return producer_.get_index_tail() - consumer_.get_index_head();
    }

    inline bool empty() const
    {
        return producer_.get_index_tail() == consumer_.get_index_head();
    }

    inline bool real_empty() const
    {
        return producer_.get_index_head() == consumer_.get_index_head();
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

    inline size_t ph() const {return producer_.get_index_head();}
    inline size_t pt() const {return producer_.get_index_tail();}
    inline size_t ch() const {return consumer_.get_index_head();}
    inline size_t ct() const {return consumer_.get_index_tail();}
    inline size_t wm() const {return water_mark_.load(std::memory_order_relaxed);}

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
};

template <typename T, bool P=false>
using ring_buffer_dd = typename tll::lf2::ccfifo<T, type::buffer, mode::dense, mode::dense, P>;
template <typename T, bool P=false>
using ring_buffer_ds = typename tll::lf2::ccfifo<T, type::buffer, mode::dense, mode::sparse, P>;
template <typename T, bool P=false>
using ring_buffer_ss = typename tll::lf2::ccfifo<T, type::buffer, mode::sparse, mode::sparse, P>;
template <typename T, bool P=false>
using ring_buffer_sd = typename tll::lf2::ccfifo<T, type::buffer, mode::sparse, mode::dense, P>;



template <typename T, bool P=false>
using ring_buffer_mpmc = typename tll::lf2::ring_buffer_dd<T, P>;

template <typename T, bool P=false>
using ring_buffer_mpsc = typename tll::lf2::ring_buffer_ds<T, P>;

template <typename T, bool P=false>
using ring_buffer_spmc = typename tll::lf2::ring_buffer_sd<T, P>;

template <typename T, bool P=false>
using ring_buffer_spsc = typename tll::lf2::ring_buffer_ss<T, P>;

} /// tll::lf
