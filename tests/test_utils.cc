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
            int nts = omp_get_num_threads();
            assert(nts > 1);
            if((tid & 1) == 0)
            {
                // LOGD("Producer: %d", tid);
                auto &timer_w = timer(ccb_type + std::to_string(tid));
                timer_w.start();
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
                timer_w.stop();
                // LOGD("Prod %d Done", tid);
            }
            else
            {
                size_t total_size = rd.size()/(nts/2);
                size_t offset = ((tid)/2)*total_size;
                // LOGD("Consumer: %d, total: %ld, offset: %ld", tid, total_size, offset);
                /// consumer
                auto &timer_r = timer(ccb_type + std::to_string(tid));
                timer_r.start();
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
                timer_r.stop();
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
    bool mt_rs = true;
    bool lf_rs = true;
    printf("Thread Num: %d\n", thread_num);
    double mt_tt_time = 0;
    double lf_tt_time = 0;
    for(int i=shift_init; i<shift_max; i++)
    {
        const size_t kSize = 0x100 << i;
        timer("mt").start();
        mt_rs = __testCCB<tll::mt::CCBuffer, thread_num>(kSize, timer, "mt");
        timer("mt").stop();
        timer("lf").start();
        lf_rs = __testCCB<tll::lf::CCBuffer, thread_num>(kSize, timer, "lf");
        timer("lf").stop();
        if(!mt_rs || !lf_rs) return false;
        mt_tt_time += timer("mt").duration().count();
        lf_tt_time += timer("lf").duration().count();
        printf(" size: %ld (0x%lx) mt: %.9f, lf: %.9f (%.3f)\n", kSize * thread_num, kSize * thread_num, timer("mt").duration().count(), timer("lf").duration().count(), timer("mt").duration().count() / timer("lf").duration().count());

        double mt_time_push = 0;
        double mt_time_pop = 0;
        double lf_time_push = 0;
        double lf_time_pop = 0;
        for(int c=0; c<thread_num; c++)
        {
            if(c&1)
            {
                mt_time_pop += timer("mt" + std::to_string(c)).total().count();
                lf_time_pop += timer("lf" + std::to_string(c)).total().count();
            }
            else
            {
                mt_time_push += timer("mt" + std::to_string(c)).total().count();
                lf_time_push += timer("lf" + std::to_string(c)).total().count();
            }
        }
        printf(" - mt push: %.9f, pop: %.9f (%.3f)\n", mt_time_push, mt_time_pop, mt_time_push/mt_time_pop);
        printf(" - lf push: %.9f, pop: %.9f (%.3f)\n", lf_time_push, lf_time_pop, lf_time_push/lf_time_pop);
    }
    printf(" Total: mt: %.9f, lf: %.9f (%.3f)\n\n", mt_tt_time, lf_tt_time, mt_tt_time / lf_tt_time);

    return true;
}

bool testCCB()
{

    constexpr int kInitShift = 8;
    constexpr int kShift = 10;
    tll::time::Map<> timer{"lf", "mt"};
    if(_testCCB<2>(kInitShift, kShift, timer) == false)
        return false;
    if(_testCCB<4>(kInitShift, kShift, timer) == false)
        return false;
    if(_testCCB<6>(kInitShift, kShift, timer) == false)
        return false;
    if(_testCCB<8>(kInitShift, kShift, timer) == false)
        return false;

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
    return 0;
}