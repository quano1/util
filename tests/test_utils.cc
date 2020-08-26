#include <cmath>
#include "../libs/logger.h"
#include "../libs/utils.h"

bool testTimer()
{
    TLL_GLOGTF();
    tll::time::List<std::chrono::duration<uint32_t, std::ratio<1, 1000>>> time_lst{{"all"}};
    uint32_t total_delta = 0;
    constexpr uint32_t kPeriodMs = 5;
    constexpr int kLoop = 100;
    int count=0;
    for (int i=0; i<kLoop; i++)
    {
        uint32_t delta = time_lst().restart().last().count();
        total_delta+=delta;
        if(total_delta >= kPeriodMs)
        {
            total_delta = total_delta%kPeriodMs;
            count++;
        }

        uint32_t sleep_ms = kPeriodMs - total_delta;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    total_delta = time_lst("all").stop().last().count();
    LOGD("total: %d", total_delta);
    LOGD("total2: %d", time_lst("all").total().count());
    LOGD("count: %d", count);
    // std::chrono::duration<double, std::ratio<1>>
    return std::round(std::chrono::duration_cast<tll::time::Counter<>::Duration>(time_lst("all").total()).count()) == std::round(tracer__.timer().elapse().count());
}

bool testGuard()
{
    TLL_GLOGTF();
    tll::time::List<tll::time::List<>::Duration, std::chrono::system_clock> time_lst{};
    {
        tll::util::Guard time_guard{time_lst()};
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
    }

    LOGD("total: %f", time_lst().total().count());
    LOGD("last: %f", time_lst().last().count());
    LOGD("elapse: %f", time_lst().elapse().count());

    {
        time_lst().start();
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
        time_lst().stop();
    }

    LOGD("total: %f", time_lst().total().count());
    LOGD("last: %f", time_lst().last().count());
    LOGD("elapse: %f", time_lst().elapse().count());

    return std::round(time_lst().total().count()) == std::round(tracer__.timer().elapse().count()) && std::round(time_lst().period(0).count()) == std::round(time_lst().period(1).count());
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
    // rs = testTimer();
    // LOGD("testTimer: %s", rs?"Passed":"FAILED");
    rs = testGuard();
    LOGD("testGuard: %s", rs?"Passed":"FAILED");

    rs = testContiRB();
    LOGD("testContiRB: %s", rs?"Passed":"FAILED");
    return 0;
}