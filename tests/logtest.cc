#include <gtest/gtest.h>

#include <omp.h>

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/lffifo.h"
#include "../libs/log2.h"

using namespace tll::log2;

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


TEST_F(LoggerTest, Logs)
{
    tll::time::Counter<> counter;
    for(int i=0; i<1000000; i++) __asm__("nop");
    LOGD("%f", counter.elapse().count());
    counter.start();
    for(int i=0; i<1000000; i++)
    {
        __asm__("nop");
        TLL_GLOGD("");
    }
    double time = counter.elapse().count();
    LOGD("%f:%f", time/1000000, time);
}
