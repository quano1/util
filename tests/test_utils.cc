/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#include <cmath>
// #include "../libs/timer.h"
// #include "../libs/util.h"
// #include "../libs/log.h"

#include "../libs/tll.h"

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

template <class CCB, int omp_thread_num>
bool __testCCB(size_t buff_size,
              tll::time::Map<> &timer, const std::string &ccb_type)
{
    buff_size *= omp_thread_num;
    CCB ccb(buff_size);

    bool ret = true;
    constexpr size_t kPushVal = 0xaaaaaaaaaaaaaaaau;
    std::vector<char> rd;
    for(size_t push_size=1; push_size<=8; push_size++) /// 2 4 6 8
    {
        rd.resize(buff_size * push_size);
        ccb.reset(buff_size);
        memset(rd.data(), 0xff, rd.size());
        // LOGD("push_size: %ld", push_size);
        std::atomic<size_t> total_r{0};
        std::atomic<size_t> total_w{0};

        #pragma omp parallel num_threads ( omp_thread_num ) shared(ccb, rd, push_size)
        {
            int tid = omp_get_thread_num();
            // int nts = omp_get_num_threads();
            assert(omp_thread_num > 1);
            if((tid & 1) == 0)
            {
                // LOGD("Producer: %d", tid);
                auto &timer_w = timer(ccb_type + std::to_string(tid));
                for(int i=0; i<buff_size/(omp_thread_num/2) ; i++)
                {
                    timer_w.start();
                    if(ccb.push((char*)(&kPushVal), push_size) == false)
                    {
                        i--;
                    }
                    else
                    {
                        total_w.fetch_add(push_size, std::memory_order_relaxed);
                        timer_w.elapsed();
                    }
                    std::this_thread::yield();
                }
                // LOGD("Prod %d Done", tid);
            }
            else
            {
                size_t total_size = rd.size()/(omp_thread_num/2);
                size_t offset = ((tid)/2)*total_size;
                // LOGD("Consumer: %d, total: %ld, offset: %ld", tid, total_size, offset);
                /// consumer
                auto &timer_r = timer(ccb_type + std::to_string(tid));
                for(size_t pop_size=0; pop_size<(total_size) ;)
                {
                    size_t ps = push_size;
                    timer_r.start();
                    if(ccb.pop(rd.data() + pop_size + offset, ps))
                    {
                        pop_size += ps;
                        total_r.fetch_add(ps, std::memory_order_relaxed);
                        timer_r.elapsed();
                    }
                    std::this_thread::yield();
                }
                // LOGD("Cons %d Done", tid);
            }
            // LOGD("total_w: %ld", total_w.load(std::memory_order_relaxed));
            // LOGD("total_r: %ld", total_r.load(std::memory_order_relaxed));
        }

        // LOGD("rd[0]: %x", *((uint8_t *)(rd.data())));
        int cmp = memcmp(rd.data(), rd.data() + 1, rd.size() - 1);
        // LOGD("memcmp: %d", cmp);
        if(cmp && buff_size <= 0x100)
        {
            for(int i=0; i<rd.size(); i++)
                printf("%x", *(uint8_t*)&rd[i]);
            printf("\n");
            ret = false;
        }
        // printf("\n");
    }
    return ret;
}

template <int thread_num>
bool _testCCB(int shift_init, int shift_max, tll::time::Map<> &timer)
{
    constexpr int thread_num2 = thread_num * 2;
    bool mt_rs = true;
    bool lf_rs = true;
    printf("Thread Num: %d\n", thread_num2);
    double mt_tt_time = 0;
    double lf_tt_time = 0;
    for(int i=shift_init; i<shift_max; i++)
    {
        const size_t kSize = 0x100 << i;
        timer("mt").start();
        mt_rs = __testCCB<tll::mt::CCBuffer, thread_num2>(kSize, timer, "mt");
        timer("mt").stop();
        timer("lf").start();
        lf_rs = __testCCB<tll::lf::CCBuffer, thread_num2>(kSize, timer, "lf");
        timer("lf").stop();
        if(!mt_rs || !lf_rs) return false;
        mt_tt_time += timer("mt").duration().count();
        lf_tt_time += timer("lf").duration().count();
        printf(" size: %ld (0x%lx) mt: %.9f, lf: %.9f (%.3f)\n", kSize * thread_num2, kSize * thread_num2, timer("mt").duration().count(), timer("lf").duration().count(), timer("mt").duration().count() / timer("lf").duration().count());

        double mt_time_push = 0;
        double mt_time_pop = 0;
        double lf_time_push = 0;
        double lf_time_pop = 0;
        for(int c=0; c<thread_num2; c++)
        {
            if(c&1)
            {
                mt_time_pop += timer("mt" + std::to_string(c)).totalElapsed().count();
                lf_time_pop += timer("lf" + std::to_string(c)).totalElapsed().count();
            }
            else
            {
                mt_time_push += timer("mt" + std::to_string(c)).totalElapsed().count();
                lf_time_push += timer("lf" + std::to_string(c)).totalElapsed().count();
            }
        }
        printf(" - mt push: %.9f, pop: %.9f (%.3f)\n", mt_time_push, mt_time_pop, mt_time_push/mt_time_pop);
        printf(" - lf push: %.9f, pop: %.9f (%.3f)\n", lf_time_push, lf_time_pop, lf_time_push/lf_time_pop);
    }
    printf(" Total: mt: %.9f, lf: %.9f (%.3f)\n\n", mt_tt_time, lf_tt_time, mt_tt_time / lf_tt_time);

    return true;
}

template <class CCB, int omp_thread_num>
bool __testCCB2(size_t buff_size,
              tll::time::Map<> &timer, const std::string &ccb_type)
{

}

template <int thread_num, class CCB>
bool _testCCB2(const std::string &ccb_type, size_t ccb_size, size_t buff_size, tll::time::Map<> &timer)
{
    constexpr int omp_thread_num = thread_num * 2;
    printf("threads: %d, ccb_type: %s, ccb_size: 0x%lx, buff_size: 0x%lx\n", thread_num, ccb_type.data(), ccb_size, buff_size);

    CCB ccb(ccb_size);
    std::vector<char> store_buff[thread_num];
    for(int i=0; i<thread_num; i++)
    {
        store_buff[i].resize(buff_size * omp_thread_num);
        memset(store_buff[i].data(), 0, store_buff[i].size());
        timer(ccb_type + std::to_string(i));
        timer(ccb_type + std::to_string(i+thread_num));
    }

    const std::vector<char> temp_data[] = {
        {1},
        {2,2},
        {3,3,3},
        {4,4,4,4},
        {5,5,5,5,5},
        {6,6,6,6,6,6},
        {7,7,7,7,7,7,7},
        {8,8,8,8,8,8,8,8}
    };

    std::atomic<size_t> total_w{0};
    std::atomic<size_t> total_r{0};
    std::atomic<int> w_threads{0};

    assert(omp_thread_num > 1);
    #pragma omp parallel num_threads ( omp_thread_num ) shared(ccb, store_buff, temp_data)
    {
        int tid = omp_get_thread_num();
        auto &counter = timer(ccb_type + std::to_string(tid));
        if((tid & 1) == 0)
        {
            // LOGD("Producer: %d", tid);
            int i=0;
            for(;total_w.load(std::memory_order_relaxed) < (buff_size);)
            {
                counter.start();
                if(ccb.push(temp_data[i].data(), temp_data[i].size()))
                {
                    counter.elapsed();
                    total_w.fetch_add(temp_data[i].size(), std::memory_order_relaxed);
                    (++i) &= 7;
                }
                else
                {
                    /// overrun
                    std::this_thread::yield();
                }
            }

            w_threads.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            /// consumer
            size_t pop_size=0;
            for(;w_threads.load(std::memory_order_relaxed) < thread_num || total_w.load(std::memory_order_relaxed) > total_r.load(std::memory_order_relaxed);)
            {
                size_t ps = ccb_size;
                counter.start();
                if(ccb.pop(store_buff[tid/2].data() + pop_size, ps))
                {
                    counter.elapsed();
                    pop_size += ps;
                    total_r.fetch_add(ps, std::memory_order_relaxed);
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

    /// threads end
    // for(int i=0; i<thread_num; i++)
    // {
    //     tll::util::dump(store_buff[i].data(), store_buff[i].size());
    // }

    printf(" - w:%ld r:%ld\n", total_w.load(std::memory_order_relaxed), total_r.load(std::memory_order_relaxed));
    if(total_w.load(std::memory_order_relaxed) != total_r.load(std::memory_order_relaxed))
    {
         return false;
    }

    size_t tt_size=0;
    for(int i=0; i<thread_num; i++)
    {
        const auto &sb = store_buff[i];
        int i_=0;
        for(; i_<sb.size();)
        {
            const char val = sb[i_];
            if(val == 0)
            {
                break;
            }
            const auto &td = temp_data[val-1];
            int rs = memcmp(sb.data() + i_, td.data(), val);
            if(rs)
            {
                LOGD("t%d: i:%d v:%d r:%d", i, i_, val, rs);
                for(int t=0; t<thread_num; t++)
                {
                    if(i == t)
                        tll::util::dump(store_buff[t].data(), store_buff[t].size(),0,i_);
                    else
                        tll::util::dump(store_buff[t].data(), store_buff[t].size());
                }
                return false;
            }
            i_+=val;
            tt_size+=val;
        }
    }
    // LOGD("%ld", tt_size);

    double time_pop = 0;
    double time_push = 0;
    for(int c=0; c<omp_thread_num; c++)
    {
        if(c&1)
        {
            time_pop += timer(ccb_type + std::to_string(c)).totalElapsed().count();
        }
        else
        {
            time_push += timer(ccb_type + std::to_string(c)).totalElapsed().count();
        }
    }
    printf(" - push: %.9f / pop: %.9f (%.3f)\n", time_push, time_pop, time_push/time_pop);

    printf(" Total: %.6f\n\n", time_push + time_pop);

    return true;
}

bool testCCB()
{

    constexpr int kInitShift = 0;
    constexpr int kShift = 1;
    tll::time::Map<> timer{"lf", "mt"};
    constexpr size_t ccp_size = 0x80;
    constexpr size_t buf_size = 0x100;
    // if(!_testCCB2<1, tll::mt::CCBuffer>("mt", ccp_size, buf_size, timer)) return false;
    // if(!_testCCB2<1, tll::lf::CCBuffer>("lf", ccp_size, buf_size, timer)) return false;

    if(!_testCCB2<2, tll::mt::CCBuffer>("mt", ccp_size, buf_size, timer)) return false;
    // if(!_testCCB2<2, tll::lf::CCBuffer>("lf", ccp_size, buf_size, timer)) return false;

    // printf("mt/lf: %.3f\n", timer("mt").duration().count() / timer("lf").duration().count());

    // if(!_testCCB2<2, tll::mt::CCBuffer>("mt", ccp_size, buf_size, timer) || !_testCCB2<2, tll::lf::CCBuffer>("lf", ccp_size, buf_size, timer))
    //     return false;
    // printf("mt/lf: %.3f\n", timer("mt").duration().count() / timer("lf").duration().count());

    // if(_testCCB<2>(kInitShift, kShift, timer) == false)
    //     return false;
    // if(_testCCB<3>(kInitShift, kShift, timer) == false)
    //     return false;
    // if(_testCCB<4>(kInitShift, kShift, timer) == false)
    //     return false;
    // if(_testCCB<10>(kInitShift, kShift, timer) == false)
    //     return false;
    // if(_testCCB<12>(kInitShift, kShift, timer) == false)
        // return false;
    
    int mt_win=0;
    int lf_win=0;
    int draw=0;
    for(int i=0; i<timer("mt").size(); i++)
    {
        if(timer("mt").duration(i) == timer("lf").duration(i)) draw++;
        if(timer("mt").duration(i) < timer("lf").duration(i)) mt_win++;
        if(timer("mt").duration(i) > timer("lf").duration(i)) lf_win++;
    }

    printf("Draw: %d, Mutex win: %d, Lock-free win: %d\n", draw, mt_win, lf_win);
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