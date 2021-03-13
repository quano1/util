/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <thread>
#include <utility>

#include <boost/lockfree/queue.hpp>
// #include "../libs/timer.h"
// #include "../libs/util.h"
// #include "../libs/log.h"

#define ENABLE_PROFILING 1
#define PERF_TUNNEL 0
#define DUMPER
#define NOP_LOOP(loop) for(int i__=0; i__<loop; i__++) __asm__("nop")

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

template <uint8_t type = 3>
void dumpStat(const tll::cc::Stat &st, double real_total_time)
{
    using namespace std::chrono;
    LOGD("%.3f", real_total_time);
    if(type) printf("        count(K) | err(%%)|miss(%%)| try(%%)|comp(%%)| cb(%%) | all(%%)| ops/ms\n");

    if(type & 1)
    {
        double time_push_total = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_push_total)).count();
        double time_push_try = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_push_try)).count();
        double time_push_complete = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_push_complete)).count();
        double time_push_try_rate = st.time_push_try*100.f/ st.time_push_total;
        double time_push_complete_rate = st.time_push_complete*100.f/ st.time_push_total;
        double time_push_callback_rate = st.time_push_cb*100.f/ st.time_push_total;
        double time_push_all_rate = (st.time_push_cb + st.time_push_try + st.time_push_complete)*100.f/ st.time_push_total;


        double push_total = st.push_total * 1.f / 1000;
        double push_error_rate = (st.push_error*100.f)/st.push_total;
        double push_miss_rate = (st.push_miss*100.f)/st.push_total;

        // double push_size = st.push_size*1.f / 0x100000;
        // size_t push_success = st.push_total - st.push_error;
        // double push_success_size_one = push_size/push_total;
        // double push_speed = push_success_size_one;

        // double time_push_one = st.time_push_total*1.f / st.push_total;
        // double avg_time_push_one = time_push_one / thread_num;
        // double avg_push_speed = push_size*thread_num/time_push_total;

        double opss = st.push_total * 0.001f / real_total_time;

        printf(" push: %9.3f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.f\n",
               push_total, push_error_rate, push_miss_rate
               , time_push_try_rate, time_push_complete_rate, time_push_callback_rate, time_push_all_rate
               // avg_time_push_one, avg_push_speed
               , opss
               );
    }

    if(type & 2)
    {
        double time_pop_total = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_pop_total)).count();
        double time_pop_try = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_pop_try)).count();
        double time_pop_complete = duration_cast<duration<double, std::ratio<1>>>(StatDuration(st.time_pop_complete)).count();
        double time_pop_try_rate = st.time_pop_try*100.f/ st.time_pop_total;
        double time_pop_complete_rate = st.time_pop_complete*100.f/ st.time_pop_total;
        double time_pop_callback_rate = st.time_pop_cb*100.f/ st.time_pop_total;
        double time_pop_all_rate = (st.time_pop_cb + st.time_pop_try + st.time_pop_complete)*100.f/ st.time_pop_total;


        double pop_total = st.pop_total * 1.f / 1000;
        double pop_error_rate = (st.pop_error*100.f)/st.pop_total;
        double pop_miss_rate = (st.pop_miss*100.f)/st.pop_total;

        // double pop_size = st.pop_size*1.f / 0x100000;
        // size_t pop_success = st.pop_total - st.pop_error;
        // double pop_success_size_one = pop_size/pop_total;
        // double pop_speed = pop_success_size_one;

        // double time_pop_one = st.time_pop_total*1.f / st.pop_total;
        // double avg_time_pop_one = time_pop_one / thread_num;
        // double avg_pop_speed = pop_size*thread_num/time_pop_total;
        double opss = st.pop_total * 0.001f / real_total_time;
        printf(" pop : %9.3f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.f\n",
               pop_total, pop_error_rate, pop_miss_rate
               , time_pop_try_rate, time_pop_complete_rate, time_pop_callback_rate, time_pop_all_rate
               // avg_time_pop_one
               , opss
               );
    }
}

template <int prod_num, class FIFO>
void perfTunnelCCB(size_t write_count, tll::time::Counter<> &counter, double *opss=nullptr)
{
    constexpr int kThreadNum = prod_num;
    FIFO fifo{write_count * 2};
    std::atomic<int> w_threads{0};
    assert(kThreadNum > 0);
    std::atomic<size_t> total_push_count{0}, total_pop_count{0};

    counter.start();
    #pragma omp parallel num_threads ( kThreadNum ) shared(fifo)
    {
        int i=0;
        for(;total_push_count.load(std::memory_order_relaxed) < (write_count);)
        {
            bool ws = fifo.push([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, 1);
            if(ws)
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
    dumpStat<1>(fifo.stat(), counter.lastElapsed().count());
    counter.start();
    #pragma omp parallel num_threads ( kThreadNum ) shared(fifo)
    {
        for(;total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
        {
            bool ps = fifo.pop([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, 1);
            if(ps)
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
    dumpStat<2>(fifo.stat(), counter.lastElapsed().count());

    if(opss) {
        opss[0] = fifo.stat().push_total / counter.lastElapsed().count();
        opss[1] = fifo.stat().pop_total / counter.lastElapsed().count();
    }

    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        tll::cc::Stat stat = fifo.stat();
        // printf("\n");
        printf(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        printf(" - w:%ld r:%ld\n", stat.push_size, stat.pop_size);
        abort();
    }
}

template <int prod_num, class CQ>
void perfTunnelCQ(size_t write_count, tll::time::Counter<> &counter, double *opss=nullptr)
{
    constexpr int kThreadNum = prod_num;
    CQ fifo{write_count * 2};
    std::atomic<int> w_threads{0};
    assert(kThreadNum > 0);
    std::atomic<size_t> total_push_count{0}, total_pop_count{0};

    counter.start();
    #pragma omp parallel num_threads ( kThreadNum ) shared(fifo)
    {
        int i=0;
        for(;total_push_count.load(std::memory_order_relaxed) < (write_count);)
        {
            bool ws = fifo.enQueue([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); });
            if(ws)
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
    dumpStat<1>(fifo.stat(), counter.lastElapsed().count());
    if(opss) {
        opss[0] = fifo.stat().push_total / counter.lastElapsed().count();
    }
    counter.start();
    #pragma omp parallel num_threads ( kThreadNum ) shared(fifo)
    {
        for(;total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
        {
            bool ps = fifo.deQueue([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); });
            if(ps)
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
    dumpStat<2>(fifo.stat(), counter.lastElapsed().count());

    if(opss) {
        opss[1] = fifo.stat().pop_total / counter.lastElapsed().count();
    }

    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        tll::cc::Stat stat = fifo.stat();
        // printf("\n");
        printf(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        printf(" - w:%ld r:%ld\n", stat.push_size, stat.pop_size);
        abort();
    }
}


template <int prod_num>
void perfTunnelBoost(size_t write_count, tll::time::Counter<> &counter, double *opss=nullptr)
{
    constexpr int kThreadNum = prod_num;
    boost::lockfree::queue<char> fifo{write_count * 2};

    std::atomic<int> w_threads{0};
    assert(kThreadNum > 0);
    std::atomic<size_t> total_push_count{0}, total_pop_count{0};

    counter.start();
    #pragma omp parallel num_threads ( kThreadNum ) shared(fifo)
    {
        int i=0;
        for(;total_push_count.load(std::memory_order_relaxed) < (write_count);)
        {
            bool ws = fifo.push((char)1);
            if(ws)
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
    // dumpStat<1>(fifo.stat(), counter.lastElapsed().count());
    if(opss) {
        LOGD("%ld %.3f", total_push_count.load(std::memory_order_relaxed), counter.lastElapsed().count());
        opss[0] = total_push_count.load(std::memory_order_relaxed) / counter.lastElapsed().count();
    }
    counter.start();
    #pragma omp parallel num_threads ( kThreadNum ) shared(fifo)
    {
        for(;total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
        {
            char val;
            bool ps = fifo.pop(val);
            if(ps)
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
    // dumpStat<2>(fifo.stat(), counter.lastElapsed().count());

    if(opss) {
        LOGD("%ld %.3f", total_pop_count.load(std::memory_order_relaxed), counter.lastElapsed().count());
        opss[1] = total_pop_count.load(std::memory_order_relaxed) / counter.lastElapsed().count();
    }

    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        // tll::cc::Stat stat = fifo.stat();
        // printf("\n");
        printf(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        // printf(" - w:%ld r:%ld\n", stat.push_size, stat.pop_size);
        abort();
    }
}

template<int N> void f(size_t write_count, auto &counter)
{
    LOGD("Number Of Threads: %d", N);
    {
        constexpr size_t kN2 = N * 512;
        constexpr size_t kTN = (tll::util::isPowerOf2(kN2) ? kN2 : tll::util::nextPowerOf2(kN2));
        LOGD("thread size: %ld", kTN);
        perfTunnelCQ<N, tll::lf::CCFIFO<char, kTN>>(write_count, counter);
    }
    LOGD("=========================");
}

template<int beg, class F, int... Is>
constexpr void magic(F f, std::integer_sequence<int, Is...>)
{
    int expand[] = { (f(std::integral_constant<int, beg+Is>{}), void(), 0)... };
    (void)expand;
}

template<int beg, int end, class F>
constexpr auto magic(F f)
{
    //                                              v~~~~~~~v see below (*)
    return magic<beg>(f, std::make_integer_sequence<int, end-beg+1>{});
}

void perfTunnel()
{
    std::ofstream ofs{"benchmark.dat"};
    tll::time::Counter<> counter;
    constexpr size_t kCount = 1000000;
    const int cores = std::thread::hardware_concurrency();
    magic<1, NUM_CPU>( [&](auto x)
    {
        double opss[2];
        LOGD("Number Of Threads: %d", x.value);
        perfTunnelCCB<x.value, tll::lf::CCFIFO<char>>(kCount, counter, opss);
        LOGD("=========================");
    });


    magic<1, NUM_CPU * 4>( [&](auto x)
    {
        double opss[2];
        opss[0] = 0;
        opss[1] = 0;
        LOGD("Number Of Threads: %d", x.value);
        constexpr size_t kN2 = x.value * 512;
        constexpr size_t kTN = (tll::util::isPowerOf2(kN2) ? kN2 : tll::util::nextPowerOf2(kN2));
        LOGD("thread size: %ld", kTN);
        perfTunnelCQ<x.value, tll::lf::CCFIFO<char, kTN>>(kCount, counter, opss);
        LOGD("=========================");
        ofs << x.value << " " << opss[0] * 0.001 << " " << opss[1] * 0.001 << " ";
        perfTunnelBoost<NUM_CPU>(kCount, counter, opss);
        ofs << x.value << " " << opss[0] * 0.001 << " " << opss[1] * 0.001;
        ofs << "\n";
        LOGD("=========================");
    });
    ofs << std::endl;
}

int main(int argc, char **argv)
{
    perfTunnel();
    return 0;
}