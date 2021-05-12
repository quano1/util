#include <gtest/gtest.h>

#include <omp.h>

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/lffifo.h"
#include "../libs/log.h"

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

TEST_F(LoggerTest, LogSingle)
{
    tll::time::Counter<> counter;
    TLL_GLOGD("");
    LOGD("%f", counter.elapse().count());
}

TEST_F(LoggerTest, SequenceLog)
{
    tll::time::Counter<> counter;
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
    tll::time::Counter<> counter;
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
