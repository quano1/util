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
    constexpr int kSize = 16;
    tll::util::ContiRB crb(kSize);
    std::vector<char> wt(kSize);
    std::vector<char> rd(kSize);
    memset(wt.data(), 0xF, kSize);

    size_t size;
    bool ret = true;

    // #pragma omp parallel num_threads ( 4 )
    for(int i=0; i<10; i++)
    {
        LOGD("");
        size = crb.push(wt.data(), 5);
        // LOGD("push: %ld", size);
        crb.dump();
        // LOGD("");
        size = crb.pop(rd.data(), size);
        // LOGD(" - pop: %ld", size);
        crb.dump();
        // if(memcmp(wt.data(), rd.data(), size))
        // {
        //     ret = false;
        //     break;
        // }
    }

    // size = crb.push(wt.data(), 3);
    // LOGD("push: %ld", size);
    // crb.dump();

    // size = crb.pop(rd.data(), size);
    // ret += memcmp(wt.data(), rd.data(), size);
    // LOGD("pop:  %ld, %d", size, ret);
    // crb.dump();

    // size = crb.push(wt.data(), 3);
    // LOGD("push: %ld", size);
    // crb.dump();

    // size = crb.pop(rd.data(), size);
    // ret += memcmp(wt.data(), rd.data(), size);
    // LOGD("pop:  %ld, %d", size, ret);
    // crb.dump();

    // // LOGD("pop:  %ld", size);
    // size = crb.push(wt.data(), 3);
    // LOGD("push: %ld", size);
    // crb.dump();
    
    // size = crb.pop(rd.data(), size);
    // ret += memcmp(wt.data(), rd.data(), size);
    // LOGD("pop:  %ld, %d", size, ret);
    // crb.dump();
    // LOGD("pop:  %ld", size);

    // size = crb.push(wt.data(), 3);
    // LOGD("push: %ld", size);
    // size = crb.pop(rd.data(), size);
    // LOGD("pop:  %ld", size);

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