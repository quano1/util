/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <thread>
#include <utility>

// #include <boost/lockfree/queue.hpp>
// #include <tbb/concurrent_queue.h>
// #include <concurrentqueue/concurrentqueue.h>
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
static void _perfTunnel(const auto &doPush, const auto &doPop, size_t write_count, double *time, size_t *ops)
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
            for(;total_push_count.load(std::memory_order_relaxed) < (write_count*2);)
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

int perfTunnel()
{
    std::ofstream ofs_pt{"pt.dat"};
    std::ofstream ofs_time{"pt_time.dat"};

    tll::util::CallFuncInSeq<NUM_CPU, 7>( [&](auto index_seq)
    {
        // if(index_seq.value == 1) return;
        constexpr size_t kCount = 250000 * index_seq.value;
        size_t ops[3];
        double time[3];
        LOGD("Number Of Threads: %ld, total count: %ldk", index_seq.value, kCount/1000);
        ofs_pt << index_seq.value << " ";
        ofs_time << index_seq.value * 2 << " ";
        // constexpr size_t kN2 = 0x800000;
        constexpr size_t kTN = (tll::util::isPowerOf2(kCount) ? kCount : tll::util::nextPowerOf2(kCount));

        {
            constexpr size_t kMul = 0x1000;
            constexpr size_t kTmp = (tll::util::isPowerOf2(index_seq.value * kMul) ? index_seq.value * kMul : tll::util::nextPowerOf2(index_seq.value * kMul));
            tll::lf::CCFIFO<char> fifo{kCount * 2, kTmp};
            auto doPush = [&fifo]() -> bool { return fifo.enQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); *(fifo.elemAt(i)) = 1; }); };
            auto doPop = [&fifo]() -> bool { return fifo.deQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); char val; val = *fifo.elemAt(i); }); };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _perfTunnel<index_seq.value>(doPush, doPop, kCount, time, ops);
            
            double avg_time[3] = {time[0]/index_seq.value, time[1]/index_seq.value, time[2]/index_seq.value};
            double throughput[3] = {time[0]*1000/ops[0], time[1]*1000/ops[1], time[2]*1000/ops[2]};
            ofs_pt << (throughput[0]) << " " << (throughput[1]) << " ";
            ofs_time << (avg_time[2]) << " ";
            LOGD("mul:0x%lx\t[0]:%.3f(us)/%.3f(ms), [1]:%.3f(us)/%.3f(ms), [2]:%.3f(us)/%.3f(ms)", kMul, throughput[0], avg_time[0], throughput[1], avg_time[1], throughput[2], avg_time[2]);
        }

        {
            constexpr size_t kMul = 0x2000;
            constexpr size_t kTmp = (tll::util::isPowerOf2(index_seq.value * kMul) ? index_seq.value * kMul : tll::util::nextPowerOf2(index_seq.value * kMul));
            tll::lf::CCFIFO<char> fifo{kCount * 2, kTmp};
            auto doPush = [&fifo]() -> bool { return fifo.enQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); *(fifo.elemAt(i)) = 1; }); };
            auto doPop = [&fifo]() -> bool { return fifo.deQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); char val; val = *fifo.elemAt(i); }); };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _perfTunnel<index_seq.value>(doPush, doPop, kCount, time, ops);
            
            double avg_time[3] = {time[0]/index_seq.value, time[1]/index_seq.value, time[2]/index_seq.value};
            double throughput[3] = {time[0]*1000/ops[0], time[1]*1000/ops[1], time[2]*1000/ops[2]};
            ofs_pt << (throughput[0]) << " " << (throughput[1]) << " ";
            ofs_time << (avg_time[2]) << " ";
            LOGD("mul:0x%lx\t[0]:%.3f(us)/%.3f(ms), [1]:%.3f(us)/%.3f(ms), [2]:%.3f(us)/%.3f(ms)", kMul, throughput[0], avg_time[0], throughput[1], avg_time[1], throughput[2], avg_time[2]);
        }

        {
            constexpr size_t kMul = 0x4000;
            constexpr size_t kTmp = (tll::util::isPowerOf2(index_seq.value * kMul) ? index_seq.value * kMul : tll::util::nextPowerOf2(index_seq.value * kMul));
            tll::lf::CCFIFO<char> fifo{kCount * 2, kTmp};
            auto doPush = [&fifo]() -> bool { return fifo.enQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); *(fifo.elemAt(i)) = 1; }); };
            auto doPop = [&fifo]() -> bool { return fifo.deQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); char val; val = *fifo.elemAt(i); }); };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _perfTunnel<index_seq.value>(doPush, doPop, kCount, time, ops);
            
            double avg_time[3] = {time[0]/index_seq.value, time[1]/index_seq.value, time[2]/index_seq.value};
            double throughput[3] = {time[0]*1000/ops[0], time[1]*1000/ops[1], time[2]*1000/ops[2]};
            ofs_pt << (throughput[0]) << " " << (throughput[1]) << " ";
            ofs_time << (avg_time[2]) << " ";
            LOGD("mul:0x%lx\t[0]:%.3f(us)/%.3f(ms), [1]:%.3f(us)/%.3f(ms), [2]:%.3f(us)/%.3f(ms)", kMul, throughput[0], avg_time[0], throughput[1], avg_time[1], throughput[2], avg_time[2]);
        }

        {
            constexpr size_t kMul = 0x8000;
            constexpr size_t kTmp = (tll::util::isPowerOf2(index_seq.value * kMul) ? index_seq.value * kMul : tll::util::nextPowerOf2(index_seq.value * kMul));
            tll::lf::CCFIFO<char> fifo{kCount * 2, kTmp};
            auto doPush = [&fifo]() -> bool { return fifo.enQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); *(fifo.elemAt(i)) = 1; }); };
            auto doPop = [&fifo]() -> bool { return fifo.deQueue([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); char val; val = *fifo.elemAt(i); }); };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _perfTunnel<index_seq.value>(doPush, doPop, kCount, time, ops);
            
            double avg_time[3] = {time[0]/index_seq.value, time[1]/index_seq.value, time[2]/index_seq.value};
            double throughput[3] = {time[0]*1000/ops[0], time[1]*1000/ops[1], time[2]*1000/ops[2]};
            ofs_pt << (throughput[0]) << " " << (throughput[1]) << " ";
            ofs_time << (avg_time[2]) << " ";
            LOGD("mul:0x%lx\t[0]:%.3f(us)/%.3f(ms), [1]:%.3f(us)/%.3f(ms), [2]:%.3f(us)/%.3f(ms)", kMul, throughput[0], avg_time[0], throughput[1], avg_time[1], throughput[2], avg_time[2]);
        }

        ofs_pt << "\n";
        ofs_time << "\n";
        LOGD("=========================");
    });
    ofs_pt.flush();
    ofs_time.flush();

    return 0;
}