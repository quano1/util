/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <thread>
#include <utility>
// #include "../libs/timer.h"
// #include "../libs/util.h"
// #include "../libs/log.h"

#define ENABLE_PROFILING 1
#define PERF_TUNNEL 0
// #define DUMPER
#define NOP_LOOP(loop) for(int i__=0; i__<loop; i__++) __asm__("nop")

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

template <uint8_t type = 3>
void dumpStat(const tll::cc::Stat &st, double real_total_time)
{
    using namespace std::chrono;
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

bool verifyWithTemplate(int &index, const std::vector<char> &sb, const std::vector<char> temp_data [] )
{
    index=0;
    for(; index<sb.size();)
    {
        const char val = sb[index];
        if(val == 0)
            break;
        if(val == '{' || val == '}' || val == '|')
        {
            index++;
            continue;
        }

        const auto &td = temp_data[val-1];
        int rs = memcmp(sb.data() + index, td.data() + 1, val);
        if(rs)
            return false;

        index += val;
    }
    return true;
}

// bool testTimer()
// {
//     typedef tll::time::Counter<std::chrono::duration<uint32_t, std::ratio<1, 1000000>>> UsCounter;
//     // tll::time::Map<std::chrono::duration<uint32_t, std::ratio<1, 1000>>> timer{{"all"}};
//     tll::time::Map<UsCounter> timer{{"all"}};
//     timer("all").start();

//     uint32_t total_delta = 0;
//     constexpr uint32_t kPeriodUs = 50000;
//     constexpr int kLoop = 10;
//     for (int i=0; i<kLoop; i++)
//     {
//         uint32_t delta = timer().stop().start().duration().count();
//         total_delta+=delta;
//         if(total_delta >= kPeriodUs)
//         {
//             total_delta = total_delta%kPeriodUs;
//         }

//         uint32_t sleep_us = kPeriodUs - total_delta;
//         LOGD("sleep: %d (us)", sleep_us);
//         std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
//     }

//     // total_delta = timer("all").stop().duration().count();
//     LOGD("total default: %d (us)", timer().stop().total().count());
//     LOGD("total all: %d (us)", timer("all").stop().total().count());

//     return timer("all").total().count() / 10000 == timer().total().count() / 10000;
// }

// bool testGuard()
// {
//     TLL_GLOGTF();
//     // tll::time::Map<tll::time::Map<>::Duration, std::chrono::system_clock> timer{};
//     typedef tll::time::Counter<tll::time::Counter<>::Duration, std::chrono::system_clock> SystemCounter;
//     tll::time::Map<SystemCounter> timer{};
//     {
//         tll::util::Guard<SystemCounter> time_guard{timer()};
//         std::this_thread::sleep_for(std::chrono::milliseconds(750));
//     }

//     LOGD("total: %f", timer().total().count());
//     LOGD("last: %f", timer().duration().count());
//     LOGD("elapse: %f", timer().elapse().count());

//     {
//         timer().start();
//         std::this_thread::sleep_for(std::chrono::milliseconds(750));
//         timer().stop();
//     }

//     LOGD("total: %f", timer().total().count());
//     LOGD("last: %f", timer().duration().count());
//     LOGD("elapse: %f", timer().elapse().count());

//     return std::round(timer().total().count()) == std::round(tracer__.timer().elapse().count()) && std::round(timer().duration(0).count()) == std::round(timer().duration(1).count());
// }


template <int thread_num, class CCB>
bool testCCB(const std::string &ccb_type, size_t ccb_size, size_t write_size, tll::time::Counter<> &counter, size_t *opss=nullptr)
{
    constexpr int omp_thread_num = thread_num * 2;
    CCB ccb(ccb_size);
    std::vector<char> store_buff[thread_num];

#if (defined PERF_TUNNEL) && (PERF_TUNNEL > 0)
    std::vector<char> temp_data[1];
    temp_data[0].resize(PERF_TUNNEL);
    const size_t kPSize = 1;
#else
    const std::vector<char> temp_data[] = {
{'{',1,'}'},
{'{',2,2,'}'},
{'{',3,3,3,'}'},
{'{',4,4,4,4,'}'},
{'{',5,5,5,5,5,'}'},
{'{',6,6,6,6,6,6,'}'},
{'{',7,7,7,7,7,7,7,'}'},
{'{',8,8,8,8,8,8,8,8,'}'},
{'{',9,9,9,9,9,9,9,9,9,'}'},
{'{',10,10,10,10,10,10,10,10,10,10,'}'},
{'{',11,11,11,11,11,11,11,11,11,11,11,'}'},
{'{',12,12,12,12,12,12,12,12,12,12,12,12,'}'},
{'{',13,13,13,13,13,13,13,13,13,13,13,13,13,'}'},
{'{',14,14,14,14,14,14,14,14,14,14,14,14,14,14,'}'},
{'{',15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,'}'},
{'{',16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,'}'},
{'{',17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,'}'},
{'{',18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,'}'},
{'{',19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,'}'},
{'{',20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,'}'},
{'{',21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,'}'},
{'{',22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,'}'},
{'{',23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,'}'},
{'{',24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,'}'},
{'{',25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,'}'},
{'{',26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,'}'},
{'{',27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,'}'},
{'{',28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,'}'},
{'{',29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,'}'},
{'{',30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,'}'},
{'{',31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,'}'},
{'{',32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,'}'},
    };

    for(int i=0; i<thread_num; i++)
    {
        store_buff[i].resize(write_size + 34 * thread_num); /// temp_data
        memset(store_buff[i].data(), 0, store_buff[i].size());
    }
#endif
    std::atomic<int> w_threads{0};

    assert(omp_thread_num > 1);
    counter.start();
    std::atomic<size_t> total_push_size{0}, total_pop_size{0};
    #pragma omp parallel num_threads ( omp_thread_num ) shared(ccb, store_buff, temp_data)
    {
        int tid = omp_get_thread_num();
        if(!(tid & 1))
        {
            // LOGD("Producer: %d, cpu: %d", tid, sched_getcpu());
            int i=0;
            for(;total_push_size.load(std::memory_order_relaxed) < (write_size);)
            {
#if (defined PERF_TUNNEL) && (PERF_TUNNEL > 0)
                size_t ws = ccb.push([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, kPSize);
#else
                size_t ws = ccb.push([&ccb, &temp_data, &i](size_t id, size_t size) {
                        auto dst = ccb.elemAt(id);
                        auto src = temp_data[i].data();
                        memcpy(dst, src, size);
                    }, temp_data[i].size());
#endif
                if(ws > 0)
                {
                    total_push_size.fetch_add(ws, std::memory_order_relaxed);
#if (!defined PERF_TUNNEL) || (PERF_TUNNEL==0)
                    (++i) &= 31;
#endif
                }
                else
                {
                    /// overrun
                    std::this_thread::yield();
                }
            }

            w_threads.fetch_add(1, std::memory_order_relaxed);
        }
        else if (tid & 1)
        {
            // LOGD("Consumer: %d, cpu: %d", tid, sched_getcpu());
            size_t pop_size=0;
            for(;w_threads.load(std::memory_order_relaxed) < thread_num /*- (thread_num + 1) / 2*/
                || total_push_size.load(std::memory_order_relaxed) > total_pop_size.load(std::memory_order_relaxed);)
            {
#if (defined PERF_TUNNEL) && (PERF_TUNNEL > 0)
                size_t ps = ccb.pop([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); }, kPSize);
#else
                size_t ps = ccb.pop([&ccb, &store_buff, &pop_size, tid](size_t id, size_t size) {
                    auto dst = store_buff[tid/2].data() + pop_size;
                    auto src = ccb.elemAt(id);
                    memcpy(dst, src, size);
                }, ccb_size);
#endif
                if(ps > 0)
                {
                    pop_size += ps;
                    total_pop_size.fetch_add(ps, std::memory_order_relaxed);
                }
                else
                {
                    /// underrun
                    std::this_thread::yield();
                }
            }
        }
        // LOGD("          %d Done", tid);
    }
    counter.elapsed();

// #if (!defined PERF_TUNNEL) || (PERF_TUNNEL==0)
    printf("CC type: %s\n", ccb_type.data());
    dumpStat<>(ccb.stat(), counter.lastElapsed().count());
// #endif

    if(opss) {
        opss[0] = ccb.stat().push_total;
        opss[1] = ccb.stat().pop_total;
    }
    // tll::util::dump(ccb.buffer_.data(), ccb.buffer_.size());
    // for(int i=0; i<thread_num; i++)
    // {
    //     LOGD("%d", i);
    //     tll::util::dump(store_buff[i].data(), store_buff[i].size(), 0, -1, false);
    // }


    if(total_push_size.load(std::memory_order_relaxed) != total_pop_size.load(std::memory_order_relaxed))
    {
        tll::cc::Stat stat = ccb.stat();
        // printf("\n");
        printf(" - w:%ld r:%ld\n", total_push_size.load(std::memory_order_relaxed), total_pop_size.load(std::memory_order_relaxed));
        printf(" - w:%ld r:%ld\n", stat.push_size, stat.pop_size);
        return false;
    }
    bool ret = true;
#if (!defined PERF_TUNNEL) || (PERF_TUNNEL==0)
    // size_t tt_size=0;
    for(int t=0; t<thread_num; t++)
    {
        int index;
        if(!verifyWithTemplate(index, store_buff[t], temp_data))
        {
            printf(" cons: %d", t);
            tll::util::dump(store_buff[t].data(), store_buff[t].size(),0,index);
            ret = false;
        }
    }
#endif
    // printf("\n");

    return ret;
}

// bool testCCB(int loop=1)
// {
//     constexpr size_t kOneMb = 1024 * 1024;
//     constexpr size_t kOneGb = kOneMb * 1024;
//     size_t write_size = kOneMb;
//     tll::time::Counter<> counter;
//     std::ofstream ofs{"profile.dat"};
//     size_t opss[2];

// #if (defined PERF_TUNNEL) && (PERF_TUNNEL > 0)
//     size_t const ccb_size = write_size / 8;

//     _testCCB<1, tll::lf::CCFIFO<char>>("lf", ccb_size, write_size, counter, opss);
//     printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
//     ofs << 1 << " " << opss[0] * 0.001f / counter.lastElapsed().count() << " " << opss[1] * 0.001f / counter.lastElapsed().count();
//     ofs << std::endl;

//     _testCCB<2, tll::lf::CCFIFO<char>>("lf", ccb_size, write_size, counter, opss);
//     printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
//     ofs << 2 << " " << opss[0] * 0.001f / counter.lastElapsed().count() << " " << opss[1] * 0.001f / counter.lastElapsed().count();
//     ofs << std::endl;

// #else
//     size_t const ccb_size = kOneMb;
//     printf("================================================================\n");
//     for (int l=0; l<loop; l++)
//     {
//         printf("ccb_size: %.3fMb(0x%lx), write_size: %.3fGb(0x%lx)\n", ccb_size*1.f/kOneMb, ccb_size, write_size*1.f/kOneGb, write_size);
//         printf("----------------------------------------------------------------\n");
//         printf("threads: 1\n");
//         ofs << write_size << " ";
//         if(!_testCCB<1, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter, opss)) return false;
//         printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
//         ofs << opss[0] * 0.001f / counter.lastElapsed().count() << " " << opss[1] * 0.001f / counter.lastElapsed().count() << " ";
//         printf("-------------------------------\n");
//         if(!_testCCB<1, tll::lf::CCFIFO<char>>("lf", ccb_size, write_size, counter, opss)) return false;
//         printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
//         ofs << opss[0] * 0.001f / counter.lastElapsed().count() << " " << opss[1] * 0.001f / counter.lastElapsed().count() << " ";
//         printf("-------------------------------\n");

//         printf("----------------------------------------------------------------\n");
//         printf("threads: 2\n");
//         if(!_testCCB<2, tll::lf::CCFIFO<char>>("lf", ccb_size, write_size, counter, opss)) return false;
//         printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
//         ofs << opss[0] * 0.001f / counter.lastElapsed().count() << " " << opss[1] * 0.001f / counter.lastElapsed().count() << " ";
//         printf("-------------------------------\n");

//         printf("Total duration: %.6f(s)\n", counter.totalElapsed().count());
//         // printf("\n");
//         printf("----------------------------------------------------------------\n");
//         write_size *= 2;
//         ofs << std::endl;
//     }
//     printf("================================================================\n");
// #endif

//     return true;
// }


template <int prod_num, class CQ>
bool testCQ(size_t capacity, size_t write_count, tll::time::Counter<> &counter, size_t *opss=nullptr)
{
    constexpr int kThreadNum = prod_num * 2;
    CQ cq{capacity};
    std::vector<char> store_buff[prod_num];

#if (defined PERF_TUNNEL) && (PERF_TUNNEL > 0)
    std::vector<char> temp_data[1];
    // temp_data[0].resize(PERF_TUNNEL);
    // const size_t kPSize = 1;
#else
    const std::vector<char> temp_data[] = {
{'{',1,'}'},
{'{',2,2,'}'},
{'{',3,3,3,'}'},
{'{',4,4,4,4,'}'},
{'{',5,5,5,5,5,'}'},
{'{',6,6,6,6,6,6,'}'},
{'{',7,7,7,7,7,7,7,'}'},
{'{',8,8,8,8,8,8,8,8,'}'},
{'{',9,9,9,9,9,9,9,9,9,'}'},
{'{',10,10,10,10,10,10,10,10,10,10,'}'},
{'{',11,11,11,11,11,11,11,11,11,11,11,'}'},
{'{',12,12,12,12,12,12,12,12,12,12,12,12,'}'},
{'{',13,13,13,13,13,13,13,13,13,13,13,13,13,'}'},
{'{',14,14,14,14,14,14,14,14,14,14,14,14,14,14,'}'},
{'{',15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,'}'},
{'{',16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,'}'},
{'{',17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,'}'},
{'{',18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,'}'},
{'{',19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,'}'},
{'{',20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,'}'},
{'{',21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,'}'},
{'{',22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,'}'},
{'{',23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,'}'},
{'{',24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,'}'},
{'{',25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,'}'},
{'{',26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,'}'},
{'{',27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,'}'},
{'{',28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,'}'},
{'{',29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,'}'},
{'{',30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,'}'},
{'{',31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,'}'},
{'{',32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,'}'},
    };

    for(int i=0; i<prod_num; i++)
    {
        store_buff[i].resize(34 * write_count * prod_num); /// temp_data
        memset(store_buff[i].data(), 0, store_buff[i].size());
    }
#endif
    std::atomic<int> w_threads{0};

    assert(kThreadNum > 1);
    counter.start();
    std::atomic<size_t> total_push_count{0}, total_pop_count{0};
#ifdef DUMPER
    std::thread dump_stat{[&](){
        for(;w_threads.load(std::memory_order_relaxed) < prod_num /*- (prod_num + 1) / 2*/
            || total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            LOGD("DUMPER: %s", cq.dump().data());
        }
    }};
#endif

    #pragma omp parallel num_threads ( kThreadNum ) shared(cq, store_buff, temp_data)
    {
        int tid = omp_get_thread_num();
        if(!(tid & 1))
        {
            // LOGD("Producer: %s, cpu: %d", tll::util::str_tid().data(), sched_getcpu());
            int i=0;
            for(;total_push_count.load(std::memory_order_relaxed) < (write_count);)
            {
#if (defined PERF_TUNNEL) && (PERF_TUNNEL > 0)
                bool ws = cq.enQueue([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); });
#else
                bool ws = cq.enQueue([&cq, &temp_data, &i](size_t id, size_t size) {
                        std::vector<char> *dst = cq.elemAt(id);
                        *dst = temp_data[i];
                        // LOGD("> %ld %ld", id, dst->size());
                    });
#endif
                if(ws)
                {
                    total_push_count.fetch_add(1, std::memory_order_relaxed);
#if (!defined PERF_TUNNEL) || (PERF_TUNNEL==0)
                    (++i) &= 31;
#endif
                }
                else
                {
                    /// overrun
                    std::this_thread::yield();
                }

                // LOGD("push: %ld", total_push_count.load());
            }

            w_threads.fetch_add(1, std::memory_order_relaxed);
        }
        else if (tid & 1)
        {
            // LOGD("Consumer: %s, cpu: %d", tll::util::str_tid().data(), sched_getcpu());
            size_t pop_size=0;
            for(;w_threads.load(std::memory_order_relaxed) < prod_num /*- (prod_num + 1) / 2*/
                || total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
            {
#if (defined PERF_TUNNEL) && (PERF_TUNNEL > 0)
                bool ps = cq.deQueue([](size_t, size_t){ NOP_LOOP(PERF_TUNNEL); });
#else
                bool ps = cq.deQueue([&cq, &store_buff, &pop_size, tid](size_t id, size_t size) {
                    auto dst = store_buff[tid/2].data() + pop_size;
                    std::vector<char> *src = cq.elemAt(id);
                    memcpy(dst, src->data(), src->size());
                    pop_size += src->size();
                    // LOGD("< %ld %ld", id, src->size());
                });
#endif
                if(ps)
                {
                    total_pop_count.fetch_add(1, std::memory_order_relaxed);
                }
                else
                {
                    /// underrun
                    std::this_thread::yield();
                }
                // LOGD("pop: %ld", total_pop_count.load());
            }
        }
        // LOGD("          %d Done", tid);
    }
    counter.elapsed();
#ifdef DUMPER
    dump_stat.join();
#endif
    // LOGD("%s", cq.dump().data());
// #if (!defined PERF_TUNNEL) || (PERF_TUNNEL==0)
    dumpStat<>(cq.stat(), counter.lastElapsed().count());
// #endif

    if(opss) {
        opss[0] = cq.stat().push_total;
        opss[1] = cq.stat().pop_total;
    }

    // tll::util::dump(cq.buffer_.data(), cq.buffer_.size());
    // for(int i=0; i<prod_num; i++)
    // {
    //     LOGD("%d", i);
    //     tll::util::dump(store_buff[i].data(), store_buff[i].size(), 0, -1, false);
    // }


    if(total_push_count.load(std::memory_order_relaxed) != total_pop_count.load(std::memory_order_relaxed))
    {
        tll::cc::Stat stat = cq.stat();
        // printf("\n");
        printf(" - w:%ld r:%ld\n", total_push_count.load(std::memory_order_relaxed), total_pop_count.load(std::memory_order_relaxed));
        printf(" - w:%ld r:%ld\n", stat.push_size, stat.pop_size);
        return false;
    }
    bool ret = true;
#if (!defined PERF_TUNNEL) || (PERF_TUNNEL==0)
    // size_t tt_size=0;
    for(int t=0; t<prod_num; t++)
    {
        int index;
        if(!verifyWithTemplate(index, store_buff[t], temp_data))
        {
            printf(" cons: %d", t);
            tll::util::dump(store_buff[t].data(), store_buff[t].size(),0,index);
            ret = false;
        }
    }
#endif
    // printf("\n");

    return ret;
}

template <int prod_num, class CCB>
void perfTunnelCCB(size_t write_count, tll::time::Counter<> &counter, size_t *opss=nullptr)
{
    constexpr int kThreadNum = prod_num;
    CCB fifo{write_count * 2};
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
        opss[0] = fifo.stat().push_total;
        opss[1] = fifo.stat().pop_total;
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
void perfTunnelCQ(size_t write_count, tll::time::Counter<> &counter, size_t *opss=nullptr)
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
        opss[0] = fifo.stat().push_total;
        opss[1] = fifo.stat().pop_total;
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

template<int N> void f(size_t write_count, auto &counter)
{
    LOGD("Number Of Threads: %d", N);
    // {
    //     constexpr size_t kN2 = N;
    //     constexpr size_t kTN = (tll::util::isPowerOf2(kN2) ? kN2 : tll::util::nextPowerOf2(kN2));
    //     LOGD("thread size: %ld", kTN);
    //     perfTunnelCQ<N, tll::lf::CCFIFO<char, kTN>>(write_count, counter);
    // }
    // {
    //     constexpr size_t kN2 = N * 8;
    //     constexpr size_t kTN = (tll::util::isPowerOf2(kN2) ? kN2 : tll::util::nextPowerOf2(kN2));
    //     LOGD("thread size: %ld", kTN);
    //     perfTunnelCQ<N, tll::lf::CCFIFO<char, kTN>>(write_count, counter);
    // }
    // {
    //     constexpr size_t kN2 = N * 64;
    //     constexpr size_t kTN = (tll::util::isPowerOf2(kN2) ? kN2 : tll::util::nextPowerOf2(kN2));
    //     LOGD("thread size: %ld", kTN);
    //     perfTunnelCQ<N, tll::lf::CCFIFO<char, kTN>>(write_count, counter);
    // }
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

void performanceTunnel()
{
    tll::time::Counter<> counter;
    constexpr size_t kCount = 1000000;
    const int cores = std::thread::hardware_concurrency();
    magic<1, PROC_CNT>( [&](auto x)
    {
        LOGD("Number Of Threads: %d", x.value);
        perfTunnelCCB<x.value, tll::lf::CCFIFO<char>>(kCount, counter);
        LOGD("=========================");
    });

    magic<1, PROC_CNT * 16>( [&](auto x)
    {
        LOGD("Number Of Threads: %d", x.value);
        constexpr size_t kN2 = x.value * 512;
        constexpr size_t kTN = (tll::util::isPowerOf2(kN2) ? kN2 : tll::util::nextPowerOf2(kN2));
        LOGD("thread size: %ld", kTN);
        perfTunnelCQ<x.value, tll::lf::CCFIFO<char, kTN>>(kCount, counter);
        LOGD("=========================");
    });
}

int main(int argc, char **argv)
{
    // size_t write_count = 0x400;
    // if(argc > 1) write_count = std::stoul(argv[1]);
    tll::time::Counter<> counter;
    size_t opss[2];

    bool rs = false;
    // rs = testTimer();
    // LOGD("testTimer: %s", rs?"Passed":"FAILED");

    // rs = testGuard();
    // LOGD("testGuard: %s", rs?"Passed":"FAILED");

    rs = testCCB<2, tll::lf::CCFIFO<char>>("lf", 0x1000, 0x100000, counter, opss);
    printf("testCCB: %s\n", rs?"Passed":"FAILED");

    rs = testCQ<2, tll::lf::CCFIFO< std::vector<char> >>(0x1000, opss[0], counter);
    printf("testCQ: %s\n", rs?"Passed":"FAILED");

    performanceTunnel();
    return rs ? 0 : 1;
}