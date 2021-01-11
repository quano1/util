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
    TLL_GLOGTF();
    constexpr int kSize = 8;
    tll::util::ContiRB contiRB(kSize);
    std::vector<char> wt(kSize);
    std::vector<char> rd(kSize);
    memset(wt.data(), 1, kSize);

    size_t size;

    size = contiRB.push(wt.data(), 3);
    TLL_GLOGD("push: %ld", size);
    size = contiRB.pop(rd.data(), size);
    TLL_GLOGD("pop:  %ld", size);
    size = contiRB.push(wt.data(), 3);
    TLL_GLOGD("push: %ld", size);
    size = contiRB.pop(rd.data(), size);
    TLL_GLOGD("pop:  %ld", size);
    size = contiRB.push(wt.data(), 3);
    TLL_GLOGD("push: %ld", size);
    size = contiRB.pop(rd.data(), size);
    TLL_GLOGD("pop:  %ld", size);

    size = contiRB.push(wt.data(), 3);
    TLL_GLOGD("push: %ld", size);
    size = contiRB.pop(rd.data(), size);
    TLL_GLOGD("pop:  %ld", size);

    return true;
}

int main()
{
    bool rs = false;
    rs = testTimer();
    LOGD("testTimer: %s", rs?"Passed":"FAILED");

    rs = testGuard();
    LOGD("testGuard: %s", rs?"Passed":"FAILED");

    rs = testContiRB();
    LOGD("testContiRB: %s", rs?"Passed":"FAILED");
    return 0;
}