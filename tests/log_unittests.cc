/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#include <gtest/gtest.h>
#include <omp.h>

#include <tll.h>

using namespace tll::log;

struct LoggerTest : public ::testing::Test
{
    void SetUp()
    {
        Manager::instance().start();
    }

    void TearDown()
    {
        Manager::instance().stop();
    }
};

// TEST_F(LoggerTest, Simple)
// {
//     tll::util::Counter<> counter;
//     TLL_GLOGD2("");
//     LOGD("%f", counter.elapse().count());
//     Manager::instance().stop();

//     Manager::instance().start(1);
//     counter.start();
//     TLL_GLOGD2("");
//     LOGD("%f", counter.elapse().count());
//     Manager::instance().stop();

//     Manager::instance().remove("file");
//     Manager::instance().add(Entity{
//                 .name = "printf",
//                 [this](void *handle, bool, const char *buff, size_t size)
//                 {
//                     printf("%.*s", (int)size, buff);
//                 }
//             });
//     Manager::instance().start();
//     counter.start();
//     TLL_GLOGD2("");
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     LOGD("%f", counter.elapse().count());
//     Manager::instance().stop();
// }

// TEST_F(LoggerTest, SequenceLog)
// {
//     tll::util::Counter<> counter;
//     double time = 0;
//     counter.start();
//     {
//         for(int i=0; i<1000000; i++)
//         {
//             __asm__("nop");
//             TLL_GLOGD2("");
//         }
//     }
//     time = counter.elapse().count();
//     LOGD("%.3f:%f", (1)/(time), time);

//     counter.start();
//     {
//         for(int i=0; i<1000000; i++)
//         {
//             // TLL_GLOGTF();
//             __asm__("nop");
//         }
//     }
//     time = counter.elapse().count();
//     LOGD("%.3f:%f", (1)/(time), time);
// }

// TEST_F(LoggerTest, ConcurrentLog)
// {
//     double time = 0;
//     tll::util::Counter<> counter;
//     counter.start();
//     #pragma omp parallel num_threads ( NUM_CPU )
//     {
//         for(int i=0; i<1000000; i++)
//         {
//             __asm__("nop");
//             TLL_GLOGD2("");
//         }
//     }
//     time = counter.elapse().count();
//     LOGD("%.3f:%f", (NUM_CPU)/(time), time);

//     counter.start();
//     #pragma omp parallel num_threads ( NUM_CPU )
//     {
//         for(int i=0; i<1000000; i++)
//         {
//             // TLL_GLOGTF();
//             __asm__("nop");
//         }
//     }
//     time = counter.elapse().count();
//     LOGD("%.3f:%f", (NUM_CPU)/(time), time);
// }

// void logfunction(char *dst, char src)
// {
//     memcpy(dst, &src, 1);
// }

// TEST_F(LoggerTest, NanoLog)
// {
//     char val = 0;
//     // constexpr char src = 0xFF;
//     tll::util::Counter<> counter;
//     for(int i=0; i<100000; i++) logfunction(&val, 0xFF);

//     LOGD("%.9f: %1x", counter.elapse().count(), val);
//     // LOGV(counter.elapse().count(), (uint8_t)val);
// }


TEST_F(LoggerTest, Log2)
{
    static auto &ins = tll::log::Manager::instance();
    tll::util::Counter<> counter;
    double finish_log_time, total_log_time;

    {
        ins.total_size = 0;
        ins.start();
        std::this_thread::sleep_for(std::chrono::nanoseconds(0));
        // std::this_thread::yield();
        counter.start();
        for(int i=0;i<(int)1e5;i++)
        {
            TLL_GLOGTF2();
            TLL_GLOGT2(haha);
            TLL_GLOGD2("");
            TLL_GLOGD2("%.9f", counter.elapse().count());
            TLL_GLOGI2("%.9f", counter.elapse().count());
            TLL_GLOGW2("%.9f", counter.elapse().count());
            TLL_GLOGF2("%.9f", counter.elapse().count());
        }
        finish_log_time = counter.elapse().count();
        // std::this_thread::sleep_for(std::chrono::seconds(10));
        ins.stop();
        total_log_time = counter.elapse().count();
        LOGD("%ld\t%.9f\t%.9f", ins.total_count, finish_log_time, finish_log_time / ins.total_count);
        LOGD("In:Out speed: %.3f / %.3f Mbs", ins.total_size / finish_log_time / 0x100000, ins.total_size / total_log_time / 0x100000);
    }

    // {
    //     ins.total_size = 0;
    //     ins.start();
    //     counter.start();
    //     for(int i=0;i<(int)1e6;i++)
    //     {
    //         // TLL_GLOGD2("%.9f", counter.elapse().count());
    //         TLL_GLOG(1, "%.9f", counter.elapse().count());
    //     }
    //     LOGD("%.9f", counter.elapse().count() * 1e-6);
    //     ins.stop();
    // }
    // LOGD("Out speed: %.3f Mbs", ins.total_size / counter.elapse().count() / 0x100000);

    // tll::log::Manager::instance().start();
    // counter.start();
    // #pragma omp parallel num_threads ( NUM_CPU )
    // {
    //     for(int i=0;i<1000000;i++)
    //         TLL_GLOGD2("%f", counter.elapse().count());
    // }
    // LOGD("%.9f", counter.elapse().count() * 1e-6 / NUM_CPU);
    // // tll::log::Manager::instance().log2<Mode::kAsync>((tll::log::Type::kDebug), __FILE__, __FUNCTION__, __LINE__, "");

    // tll::log::Manager::instance().stop();
}