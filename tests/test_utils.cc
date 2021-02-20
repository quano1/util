/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
// #include "../libs/timer.h"
// #include "../libs/util.h"
// #include "../libs/log.h"

#define ENABLE_PROFILING 1
#define PERF_TUNNEL 1

#define NOP_LOOP(loop) for(int i__=0; i__<loop; i__++) __asm__("nop")

#include "../libs/tll.h"

void dumpStat(const tll::cc::Stat &st, double real_total_time)
{
    using namespace std::chrono;

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

    printf("        count(K) | err(%%)|miss(%%)| try(%%)|comp(%%)| cb(%%) | all(%%)| ops/ms\n");
    printf(" push: %9.3f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.f\n",
           push_total, push_error_rate, push_miss_rate
           , time_push_try_rate, time_push_complete_rate, time_push_callback_rate, time_push_all_rate
           // avg_time_push_one, avg_push_speed
           , opss
           );

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
    opss = st.pop_total * 0.001f / real_total_time;
    printf(" pop : %9.3f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %5.2f | %.f\n",
           pop_total, pop_error_rate, pop_miss_rate
           , time_pop_try_rate, time_pop_complete_rate, time_pop_callback_rate, time_pop_all_rate
           // avg_time_pop_one
           , opss
           );
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
bool _testCCB(const std::string &ccb_type, size_t ccb_size, size_t write_size, tll::time::Counter<> &counter, size_t *opss=nullptr)
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
        store_buff[i].resize(write_size + 32 * 2); /// temp_data
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
            tll::cc::Callback push_cb;
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
            tll::cc::Callback pop_cb;
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

    printf("CC type: %s\n", ccb_type.data());
    // ccb.dumpStat();
    dumpStat(ccb.stat(), counter.lastElapsed().count());
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

bool _testCCB2()
{
    bool ret = true;
    return ret;
}

bool testCCB()
{

    // constexpr int kInitShift = 0;
    // constexpr int kShift = 1;
    constexpr size_t kOneMb = 1024 * 1024;
    constexpr size_t kOneGb = kOneMb * 1024;
    // size_t write_size = kOneMb * 100;
    // size_t ccb_size = write_size / 8;
    /// 1Mb, 10Mb, 100Mb, 1Gb
    // for(; ccb_size < write_size * 3; ccb_size = ccb_size << 1)
    // for(;write_size <= kOneMb * 1000; kOneMb)
    // int i = 1;
    std::ofstream ofs{"run.dat"};
    size_t opss[2];
#if (defined PERF_TUNNEL) && (PERF_TUNNEL > 0)
    // for(int i=1; i<=0x1000; i*=2)
    int i = 1;
#else
    for(int i=1; i<=128; i*=2)
#endif
    {
        size_t write_size = kOneMb * i;
        size_t const ccb_size = write_size / 8;

        printf("ccb_size: %.3fMb(0x%lx), write_size: %.3fGb(0x%lx)\n", ccb_size*1.f/kOneMb, ccb_size, write_size*1.f/kOneGb, write_size);
        printf("================================================================\n");
        // printf("----------------------------------------------------------------\n");
        printf("threads: 1\n");
        tll::time::Counter<> counter;
        ofs << 1 << " ";
        // counter.start();
        // if(!_testCCB<1, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
        // ofs << tll::util::stringFormat("%.6f ", counter.lastElapsed().count());
        // printf("-------------------------------\n");
        if(!_testCCB<1, tll::lf::CCFIFO<char>>("lf", ccb_size, write_size, counter, opss)) return false;
        printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
        // ofs << tll::util::stringFormat("%.6f ", counter.lastElapsed().count());
        ofs << opss[0] * 0.001f / counter.lastElapsed().count() << " " << opss[1] * 0.001f / counter.lastElapsed().count();
        printf("-------------------------------\n");
        ofs << std::endl;

        ofs << 2 << " ";
        printf("----------------------------------------------------------------\n");
        printf("threads: 2\n");
        // if(!_testCCB<2, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
        // printf("-------------------------------\n");
        if(!_testCCB<2, tll::lf::CCFIFO<char>>("lf", ccb_size, write_size, counter, opss)) return false;
        printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
        // ofs << tll::util::stringFormat("%.6f ", counter.lastElapsed().count());
        ofs << opss[0] * 0.001f / counter.lastElapsed().count() << " " << opss[1] * 0.001f / counter.lastElapsed().count();
        printf("-------------------------------\n");
        ofs << std::endl;

        // ofs << 3 << " ";
        // printf("----------------------------------------------------------------\n");
        // printf("threads: 3\n");
        // if(!_testCCB<3, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
        // printf("-------------------------------\n");
        // if(!_testCCB<3, tll::lf::CCFIFO<char>>("lf", ccb_size, write_size, counter, opss)) return false;
        // printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
        // // ofs << tll::util::stringFormat("%.6f ", counter.lastElapsed().count());
        // ofs << opss[0] * 0.001f / counter.lastElapsed().count() << " " << opss[1] * 0.001f / counter.lastElapsed().count();
        // printf("-------------------------------\n");
        // ofs << std::endl;

        // printf("----------------------------------------------------------------\n");
        // printf("threads: 4\n");
        // if(!_testCCB<4, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
        // printf("-------------------------------\n");
        // if(!_testCCB<4, tll::lf::CCFIFO<char>>("lf", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f(s)\n", counter.lastElapsed().count());
        // ofs << tll::util::stringFormat("%.6f ", counter.lastElapsed().count());
        // printf("-------------------------------\n");

        printf("Total duration: %.6f(s)\n", counter.totalElapsed().count());
        printf("================================================================\n");
        printf("\n");
        ofs << std::endl;
    }

    return true;
}

int main()
{
    bool rs = false;
    // rs = testTimer();
    // LOGD("testTimer: %s", rs?"Passed":"FAILED");

    // rs = testGuard();
    // LOGD("testGuard: %s", rs?"Passed":"FAILED");


    rs = testCCB();
    printf("testCCB: %s\n", rs?"Passed":"FAILED");
    return rs ? 0 : 1;
}