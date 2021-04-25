#pragma once

#if (HAVE_OPENMP)

#include <functional>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>

#include <omp.h>
#include "../libs/util.h"
#include "../libs/counter.h"

#define NOP_LOOP() for(int i__=0; i__<0x400; i__++) __asm__("nop")
namespace tll::test {

template <int num_of_threads>
std::pair<size_t, size_t> fifing(
        const std::function<size_t(int, size_t, size_t, size_t, size_t)> &do_push, /// true == producer
        const std::function<size_t(int, size_t, size_t, size_t, size_t)> &do_pop, /// true == producer
        const std::function<bool(int)> &is_prod, /// true == producer
        const std::function<void()> &do_rest, /// std::this_thread::yield()
        size_t max_val,
        std::vector<int> &threads_lst,
        std::vector<double> &time_lst,
        std::vector<size_t> &tt_count_lst)
{
    static_assert(num_of_threads > 0);
    std::atomic<size_t> running_prod{0}, 
        tt_push_count{0}, tt_pop_count{0},
        tt_push_size{0}, tt_pop_size{0};
    tll::time::Counter<std::chrono::duration<double, std::ratio<1, 1>>> counter; /// second
    // double tt_time;
    counter.start();
    
    #pragma omp parallel num_threads ( num_of_threads )
    {
        const int kTid = omp_get_thread_num();
        size_t loop_num = 0;
        size_t lc_tt_size = 0;
        if(is_prod(kTid)) /// Producer
        {
            // LOGD("Producer: %d, cpu: %d", kTid, sched_getcpu());
            running_prod.fetch_add(1);
            for(;tt_push_size.load(std::memory_order_relaxed) < (max_val);)
            {
                size_t ret = do_push(kTid, loop_num, lc_tt_size, 
                                     tt_push_count.load(std::memory_order_relaxed),
                                     tt_push_size.load(std::memory_order_relaxed));
                if(ret)
                {
                    tt_push_count.fetch_add(1, std::memory_order_relaxed);
                    tt_push_size.fetch_add(ret, std::memory_order_relaxed);
                    lc_tt_size+=ret;
                    NOP_LOOP();
                    do_rest();
                }
                loop_num++;
            }
            running_prod.fetch_add(-1);
            // LOGD("Prod Done");
        }
        else /// Consumer
        {
            // LOGD("Consumer: %s, cpu: %d", tll::util::str_tid().data(), sched_getcpu());
            for(;
                running_prod.load(std::memory_order_relaxed) > 0
                || (tt_pop_count.load(std::memory_order_relaxed) < tt_push_count.load(std::memory_order_relaxed))
                || (tt_pop_size.load(std::memory_order_relaxed) < max_val)
                ;)
            {
                size_t ret = do_pop(kTid, loop_num, lc_tt_size, 
                                    tt_pop_count.load(std::memory_order_relaxed),
                                    tt_pop_size.load(std::memory_order_relaxed));
                if(ret)
                {
                    tt_pop_count.fetch_add(1, std::memory_order_relaxed);
                    tt_pop_size.fetch_add(ret, std::memory_order_relaxed);
                    lc_tt_size+=ret;
                    NOP_LOOP();
                    do_rest();
                }
                loop_num++;
            }
            // LOGD("Cons Done");
        }

    }
    threads_lst.push_back(num_of_threads);
    time_lst.push_back(counter.elapsed().count());
    if(do_push) tt_count_lst.push_back(tt_push_count.load());
    if(do_pop) tt_count_lst.push_back(tt_pop_count.load());
    return {tt_push_size.load(), tt_pop_size.load()};
}

void plot_data(const std::string &file_name, const std::string &title, const std::string &y_title, 
                const std::vector<std::string> column_lst,
                const std::vector<int> x_axes,
                const std::vector<double> data)
{
    std::ofstream ofs{file_name, std::ios::out | std::ios::binary};
    assert(ofs.is_open());

    // ofs << tll::util::stringFormat("#%d\n", column_lst.size());
    // ofs << tll::util::stringFormat("#%d\n", NUM_CPU);
    auto col_size = column_lst.size();
    auto x_size = data.size() / col_size;

    ofs << "#" << col_size << "\n";
    ofs << "#" << NUM_CPU << "\n";
    ofs << "#" << title << "\n";
    ofs << "#Number of threads\n";
    ofs << "#" << y_title << "\n";
    ofs << "\"\"\t";
    for(int i=0; i<col_size; i++)
    {
        ofs << "\"" << column_lst[i] << "\"\t";
    }

    // ofs << "\n" << x_axes[0] << "\t";

    for(int i=0; i<data.size(); i++)
    {
        if(i % col_size == 0)
        {
            /// 0, 4, 8, 12
            ofs << "\n" << x_axes[i/col_size] << "\t";
        }

        ofs << data[i] << "\t";
    }

    ofs.flush();

    int cmd = 0;
    cmd = system(tll::util::stringFormat("./plothist.sh %s", file_name.data()).data());
    cmd = system(tll::util::stringFormat("./plot.sh %s", file_name.data()).data());
}

} /// tll::test

#endif