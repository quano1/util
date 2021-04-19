#pragma once

#if (HAVE_OPENMP)

#include <functional>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

#include <omp.h>
#include "../libs/util.h"
#include "../libs/counter.h"

namespace tll::test {

template <int num_of_threads>
void fifing(
        const std::function<size_t(int, size_t, size_t)> &do_push, /// true == producer
        const std::function<size_t(int, size_t, size_t)> &do_pop, /// true == producer
        const std::function<bool(int)> &is_prod, /// true == producer
        const std::function<void()> &resting, /// std::this_thread::yield()
        size_t max_val,
        std::vector<double> &time_lst,
        std::vector<size_t> &total_lst)
{
    static_assert(num_of_threads > 0);
    std::atomic<size_t> running_prod{0}, total_push{0}, total_pop{0};
    tll::time::Counter<std::chrono::duration<double, std::ratio<1, 1>>> counter; /// second
    // double tt_time;
    counter.start();
    
    #pragma omp parallel num_threads ( num_of_threads )
    {
        const int kTid = omp_get_thread_num();
        size_t loop_num = 0;
        size_t local_total = 0;
        if(is_prod(kTid)) /// Producer
        {
            LOGD("Producer: %d, cpu: %d", kTid, sched_getcpu());
            running_prod.fetch_add(1);
            for(;total_push.load(std::memory_order_relaxed) < (max_val);)
            {
                size_t ret = do_push(kTid, loop_num, local_total);
                if(ret)
                {
                    total_push.fetch_add(ret, std::memory_order_relaxed);
                    local_total+=ret;
                }
                loop_num++;
                resting();
            }
            running_prod.fetch_add(-1);
        }
        else /// Consumer
        {
            // LOGD("Consumer: %s, cpu: %d", tll::util::str_tid().data(), sched_getcpu());
            for(;
                (total_pop.load(std::memory_order_relaxed) < max_val)
                || (total_pop.load(std::memory_order_relaxed) < total_push.load(std::memory_order_relaxed))
                || running_prod.load(std::memory_order_relaxed) > 0
                ;)
            {
                size_t ret = do_pop(kTid, loop_num, local_total);
                if(ret)
                {
                    total_pop.fetch_add(ret, std::memory_order_relaxed);
                    local_total+=ret;
                }
                loop_num++;
                resting();
            }
        }

        // LOGD("Done");
    }

    time_lst.push_back(counter.elapsed().count());
    total_lst.push_back(total_push.load());
    total_lst.push_back(total_pop.load());
}

}

#endif