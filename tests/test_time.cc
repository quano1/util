/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
#include <chrono>
#include "../libs/tll.h"

bool testTimer()
{
    using namespace std::chrono;
    // typedef tll::time::Counter<std::chrono::duration<uint32_t, std::ratio<1, 1000000>>> UsCounter;
    // tll::time::Map<std::chrono::duration<uint32_t, std::ratio<1, 1000>>> timer{{"all"}};
    // tll::time::Map<UsCounter> timer{{"all"}};
    tll::time::Counter<> counter1, counter2;

    steady_clock sc;
    const auto tse = sc.now().time_since_epoch();
    const auto begin = sc.now();
    uint32_t total_delta = 0;
    constexpr uint32_t kPeriodUs = 50000;
    constexpr int kLoop = 10;

    counter2.start();
    for (int i=0; i<kLoop; i++)
    {
        LOGD("%.9f", duration_cast<duration<double>>(sc.now() - begin + tse).count());
    }
    counter2.elapsed();

    counter1.start();
    for (int i=0; i<kLoop; i++)
    {
        LOGD("%.9f", duration_cast<duration<double>>(sc.now().time_since_epoch()).count());
    }
    counter1.elapsed();

    // total_delta = timer("all").stop().duration().count();
    LOGD("total: %.9f (s)", counter1.totalElapsed().count());
    LOGD("total: %.9f (s)", counter2.totalElapsed().count());

    return true;
}

int main()
{
    bool rs = false;
    rs = testTimer();
    LOGD("testTimer: %s", rs?"Passed":"FAILED");

    return rs ? 0 : 1;
}