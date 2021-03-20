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
#define LOOP_COUNT 0x200
// #define DUMPER
#define NOP_LOOP(loop) for(int i__=0; i__<loop; i__++) __asm__("nop")

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

template <int num_of_threads>
void benchmark(const auto &doPush, const auto &doPop, size_t write_count, double *time, size_t *ops)
{
    // constexpr int kThreadNum = num_of_threads / 2;
    static_assert(num_of_threads > 0);
    std::atomic<size_t> total_push_count{0}, total_pop_count{0};
    tll::time::Counter<std::chrono::duration<double, std::ratio<1, 1000>>> counter; /// us
    std::atomic<int> w_threads{0};

    counter.start();
    #pragma omp parallel num_threads ( num_of_threads ) shared(doPush)
    {
        // LOGD("Thread: %s, cpu: %d", tll::util::str_tid().data(), sched_getcpu());
        for(;total_push_count.load(std::memory_order_relaxed) < (write_count);)
        {
            // bool ws = fifo.push([](size_t, size_t){ NOP_LOOP(LOOP_COUNT); }, 1);
            if(doPush())
            {
                total_push_count.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                /// overrun
                LOGD("");
                abort();
            }
        }

        // LOGD("Done");
    }
    counter.elapsed();
    time[0] = counter.lastElapsed().count();

    counter.start();
    #pragma omp parallel num_threads ( num_of_threads ) shared(doPop)
    {
        // LOGD("Thread: %s, cpu: %d", tll::util::str_tid().data(), sched_getcpu());
        for(;total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
        {
            if(doPop())
            {
                total_pop_count.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                // LOGD("");
                break;
            }
        }
        // LOGD("Done");
    }
    counter.elapsed();
    time[1] = counter.lastElapsed().count();

    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        LOGD(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        abort();
    }

    ops[0] = total_push_count.load(std::memory_order_relaxed);
    ops[1] = total_pop_count.load(std::memory_order_relaxed);

    total_push_count.store(0, std::memory_order_relaxed);
    total_pop_count.store(0, std::memory_order_relaxed);
    counter.start();
    #pragma omp parallel num_threads ( num_of_threads * 2 ) shared(doPush, doPop)
    {
        int tid = omp_get_thread_num();
        if(!(tid & 1))
        {
            for(;total_push_count.load(std::memory_order_relaxed) < (write_count);)
            {
                if(doPush())
                {
                    total_push_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
            w_threads.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            for(;w_threads.load(std::memory_order_relaxed) < (num_of_threads) || 
                total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
            {
                if(doPop())
                {
                    total_pop_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
    }
    counter.elapsed();
    time[2] = counter.lastElapsed().count();
    ops[2] = total_push_count.load(std::memory_order_relaxed);

    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        LOGD(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        abort();
    }
}

int main(int argc, char **argv)
{
    std::ofstream ofs_throughput{"bm_throughput.dat"};
    std::ofstream ofs_time{"bm_time.dat"};

    tll::util::CallFuncInSeq<NUM_CPU, 3>( [&](auto index_seq)
    {
        constexpr size_t kCount = 1000000 * index_seq.value;
        // if(index_seq.value <= 64) return;
        size_t ops[3];
        double time[3];
        LOGD("Number Of Threads: %ld, count: %ld", index_seq.value, kCount);
        ofs_throughput << index_seq.value << " ";
        ofs_time << index_seq.value * 2 << " ";
        // constexpr size_t kN2 = 0x800000;
        constexpr size_t kTN = (tll::util::isPowerOf2(kCount) ? kCount : tll::util::nextPowerOf2(kCount));
        // LOGD("%lx", kTN);

        // if(index_seq.value <= NUM_CPU)
        // {
        //     tll::lf::CCFIFO<char, kTN> fifo{kCount * 2};
        //     auto doPush = [&fifo]() -> bool { return fifo.push([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); *(fifo.elemAt(i)) = 1; },1); };
        //     auto doPop = [&fifo]() -> bool { return fifo.pop([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); char val; val = *fifo.elemAt(i); },1); };
        //     benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
        //     ofs_throughput << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
        //     ofs_time << time[2] << " ";
        //     LOGD("CCFIFO time\tpush:%.3f, pop: %.3f, pp: %.3f (s)", time[0], time[1], time[2]);
        // }
        // else
        // {
        //     ofs_throughput << 0 << " " << 0 << " ";
        //     ofs_time << -1 << " ";
        // }

        {
            tll::lf::CCFIFO<char, kTN> fifo{kCount * 2};
            auto doPush = [&fifo]() -> bool { return fifo.enQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); *(fifo.elemAt(i)) = 1; }); };
            auto doPop = [&fifo]() -> bool { return fifo.deQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); char val; val = *fifo.elemAt(i); }); };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
            // ofs_throughput << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
            // ofs_time << time[2] << " ";
            time[0]/=index_seq.value;
            time[1]/=index_seq.value;
            time[2]/=index_seq.value;
            ofs_throughput << (time[0]) << " " << (time[1]) << " ";
            ofs_time << (time[2]) << " ";
            LOGD("CCFIFO\tpush:%.3f, pop: %.3f, pp: %.3f (ms)", time[0], time[1], time[2]);
        }
        {
            boost::lockfree::queue<char> fifo{kCount * 2};
            auto doPush = [&fifo]() -> bool { NOP_LOOP(LOOP_COUNT); return fifo.push((char)1); };
            auto doPop = [&fifo]() -> bool { NOP_LOOP(LOOP_COUNT); char val; return fifo.pop(val); };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
            // ofs_throughput << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
            // ofs_time << time[2] << " ";
            time[0]/=index_seq.value;
            time[1]/=index_seq.value;
            time[2]/=index_seq.value;
            ofs_throughput << (time[0]) << " " << (time[1]) << " ";
            ofs_time << (time[2]) << " ";
            LOGD("boost\tpush:%.3f, pop: %.3f, pp: %.3f (ms)", time[0], time[1], time[2]);
        }

        // {
        //     tbb::concurrent_queue<char> fifo;
        //     auto doPush = [&fifo]() -> bool { NOP_LOOP(LOOP_COUNT); fifo.push((char)1); return true; };
        //     auto doPop = [&fifo]() -> bool { NOP_LOOP(LOOP_COUNT); char val; return fifo.try_pop(val); };
            
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //     benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
        //     ofs_throughput << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
        //     ofs_time << time[2] << " ";
        //     LOGD("tbb time\tpush:%.3f, pop: %.3f, pp: %.3f (ms)", time[0], time[1], time[2]);
        // }

        {
            moodycamel::ConcurrentQueue<char> fifo{kCount * 2};
            auto doPush = [&fifo]() -> bool { NOP_LOOP(LOOP_COUNT); fifo.enqueue((char)1); return true; };
            auto doPop = [&fifo]() -> bool { NOP_LOOP(LOOP_COUNT); char val; return fifo.try_dequeue(val); };

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
            // ofs_throughput << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
            // ofs_time << time[2] << " ";
            time[0]/=index_seq.value;
            time[1]/=index_seq.value;
            time[2]/=index_seq.value;
            ofs_throughput << (time[0]) << " " << (time[1]) << " ";
            ofs_time << (time[2]) << " ";
            LOGD("MoodyC\tpush:%.3f, pop: %.3f, pp: %.3f (ms)", time[0], time[1], time[2]);
        }

        ofs_throughput << "\n";
        ofs_time << "\n";
        LOGD("=========================");
    });
    ofs_throughput.flush();
    ofs_time.flush();

    return 0;
}