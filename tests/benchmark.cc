/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <thread>
#include <utility>

#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_queue.h>
#include <concurrentqueue/concurrentqueue.h>
// #include "../libs/timer.h"
// #include "../libs/util.h"
// #include "../libs/log.h"

#define ENABLE_PROFILING 0
#define PERF_TUNNEL 0
// #define DUMPER
#define NOP_LOOP(loop) for(int i__=0; i__<loop; i__++) __asm__("nop")

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

template <int prod_num>
void _benchmark(const auto &doPush, const auto &doPop, size_t write_count, double *time, size_t *ops)
{
    constexpr int kThreadNum = prod_num;
    static_assert(kThreadNum > 0);
    std::atomic<int> w_threads{0};
    std::atomic<size_t> total_push_count{0}, total_pop_count{0};
    tll::time::Counter<> counter;

    counter.start();
    #pragma omp parallel num_threads ( kThreadNum ) shared(doPush)
    {
        int i=0;
        for(;total_push_count.load(std::memory_order_relaxed) < (write_count);)
        {
            // bool ws = fifo.push([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, 1);
            if(doPush())
            {
                total_push_count.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                /// overrun
                abort();
            }
        }

        w_threads.fetch_add(1, std::memory_order_relaxed);
    }
    counter.elapsed();
    time[0] = counter.lastElapsed().count();

    counter.start();
    #pragma omp parallel num_threads ( kThreadNum ) shared(doPop)
    {
        for(;total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
        {
            if(doPop())
            {
                total_pop_count.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                break;
            }
        }
    }
    counter.elapsed();
    time[1] = counter.lastElapsed().count();

    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        printf(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        abort();
    }

    ops[0] = total_push_count.load(std::memory_order_relaxed);
    ops[1] = total_pop_count.load(std::memory_order_relaxed);
}

void benchmark()
{
    std::ofstream ofs{"benchmark.dat"};
    constexpr size_t kCount = 2000000;

    // CallFuncInSeq<1, NUM_CPU>( [&](auto x)
    // {
    //     double time[2], ops[2];
    //     tll::lf::CCFIFO<char> fifo{kCount * 2};
    //     auto doPush = [&]() -> bool { return fifo.push([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, 1); };
    //     auto doPop = [&]() -> bool { return fifo.pop([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, 1); };

    //     LOGD("Number Of Threads: %d", index.value);
    //     _benchmark<index.value>(doPush, doPop, kCount, time, ops);
    //     LOGD("=========================");
    // });

    tll::util::CallFuncInSeq<NUM_CPU, 12>( [&](auto index_seq)
    {
        size_t ops[2];
        double time[2];
        LOGD("Number Of Threads: %ld", index_seq.value);
        ofs << index_seq.value << " ";
        constexpr size_t kN2 = index_seq.value * 512;
        constexpr size_t kTN = (tll::util::isPowerOf2(kN2) ? kN2 : tll::util::nextPowerOf2(kN2));

        {
            tll::lf::CCFIFO<char, kTN> fifo{kCount * 2};
            auto doPush = [&fifo]() -> bool { return fifo.enQueue([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }); };
            auto doPop = [&fifo]() -> bool { return fifo.deQueue([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }); };
            _benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
            ofs << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
            LOGD("CCFIFO time(s)\tpush:%.3f, pop: %.3f", time[0], time[1]);
        }

        {
            boost::lockfree::queue<char> fifo{kCount * 2};
            auto doPush = [&fifo]() -> bool { return fifo.push((char)1); };
            auto doPop = [&fifo]() -> bool { char val; return fifo.pop(val); };
            _benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
            ofs << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
            LOGD("Boost time(s)\tpush:%.3f, pop: %.3f", time[0], time[1]);
        }

        {
            tbb::concurrent_queue<char> fifo;
            auto doPush = [&fifo]() -> bool { fifo.push((char)1); return true; };
            auto doPop = [&fifo]() -> bool { char val; return fifo.try_pop(val); };
            _benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
            ofs << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
            LOGD("TBB time(s)\tpush:%.3f, pop: %.3f", time[0], time[1]);
        }

        {
            moodycamel::ConcurrentQueue<char> fifo;
            auto doPush = [&fifo]() -> bool { fifo.enqueue((char)1); return true; };
            auto doPop = [&fifo]() -> bool { char val; return fifo.try_dequeue(val); };
            _benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
            ofs << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
            LOGD("MC time(s)\tpush:%.3f, pop: %.3f", time[0], time[1]);
        }

        // {
        //     tll::lf::CCFIFO<char> fifo{kCount * 2};
        //     auto doPush = [&]() -> bool { return fifo.push([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, 1); };
        //     auto doPop = [&]() -> bool { return fifo.pop([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, 1); };
        //     _benchmark<index.value>(doPush, doPop, kCount, time, ops);
        //     ofs << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
        // }

        ofs << "\n";
        LOGD("=========================");
    });
    ofs.flush();
}

int main(int argc, char **argv)
{
    benchmark();
    return 0;
}