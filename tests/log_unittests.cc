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

TEST_F(LoggerTest, Simple)
{
    tll::util::Counter<> counter;
    TLL_GLOGD("");
    LOGD("%f", counter.elapse().count());
    Manager::instance().stop();

    Manager::instance().start(1);
    counter.start();
    TLL_GLOGD("");
    LOGD("%f", counter.elapse().count());
    Manager::instance().stop();

    Manager::instance().remove("file");
    Manager::instance().add(Entity{
                .name = "printf",
                [this](void *handle, const char *buff, size_t size)
                {
                    printf("%.*s", (int)size, buff);
                }
            });
    Manager::instance().start();
    counter.start();
    TLL_GLOGD("");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    LOGD("%f", counter.elapse().count());
    Manager::instance().stop();
}

TEST_F(LoggerTest, SequenceLog)
{
    tll::util::Counter<> counter;
    double time = 0;
    counter.start();
    {
        for(int i=0; i<1000000; i++)
        {
            __asm__("nop");
            TLL_GLOGD("");
        }
    }
    time = counter.elapse().count();
    LOGD("%.3f:%f", (1)/(time), time);

    counter.start();
    {
        for(int i=0; i<1000000; i++)
        {
            TLL_GLOGTF();
            __asm__("nop");
        }
    }
    time = counter.elapse().count();
    LOGD("%.3f:%f", (1)/(time), time);
}

TEST_F(LoggerTest, ConcurrentLog)
{
    double time = 0;
    tll::util::Counter<> counter;
    counter.start();
    #pragma omp parallel num_threads ( NUM_CPU )
    {
        for(int i=0; i<1000000; i++)
        {
            __asm__("nop");
            TLL_GLOGD("");
        }
    }
    time = counter.elapse().count();
    LOGD("%.3f:%f", (NUM_CPU)/(time), time);

    counter.start();
    #pragma omp parallel num_threads ( NUM_CPU )
    {
        for(int i=0; i<1000000; i++)
        {
            TLL_GLOGTF();
            __asm__("nop");
        }
    }
    time = counter.elapse().count();
    LOGD("%.3f:%f", (NUM_CPU)/(time), time);
}

void logfunction(char *dst, char src)
{
    memcpy(dst, &src, 1);
}

TEST_F(LoggerTest, NanoLog)
{
    char val = 0;
    // constexpr char src = 0xFF;
    tll::util::Counter<> counter;
    for(int i=0; i<100000; i++) logfunction(&val, 0xFF);

    LOGD("%.9f: %1x", counter.elapse().count(), val);
    // LOGV(counter.elapse().count(), (uint8_t)val);
}
