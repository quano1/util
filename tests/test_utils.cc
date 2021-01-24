#include <cmath>
// #include "../libs/timer.h"
// #include "../libs/util.h"
// #include "../libs/log.h"

#include "../libs/tll.h"

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

template <class CCB, int omp_thread_num=2>
bool _testCCB(size_t buff_size = 0x100)
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
            int nts = omp_get_num_threads();
            assert(nts > 1);

            if((tid & 1) == 0)
            {
                // LOGD("Producer: %d", tid);
                for(int i=0; i<buff_size/(nts/2) ; i++)
                {
                    if(ccb.push((char*)(&kPushVal), push_size) == false)
                    {
                        i--;
                    }
                    else
                    {
                        total_w.fetch_add(push_size, std::memory_order_relaxed);
                    }
                    std::this_thread::yield();
                }
                // LOGD("Prod %d Done", tid);
            }
            else
            {
                size_t total_size = rd.size()/(nts/2);
                size_t offset = ((tid)/2)*total_size;
                // LOGD("Consumer: %d, total: %ld, offset: %ld", tid, total_size, offset);
                /// consumer
                for(size_t pop_size=0; pop_size<(total_size) ;)
                {
                    size_t ps = push_size;
                    if(ccb.pop(rd.data() + pop_size + offset, ps))
                    {
                        pop_size += ps;
                        total_r.fetch_add(ps, std::memory_order_relaxed);
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

bool testCCB()
{
    bool mt_rs = true;
    bool lf_rs = true;
    constexpr int kShift = 8;
    tll::time::Map<> timer{"lf", "mt"};
    {
        constexpr int kThreadNum = 2;
        LOGD("Thread Num == %d", kThreadNum);
        double mt_tt_time = 0;
        double lf_tt_time = 0;
        for(int i=0; i<kShift; i++)
        {
            const size_t kSize = 0x100 << i;
            timer("mt").start();
            mt_rs = _testCCB<tll::mt::CCBuffer, kThreadNum>(kSize);
            timer("mt").stop();
            timer("lf").start();
            lf_rs = _testCCB<tll::lf::CCBuffer, kThreadNum>(kSize);
            timer("lf").stop();
            mt_tt_time += timer("mt").duration().count();
            lf_tt_time += timer("lf").duration().count();
            LOGD("size: %ld (0x%lx) mt: %.9f, lf: %.9f", kSize * kThreadNum, kSize * kThreadNum, timer("mt").duration().count(), timer("lf").duration().count());
            if(!mt_rs || !lf_rs) return false;
        }
        LOGD("Total: mt: %.9f, lf: %.9f", mt_tt_time, lf_tt_time);
    }

    {
        constexpr int kThreadNum = 4;
        LOGD("Thread Num == %d", kThreadNum);
        double mt_tt_time = 0;
        double lf_tt_time = 0;
        for(int i=0; i<kShift; i++)
        {
            const size_t kSize = 0x100 << i;
            timer("mt").start();
            mt_rs = _testCCB<tll::mt::CCBuffer, kThreadNum>(kSize);
            timer("mt").stop();
            timer("lf").start();
            lf_rs = _testCCB<tll::lf::CCBuffer, kThreadNum>(kSize);
            timer("lf").stop();
            mt_tt_time += timer("mt").duration().count();
            lf_tt_time += timer("lf").duration().count();
            LOGD("size: %ld (0x%lx) mt: %.9f, lf: %.9f", kSize * kThreadNum, kSize * kThreadNum, timer("mt").duration().count(), timer("lf").duration().count());
            if(!mt_rs || !lf_rs) return false;
        }
        LOGD("Total: mt: %.9f, lf: %.9f", mt_tt_time, lf_tt_time);
    }

    {
        constexpr int kThreadNum = 6;
        LOGD("Thread Num == %d", kThreadNum);
        double mt_tt_time = 0;
        double lf_tt_time = 0;
        for(int i=0; i<kShift; i++)
        {
            const size_t kSize = 0x100 << i;
            timer("mt").start();
            mt_rs = _testCCB<tll::mt::CCBuffer, kThreadNum>(kSize);
            timer("mt").stop();
            timer("lf").start();
            lf_rs = _testCCB<tll::lf::CCBuffer, kThreadNum>(kSize);
            timer("lf").stop();
            mt_tt_time += timer("mt").duration().count();
            lf_tt_time += timer("lf").duration().count();
            LOGD("size: %ld (0x%lx) mt: %.9f, lf: %.9f", kSize * kThreadNum, kSize * kThreadNum, timer("mt").duration().count(), timer("lf").duration().count());
            if(!mt_rs || !lf_rs) return false;
        }
        LOGD("Total: mt: %.9f, lf: %.9f", mt_tt_time, lf_tt_time);
    }

    {
        constexpr int kThreadNum = 8;
        LOGD("Thread Num == %d", kThreadNum);
        double mt_tt_time = 0;
        double lf_tt_time = 0;
        for(int i=0; i<kShift; i++)
        {
            const size_t kSize = 0x100 << i;
            timer("mt").start();
            mt_rs = _testCCB<tll::mt::CCBuffer, kThreadNum>(kSize);
            timer("mt").stop();
            timer("lf").start();
            lf_rs = _testCCB<tll::lf::CCBuffer, kThreadNum>(kSize);
            timer("lf").stop();
            mt_tt_time += timer("mt").duration().count();
            lf_tt_time += timer("lf").duration().count();
            LOGD("size: %ld (0x%lx) mt: %.9f, lf: %.9f", kSize * kThreadNum, kSize * kThreadNum, timer("mt").duration().count(), timer("lf").duration().count());
            if(!mt_rs || !lf_rs) return false;
        }
        LOGD("Total: mt: %.9f, lf: %.9f", mt_tt_time, lf_tt_time);
    }

    int mt_win=0;
    int lf_win=0;
    int draw=0;
    for(int i=0; i<timer("mt").size(); i++)
    {
        if(timer("mt").duration(i) == timer("lf").duration(i)) draw++;
        if(timer("mt").duration(i) < timer("lf").duration(i)) mt_win++;
        if(timer("mt").duration(i) > timer("lf").duration(i)) lf_win++;
    }

    LOGD("Draw: %d, Mutex win: %d, Lock-free win: %d", draw, mt_win, lf_win);
    LOGD("Lock-free is faster %.2f times", (timer("mt").total().count() / timer("lf").total().count()));
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
    LOGD("testCCB: %s", rs?"Passed":"FAILED");
    return 0;
}