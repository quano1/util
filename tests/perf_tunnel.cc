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
#define LOOP_COUNT 0x800
#define EXTENDING -1u
// #define DUMPER
// #define NOP_LOOP(loop) for(int i__=0; i__<loop; i__++) __asm__("nop")
static char dst[LOOP_COUNT], src[LOOP_COUNT];
#define DUMMY_LOOP() memcpy(dst, src, LOOP_COUNT)

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

template <int num_of_threads, typename Fifo, typename FPush, typename Fpop>
static void doFifing(Fifo &fifo, const FPush &doPush, const Fpop &doPop, size_t write_count, double *time, size_t *ops)
{
    // constexpr int kThreadNum = num_of_threads / 2;
    static_assert(num_of_threads > 0);
    std::atomic<size_t> total_push_count{0}, total_pop_count{0};
    tll::time::Counter<std::chrono::duration<double, std::ratio<1, 1000>>> counter; /// us
    std::atomic<int> w_threads{0};
    double tt_time;

    counter.start();
    #pragma omp parallel num_threads ( num_of_threads ) shared(doPush)
    {
        // LOGD("Thread: %s, cpu: %d", tll::util::str_tid().data(), sched_getcpu());
        for(;total_push_count.load(std::memory_order_relaxed) < (write_count);)
        {
            // bool ws = fifo.push([](size_t, size_t){ DUMMY_LOOP(); }, 1);
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
            std::this_thread::yield();
        }

        // LOGD("Done");
    }
    tt_time = counter.elapsed().count();
    {
        auto stats = fifo.statistics();
        tll::cc::dumpStat<1>(stats, tt_time);
    }
    time[0] = tt_time;

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
            std::this_thread::yield();
        }
        // LOGD("Done");
    }
    tt_time = counter.elapsed().count();
    {
        auto stats = fifo.statistics();
        tll::cc::dumpStat<2>(stats, tt_time);
    }
    time[1] = tt_time;

    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        LOGD(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        abort();
    }

    ops[0] = total_push_count.load(std::memory_order_relaxed);
    ops[1] = total_pop_count.load(std::memory_order_relaxed);

    total_push_count.store(0, std::memory_order_relaxed);
    total_pop_count.store(0, std::memory_order_relaxed);
    fifo.reset();
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
    tt_time = counter.elapsed().count();
    {
        auto stats = fifo.statistics();
        tll::cc::dumpStat<>(stats, tt_time);
    }
    time[2] = tt_time;
    ops[2] = total_push_count.load(std::memory_order_relaxed);

    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        LOGD(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        abort();
    }
}

template <int num_of_threads, typename Fifo, typename FPush, typename Fpop>
static void __perfTunnel(Fifo &fifo, size_t test_val, size_t count,
                        std::vector<double> &tp_push_lst,
                        std::vector<double> &tp_pop_lst,
                        std::vector<double> &avg_time_lst,
                        const FPush &doPush,
                        const Fpop &doPop)
                        // std::ofstream &ofs_tp_push, std::ofstream &ofs_tp_pop, std::ofstream &ofs_time)
{
    size_t ops[3];
    double time[3];
    // size_t tn = (tll::util::isPowerOf2(count) ? count : tll::util::nextPowerOf2(count));
    size_t threads_indicies_size = (tll::util::isPowerOf2(test_val*num_of_threads) ? test_val*num_of_threads : tll::util::nextPowerOf2(test_val*num_of_threads));
    fifo.reserve(count * 2, threads_indicies_size);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    doFifing<num_of_threads>(fifo, doPush, doPop, count, time, ops);

    /// average time to complete the test (divide by the number of threads)
    double avg_time[3] = {time[0]/num_of_threads, time[1]/num_of_threads, time[2]/num_of_threads};
    /// number of operations per time
    double throughput[3] = {(ops[0]/time[0])*0.001, (ops[1]/time[1])*0.001, (ops[2]/time[2])*0.001};
    tp_push_lst.push_back(throughput[0]);
    tp_pop_lst.push_back(throughput[1]);
    avg_time_lst.push_back(avg_time[2]);

    LOGD("val:0x%lx\t[0]:%.3f/%.3f\t[1]:%.3f/%.3f\t[2]:%.3f/%.3f", test_val, throughput[0], avg_time[0], throughput[1], avg_time[1], throughput[2], avg_time[2]);
}


static void _perfTunnel(
                        std::ofstream &ofs_tp_push,
                        std::ofstream &ofs_tp_pop,
                        std::ofstream &ofs_avg_time,
                        std::vector<size_t> test_list)
                        // std::ofstream &ofs_tp_push, std::ofstream &ofs_tp_pop, std::ofstream &ofs_time)
{
    size_t ops[3];
    double time[3];

    // std::string columns = "\"\"\tboost::lockfree::queue\tmoodycamel::ConcurrentQueue\ttll::lf::ccfifo";

    ofs_tp_push << "#9\n" << tll::util::stringFormat("#%d\n",NUM_CPU) 
        << tll::util::stringFormat("#push contiguously (max cpu: %d)\\nHigher is better\n",NUM_CPU)
        << "#Number of threads\n"
        << "#Operations (million) per second\n\"\"\t";

    ofs_tp_pop << "#9\n" << tll::util::stringFormat("#%d\n",NUM_CPU) 
        << tll::util::stringFormat("#pop contiguously (max cpu: %d)\\nHigher is better\n",NUM_CPU)
        << "#Number of threads\n"
        << "#Operations (million) per second\n\"\"\t";

    ofs_avg_time << "#9\n" << tll::util::stringFormat("#%d\n",NUM_CPU) 
        << tll::util::stringFormat("#push and pop simultaneously (max cpu: %d)\\nLower is better\n",NUM_CPU)
        << "#Number of threads\n"
        << "#Average time for completing the test (ms)\n\"\"\t";

    // ofs_tp_push << tll::util::stringFormat(";%d\n",NUM_CPU);
    // ofs_tp_pop << tll::util::stringFormat(";%d\n",NUM_CPU);
    // ofs_avg_time << tll::util::stringFormat(";%d\n",NUM_CPU);
    // ofs_tp_push << tll::util::stringFormat(";push contiguously (max cpu: %d)\\nClose to 1 is better\n",NUM_CPU);
    // ofs_tp_pop << tll::util::stringFormat(";pop contiguously (max cpu: %d)\\nClose to 1 is better\n",NUM_CPU);
    // ofs_avg_time << tll::util::stringFormat(";push and pop simultaneously (max cpu: %d)\\nClose to 1 is better\n",NUM_CPU);

    // ofs_tp_push << ";Number of threads\n";
    // ofs_tp_pop << ";Number of threads\n";
    // ofs_avg_time << ";Number of threads\n";
    // ofs_tp_push << ";Throughput (millions operations/second)\n";
    // ofs_tp_pop << ";Throughput (millions operations/second)\n";
    // ofs_avg_time << ";Rate of Average time\n";

    for(auto val : test_list)
    {
        std::string str = tll::util::stringFormat("\"queue (0x%lx)\"\t", val);
        ofs_tp_push << str;
        ofs_tp_pop << str;
        ofs_avg_time << str;
    }

    for(auto val : test_list)
    {
        std::string str = tll::util::stringFormat("\"bufferDD (0x%lx)\"\t", val);
        ofs_tp_push << str;
        ofs_tp_pop << str;
        ofs_avg_time << str;
    }

    for(auto val : test_list)
    {
        std::string str = tll::util::stringFormat("\"bufferSS (0x%lx)\"\t", val);
        ofs_tp_push << str;
        ofs_tp_pop << str;
        ofs_avg_time << str;
    }

    ofs_tp_push << "\n";
    ofs_tp_pop << "\n";
    ofs_avg_time << "\n";

    tll::util::CallFuncInSeq<NUM_CPU, EXTENDING>( [&](auto index_seq)
    {
        // if(index_seq.value < NUM_CPU) return;
        ofs_tp_push << index_seq.value << " ";
        ofs_tp_pop << index_seq.value << " ";
        ofs_avg_time << index_seq.value << " ";
        constexpr size_t kCount = 250000 * index_seq.value;
        LOGD("Number Of Threads: %ld, total count: %ldK", index_seq.value, kCount/1000);
        // for (size_t i=0,tis=0x10000; i<8; i++,tis*=2)
        std::vector<double> tp_push_lst, tp_pop_lst, avg_time_lst;

        tll::lf2::ccfifo<char, tll::lf2::mode::dense, tll::lf2::mode::dense, true> fifoDD;
        tll::lf2::ccfifo<char, tll::lf2::mode::sparse, tll::lf2::mode::sparse, true> fifoSS;

        auto doPushHH = [&fifoDD]() -> bool { DUMMY_LOOP(); return fifoDD.push( (char)(1) ); };
        auto doPopHH = [&fifoDD]() -> bool { char val; DUMMY_LOOP(); return fifoDD.pop(val); };

        auto doPushLL = [&fifoSS]() -> bool { DUMMY_LOOP(); return fifoSS.push( (char)(1) ); };
        auto doPopLL = [&fifoSS]() -> bool { char val; DUMMY_LOOP(); return fifoSS.pop(val); };

        auto doEnQ = [&fifoDD]() -> bool { DUMMY_LOOP(); return fifoDD.enQueue( (char)(1) ); };
        auto doDeQ = [&fifoDD]() -> bool { char val; DUMMY_LOOP(); return fifoDD.deQueue(val); };

        for(auto val : test_list)
        {
            __perfTunnel<index_seq.value>(fifoDD, val, kCount, tp_push_lst, tp_pop_lst, avg_time_lst, doEnQ, doDeQ);
            ofs_tp_push << tp_push_lst.back() << " ";
            ofs_tp_pop << tp_pop_lst.back() << " ";
            ofs_avg_time << avg_time_lst.back() << " ";

            fifoDD.reset();

            __perfTunnel<index_seq.value>(fifoDD, val, kCount, tp_push_lst, tp_pop_lst, avg_time_lst, doPushHH, doPopHH);
            ofs_tp_push << tp_push_lst.back() << " ";
            ofs_tp_pop << tp_pop_lst.back() << " ";
            ofs_avg_time << avg_time_lst.back() << " ";


            __perfTunnel<index_seq.value>(fifoSS, val, kCount, tp_push_lst, tp_pop_lst, avg_time_lst, doPushLL, doPopLL);
            ofs_tp_push << tp_push_lst.back() << " ";
            ofs_tp_pop << tp_pop_lst.back() << " ";
            ofs_avg_time << avg_time_lst.back() << " ";
        }
        ofs_tp_push << "\n";
        ofs_tp_pop << "\n";
        ofs_avg_time << "\n";

        LOGD("=========================");
    });
}

int perfTunnel()
{
    std::ofstream ofs_tp_push{"pt_tp_push.dat"};
    std::ofstream ofs_tp_pop{"pt_tp_pop.dat"};
    std::ofstream ofs_avg_time{"pt_time.dat"};

    LOGD("val: Size of threads indices");
    LOGD("Million of operations per seconds (higher better)");
    LOGD("Average milliseconds to complete the test (lower better)");
    LOGD("\t\t[0]:push only\t\t[1]:pop only\t\t[2]:push pop simul (threads x 2)");

    _perfTunnel(ofs_tp_push, ofs_tp_pop, ofs_avg_time,
                {0x1000, 0x2000, 0x4000});

    ofs_tp_push.flush();
    ofs_tp_pop.flush();
    ofs_avg_time.flush();

    return 0;
}