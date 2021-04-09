/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <thread>
#include <utility>

#include <tests/third-party/boostorg/lockfree/include/boost/lockfree/queue.hpp>
#include <tests/third-party/concurrentqueue/concurrentqueue.h>

#define ENABLE_PROFILING 0
#define LOOP_COUNT 0x800
#define EXTENDING 1
// #define DUMPER
// #define NOP_LOOP(loop) for(int i__=0; i__<loop; i__++) __asm__("nop")

static char dst[LOOP_COUNT], src[LOOP_COUNT];
#define DUMMY_LOOP() memcpy(dst, src, LOOP_COUNT)
// #define DUMMY_LOOP() for(int i__=0; i__<LOOP_COUNT; i__++) __asm__("nop")


// static void dummyCpy()
// {
//     memcpy(dst, src, LOOP_COUNT);
// }

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

template <int num_of_threads>
static void _benchmark(const std::string &name, const auto &doPush, const auto &doPop, size_t write_count, double *time, size_t *ops, std::vector<std::ofstream> &ofss)
{
    // constexpr int kThreadNum = num_of_threads / 2;
    static_assert(num_of_threads > 0);
    std::atomic<size_t> total_push_count{0}, total_pop_count{0};
    tll::time::Counter<std::chrono::duration<double, std::ratio<1>>> counter; /// seconds
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
                std::this_thread::yield();
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
                std::this_thread::yield();
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
                std::this_thread::yield();
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
                std::this_thread::yield();
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


    double avg_time[3] = {time[0]*1000/num_of_threads, time[1]*1000/num_of_threads, time[2]*1000/num_of_threads};
    double throughput[3] = {ops[0]*0.000001/time[0], ops[1]*0.000001/time[1], ops[2]*0.000001/time[2]};
    ofss[0] << (throughput[0]) << " ";
    ofss[1] << (throughput[1]) << " ";
    ofss[2] << (avg_time[2]) << " ";
    LOGD("%s\t%.3f/%.3f\t%.3f/%.3f\t%.3f/%.3f\t%.3f", name.data(), throughput[0], avg_time[0], throughput[1], avg_time[1], throughput[2], avg_time[2], throughput[0]/throughput[1]);
}

int benchmark()
{
    std::vector<std::ofstream> ofss;
    ofss.resize(3);
    std::ofstream &ofs_tp_push = ofss[0];
    std::ofstream &ofs_tp_pop = ofss[1];
    std::ofstream &ofs_time = ofss[2];
    ofs_tp_push.open("bm_tp_push.dat");
    ofs_tp_pop.open("bm_tp_pop.dat");
    ofs_time.open("bm_time.dat");

    ofs_tp_push << tll::util::stringFormat(";%d\n",NUM_CPU);
    ofs_tp_pop << tll::util::stringFormat(";%d\n",NUM_CPU);
    ofs_time << tll::util::stringFormat(";%d\n",NUM_CPU);
    ofs_tp_push << tll::util::stringFormat(";push contiguously (max cpu: %d)\\nHigher is better\n",NUM_CPU);
    ofs_tp_pop << tll::util::stringFormat(";pop contiguously (max cpu: %d)\\nHigher is better\n",NUM_CPU);
    ofs_time << tll::util::stringFormat(";push and pop simultaneously (max cpu: %d)\\nLower is better\n",NUM_CPU);
    ofs_tp_push << ";Number of threads\n";
    ofs_tp_pop << ";Number of threads\n";
    ofs_time << ";Number of threads\n";
    ofs_tp_push << ";Operations (million) per second\n";
    ofs_tp_pop << ";Operations (million) per second\n";
    ofs_time << ";Average time for completing the test (ms)\n";

    ofs_tp_push << ";boost::lockfree::queue\n";
    ofs_tp_pop << ";boost::lockfree::queue\n";
    ofs_time << ";boost::lockfree::queue\n";
    ofs_tp_push << ";moodycamel::ConcurrentQueue\n";
    ofs_tp_pop << ";moodycamel::ConcurrentQueue\n";
    ofs_time << ";moodycamel::ConcurrentQueue\n";
    ofs_tp_push << ";tll::lf::ccfifo\n";
    ofs_tp_pop << ";tll::lf::ccfifo\n";
    ofs_time << ";tll::lf::ccfifo\n";

    tll::util::CallFuncInSeq<NUM_CPU, EXTENDING>( [&](auto index_seq)
    {
        // if(index_seq.value > 2) return;
        constexpr size_t kCount = 250000 * index_seq.value;
        size_t ops[3];
        double time[3];
        LOGD("Number Of Threads: %ld, total count: %ldk", index_seq.value, kCount/1000);
        ofs_tp_push << index_seq.value << " ";
        ofs_tp_pop << index_seq.value << " ";
        ofs_time << index_seq.value << " ";
        // constexpr size_t kN2 = 0x800000;
        constexpr size_t kTN = (tll::util::isPowerOf2(kCount) ? kCount : tll::util::nextPowerOf2(kCount));
        // volatile char val;
        // LOGD("%lx", kTN);

        // {
        //     tbb::concurrent_queue<char> fifo;
        //     auto doPush = [&fifo]() -> bool { NOP_LOOP(LOOP_COUNT); fifo.push((char)1); return true; };
        //     auto doPop = [&fifo]() -> bool { NOP_LOOP(LOOP_COUNT); char val; return fifo.try_pop(val); };
            
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //     _benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
        //     ofs_tp_push << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
        //     ofs_time << time[2] << " ";
        //     LOGD("tbb time\tpush:%.3f, pop: %.3f, pp: %.3f (ms)", time[0]*1000/ops[0], time[1]*1000/ops[1], time[2]*1000/ops[2]);
        // }

        // if(index_seq.value <= NUM_CPU)
        // {
        //     tll::lf::ccfifo<char, kTN> fifo{kCount * 2};
        //     auto doPush = [&fifo]() -> bool { return fifo.push([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); *(fifo.elemAt(i)) = 1; },1); };
        //     auto doPop = [&fifo]() -> bool { return fifo.pop([&fifo](size_t i, size_t s){ NOP_LOOP(LOOP_COUNT); char val; val = *fifo.elemAt(i); },1); };
        //     _benchmark<index_seq.value>(doPush, doPop, kCount, time, ops);
        //     ofs_tp_push << (ops[0] * 0.000001) / time[0] << " " << (ops[1] * 0.000001) / time[1] << " ";
        //     ofs_time << time[2] << " ";
        //     LOGD("ccfifo time\tpush:%.3f, pop: %.3f, pp: %.3f (s)", time[0], time[1], time[2]);
        // }
        // else
        // {
        //     ofs_tp_push << 0 << " " << 0 << " ";
        //     ofs_time << -1 << " ";
        // }
        LOGD("\tpush/avg(ms)\tpop/avg(ms)\tpp/avg(ms)\tpush/pop");
        {
            boost::lockfree::queue<char> fifo{kCount * 2};
            auto doPush = [&fifo]() -> bool { DUMMY_LOOP(); return fifo.push((char)1); };
            auto doPop = [&fifo]() -> bool { char val; DUMMY_LOOP(); return fifo.pop(val); };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _benchmark<index_seq.value>("boost", doPush, doPop, kCount, time, ops, ofss);
        }
        
        {
            moodycamel::ConcurrentQueue<char> fifo{kCount * 2};
            auto doPush = [&fifo]() -> bool { DUMMY_LOOP(); fifo.enqueue((char)1); return true; };
            auto doPop = [&fifo]() -> bool { char val; DUMMY_LOOP(); return fifo.try_dequeue(val); };

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _benchmark<index_seq.value>("moody", doPush, doPop, kCount, time, ops, ofss);
        }

        {
            tll::lf2::ccfifo<char, tll::lf2::Mode::kLL, tll::lf2::Mode::kLL> fifo{kCount * 2, index_seq.value * 0x4000};
            auto doPush = [&fifo]() -> bool { DUMMY_LOOP(); return fifo.push((char)1); };
            auto doPop = [&fifo]() -> bool { char val; DUMMY_LOOP(); return fifo.pop(val); };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _benchmark<index_seq.value>("ccfifo", doPush, doPop, kCount, time, ops, ofss);
        }

        ofs_tp_push << "\n";
        ofs_tp_pop << "\n";
        ofs_time << "\n";
        LOGD("=========================");
    });
    ofs_tp_push.flush();
    ofs_tp_pop.flush();
    ofs_time.flush();

    return 0;
}