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
        uint32_t delta = time_lst().restart().count();
        total_delta+=delta;
        if(total_delta >= kPeriodMs)
        {
            total_delta = total_delta%kPeriodMs;
            count++;
        }

        uint32_t sleep_ms = kPeriodMs - total_delta;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    total_delta = time_lst("all").stop().count();
    LOGD("total: %d", total_delta);
    LOGD("count: %d", count);
    return count>(kLoop*0.9) && count<(kLoop*1.1);
}

int main()
{
    bool rs = testTimer();
    LOGD("testTimer: %s", rs?"Passed":"FAILED");
    return 0;
}