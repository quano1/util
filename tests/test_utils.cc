#include <cmath>
#include "../libs/timer.h"
#include "../libs/util.h"
#include "../libs/log.h"

bool testTimer()
{
    TLL_GLOGTF();
    typedef tll::time::Counter<std::chrono::duration<uint32_t, std::ratio<1, 1000000>>> UsCounter;
    // tll::time::Map<std::chrono::duration<uint32_t, std::ratio<1, 1000>>> timer{{"all"}};
    tll::time::Map<UsCounter> timer{{"all"}};
    timer("all").start();

    uint32_t total_delta = 0;
    constexpr uint32_t kPeriodMs = 50000;
    constexpr int kLoop = 10;
    for (int i=0; i<kLoop; i++)
    {
        uint32_t delta = timer().restart().duration().count();
        total_delta+=delta;
        if(total_delta >= kPeriodMs)
        {
            total_delta = total_delta%kPeriodMs;
        }

        uint32_t sleep_ms = kPeriodMs - total_delta;
        LOGD("sleep: %d (us)", sleep_ms);
        std::this_thread::sleep_for(std::chrono::microseconds(sleep_ms));
    }

    // total_delta = timer("all").stop().duration().count();
    LOGD("total default: %d (us)", timer().stop().total().count());
    LOGD("total all: %d (us)", timer("all").stop().total().count());

    return timer("all").total().count() / 10000 == timer().total().count() / 10000;
}

bool testGuard()
{
    TLL_GLOGTF();
    // tll::time::Map<tll::time::Map<>::Duration, std::chrono::system_clock> timer{};
    typedef tll::time::Counter<tll::time::Counter<>::Duration, std::chrono::system_clock> SystemCounter;
    tll::time::Map<SystemCounter> timer{};
    {
        tll::util::Guard<SystemCounter> time_guard{timer()};
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
    }

    LOGD("total: %f", timer().total().count());
    LOGD("last: %f", timer().duration().count());
    LOGD("elapse: %f", timer().elapse().count());

    {
        timer().start();
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
        timer().stop();
    }

    LOGD("total: %f", timer().total().count());
    LOGD("last: %f", timer().duration().count());
    LOGD("elapse: %f", timer().elapse().count());

    return std::round(timer().total().count()) == std::round(tracer__.timer().elapse().count()) && std::round(timer().duration(0).count()) == std::round(timer().duration(1).count());
}

bool testContiRB()
{
    // TLL_GLOGTF();
    constexpr int kSize = 8;
    tll::util::ContiRB crb(kSize);
    
    bool ret = true;
    {
        // size_t size;
        // std::vector<char> rd(kSize);
        // // #pragma omp parallel num_threads ( 4 )
        // for(int pop_size=1; pop_size<kSize; pop_size++)
        // {
        //     std::vector<char> wt(kSize);
        //     memset(wt.data(), 0xF, kSize);
        //     LOGD("pop_size: %d", pop_size);
        //     crb.reset();
        //     for(int i=0; i<kSize+1; i++)
        //     {
        //         size = pop_size;
        //         size = crb.push(wt.data(), size);
        //         size = crb.pop(rd.data(), size);
        //         crb.dump();
        //     }
        // }
    }

    // #pragma omp parallel num_threads ( 4 )
    constexpr int kSize2 = 4096;
    int push_val = 0xAABBCCDD;
    size_t push_size = sizeof(push_val);
    crb.reset(kSize2);
    std::vector<char> wt(kSize2 * push_size);
    std::vector<char> rd(kSize2 * push_size);
    // memset(rd.data(), 0, rd.size());
    #pragma omp parallel num_threads ( 8 ) shared(crb, wt, rd, push_val, push_size)
    {
        int tid = omp_get_thread_num();
        int nts = omp_get_num_threads();

        if((tid & 1) == 0)
        { 
            LOGD("Thread: %d", tid);
            /// producer
            for(int i=0; i<kSize2/(nts/2) ; i++)
            {
                crb.push((char*)(&push_val), push_size);
            }
        }
        else
        {
            size_t tmp = rd.size()/(nts/2);
            LOGD("Thread: %d: %ld", tid, ((tid/2)*tmp));
            /// consumer
            for(size_t pop_size=0; pop_size<(tmp) ;)
            {
                pop_size += crb.pop(rd.data() + pop_size + ((tid/2)*tmp), push_size);
                std::this_thread::yield();
                // pop_size += rs;
            }
        }
    }

    LOGD("%x", *((int *)(rd.data())));
    LOGD("memcmp: %d", memcmp(rd.data(), rd.data() + push_size, rd.size() - push_size));
    return ret;
}

int main()
{
    bool rs = false;
    // rs = testTimer();
    // LOGD("testTimer: %s", rs?"Passed":"FAILED");

    // rs = testGuard();
    // LOGD("testGuard: %s", rs?"Passed":"FAILED");

    rs = testContiRB();
    LOGD("testContiRB: %s", rs?"Passed":"FAILED");
    return 0;
}