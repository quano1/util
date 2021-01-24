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
    // constexpr int kSize = 8;
    constexpr int kSize = 0x100000;
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

    constexpr size_t push_val = 0xaaaaaaaaaaaaaaaau;
    // std::vector<char> wt(kSize * push_size);
    std::vector<char> rd;
    for(size_t push_size=2; push_size<=8; push_size+=2) /// 2 4 6 8
    {
        rd.resize(kSize * push_size);
        crb.reset(kSize);
        memset(rd.data(), 0xff, rd.size());
        LOGD("push_size: %ld", push_size);
        LOGD("crb size: %ld, rd size: %ld", crb.buffer_.size(), rd.size());

        #pragma omp parallel num_threads ( 8 ) shared(crb, rd, push_size)
        {
            int tid = omp_get_thread_num();
            int nts = omp_get_num_threads();
            assert(nts > 1);

            if((tid & 1) == 0)
            { 
                LOGD("Producer: %d", tid);
                for(int i=0; i<kSize/(nts/2) ; i++)
                {
                    if(crb.push((char*)(&push_val), push_size) == false)
                    {
                        // LOGD("OVER...RUN");
                        i--;
                    }
                    std::this_thread::yield();
                }
            }
            else
            {
                size_t total_size = rd.size()/(nts/2);
                size_t offset = ((tid)/2)*total_size;
                LOGD("Consumer: %d, total: %ld, offset: %ld", tid, total_size, offset);
                /// consumer
                for(size_t pop_size=0; pop_size<(total_size) ;)
                {
                    size_t ps = push_size;
                    if(crb.pop(rd.data() + pop_size + offset, ps))
                    {
                        pop_size += ps;
                    }
                    std::this_thread::yield();
                }
            }
        }

        LOGD("rd[0]: %x", *((uint8_t *)(rd.data())));
        int cmp = memcmp(rd.data(), rd.data() + 1, rd.size() - 1);
        LOGD("memcmp: %d", cmp);
        if(cmp && kSize <= 0x100)
        {
            for(int i=0; i<rd.size(); i++)
                printf("%x", *(uint8_t*)&rd[i]);
            printf("\n");
        }
        printf("\n");
    }
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