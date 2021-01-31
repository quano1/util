/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
// #include "../libs/timer.h"
// #include "../libs/util.h"
// #include "../libs/log.h"

#define ENABLE_STAT_TIME 1
#define ENABLE_STAT_COUNTER 1

#include "../libs/tll.h"

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
//     TLL_GLOGTF();
//     typedef tll::time::Counter<std::chrono::duration<uint32_t, std::ratio<1, 1000000>>> UsCounter;
//     // tll::time::Map<std::chrono::duration<uint32_t, std::ratio<1, 1000>>> timer{{"all"}};
//     tll::time::Map<UsCounter> timer{{"all"}};
//     timer("all").start();

//     uint32_t total_delta = 0;
//     constexpr uint32_t kPeriodMs = 50000;
//     constexpr int kLoop = 10;
//     for (int i=0; i<kLoop; i++)
//     {
//         uint32_t delta = timer().restart().duration().count();
//         total_delta+=delta;
//         if(total_delta >= kPeriodMs)
//         {
//             total_delta = total_delta%kPeriodMs;
//         }

//         uint32_t sleep_ms = kPeriodMs - total_delta;
//         LOGD("sleep: %d (us)", sleep_ms);
//         std::this_thread::sleep_for(std::chrono::microseconds(sleep_ms));
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
bool _testCCB(const std::string &ccb_type, size_t ccb_size, size_t write_size, tll::time::Counter<> &counter, bool verify=true)
{
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

    constexpr int omp_thread_num = thread_num * 2;

    CCB ccb(ccb_size);
    std::vector<char> store_buff[thread_num];
    for(int i=0; i<thread_num; i++)
    {
        store_buff[i].resize(write_size + 32 + 2); /// temp_data
        memset(store_buff[i].data(), 0, store_buff[i].size());
    }

    std::atomic<int> w_threads{0};

    assert(omp_thread_num > 1);
    counter.start();
    #pragma omp parallel num_threads ( omp_thread_num ) shared(ccb, store_buff, temp_data)
    {
        int tid = omp_get_thread_num();
        if(!(tid & 1))
        // if(tid == 0)
        {
            // LOGD("Producer: %d", tid);
            int i=0;
            for(;ccb.stat().push_size < (write_size);)
            {
                if(ccb.push(temp_data[i].data(), temp_data[i].size()))
                {
                    // (++i) &= 7;
                }
                else
                {
                    /// overrun
                    std::this_thread::yield();
                }
                (++i) &= 7;
            }

            w_threads.fetch_add(1, std::memory_order_relaxed);
        }
        else if (tid & 1)
        {
            // LOGD("Consumer: %d(%d)", tid, tid/2);
            size_t pop_size=0;
            for(;w_threads.load(std::memory_order_relaxed) < thread_num /*- (thread_num + 1) / 2*/
                || ccb.stat().push_size > ccb.stat().pop_size;)
            {
                size_t ps = ccb_size;
                if(ccb.pop(store_buff[tid/2].data() + pop_size, ps))
                {
                    pop_size += ps;
                    // store_buff[tid/2][pop_size] = '|';
                    // pop_size++;
                    // LOGD("%ld", pop_size);
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

    tll::StatCCI stat = ccb.stat();
    printf("CC type: %s\n", ccb_type.data());
    ccb.dumpStat();

    /// no verification
    if(!verify) return true;

    // tll::util::dump(ccb.buffer_.data(), ccb.buffer_.size());
    // for(int i=0; i<thread_num; i++)
    // {
    //     LOGD("%d", i);
    //     tll::util::dump(store_buff[i].data(), store_buff[i].size(), 0, -1, false);
    // }

    // printf(" - w:%ld r:%ld\n", total_w.load(std::memory_order_relaxed), total_r.load(std::memory_order_relaxed));
    if(stat.push_size != stat.pop_size)
    {
        // printf("\n");
        return false;
    }

    // size_t tt_size=0;
    bool ret = true;
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

    constexpr int kInitShift = 0;
    constexpr int kShift = 1;
    
    size_t write_size = 0x100000;
    size_t ccb_size = write_size / 4;
    // for(; ccb_size < write_size * 3; ccb_size = ccb_size << 1)
    {
        printf("ccb_size: %.3fMb(0x%lx), write_size: %.3fMb(0x%lx)\n", ccb_size*1.f/0x100000, ccb_size, write_size*1.f/0x100000, write_size);
        printf("================================================================\n");
        // printf("----------------------------------------------------------------\n");
        printf("threads: 1\n");
        tll::time::Counter<> counter;
        // counter.start();
        
        if(!_testCCB<1, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter)) return false;
        printf(" Test duration: %.6f\n", counter.lastElapsed().count());
        printf("-------------------------------\n");
        if(!_testCCB<1, tll::lf::GCCC<char>>("lf", ccb_size, write_size, counter)) return false;
        printf(" Test duration: %.6f\n", counter.lastElapsed().count());
        printf("-------------------------------\n");
        
        printf("----------------------------------------------------------------\n");
        printf("threads: 2\n");
        if(!_testCCB<2, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter)) return false;
        printf(" Test duration: %.6f\n", counter.lastElapsed().count());
        printf("-------------------------------\n");
        if(!_testCCB<2, tll::lf::GCCC<char>>("lf", ccb_size, write_size, counter)) return false;
        printf(" Test duration: %.6f\n", counter.lastElapsed().count());
        printf("-------------------------------\n");
        
        // printf("----------------------------------------------------------------\n");
        // printf("threads: 3\n");
        // if(!_testCCB<3, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f\n", counter.lastElapsed().count());
        // printf("-------------------------------\n");
        // if(!_testCCB<3, tll::lf::GCCC<char>>("lf", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f\n", counter.lastElapsed().count());
        // printf("-------------------------------\n");
        
        // printf("----------------------------------------------------------------\n");
        // printf("threads: 4\n");
        // if(!_testCCB<4, tll::mt::CCBuffer>("mt", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f\n", counter.lastElapsed().count());
        // printf("-------------------------------\n");
        // if(!_testCCB<4, tll::lf::GCCC<char>>("lf", ccb_size, write_size, counter)) return false;
        // printf(" Test duration: %.6f\n", counter.lastElapsed().count());
        // printf("-------------------------------\n");

        printf("Total duration: %.6f\n", counter.totalElapsed().count());
        printf("================================================================\n");
        printf("\n");
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