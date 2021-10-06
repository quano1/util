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
        Manager::instance().start<false>();
    }

    void TearDown()
    {
        Manager::instance().stop();
    }
};

TEST_F(LoggerTest, Log)
{
    static auto &ins = tll::log::Manager::instance();
    tll::util::Counter<> counter;
    double finish_log_time, total_log_time;
    size_t log_count = 0;
    {
        ins.total_size = 0;
        ins.stop();
        ins.start<false>();
        std::this_thread::sleep_for(std::chrono::nanoseconds(0));
        counter.start();
        for(log_count=0; log_count<0x1000; log_count++)
        {
            TLL_GLOGD("%.9f", counter.elapse().count());
        }
        finish_log_time = counter.elapse().count();
        ins.stop();
        total_log_time = counter.elapse().count();
        LOGV(log_count, ins.total_size, finish_log_time, total_log_time);
        LOGD("Callback: %.3f us", finish_log_time/log_count*1e6);
        LOGD("In/Out speed: %.3f / %.3f MBs", ins.total_size / finish_log_time / 0x100000, ins.total_size / total_log_time / 0x100000);
    }

    {
        ins.total_size = 0;
        ins.stop();
        ins.start<false>();
        std::this_thread::sleep_for(std::chrono::nanoseconds(0));
        counter.start();
        for(log_count=0; log_count<0x1000; log_count++)
        {
            TLL_GLOGTF();
            TLL_GLOGT("loop");
            TLL_GLOGD("%.9f", counter.elapse().count());
            TLL_GLOGI("%.9f", counter.elapse().count());
            TLL_GLOGW("%.9f", counter.elapse().count());
            TLL_GLOGF("%.9f", counter.elapse().count());
        }
        finish_log_time = counter.elapse().count();
        ins.stop();
        total_log_time = counter.elapse().count();
        LOGV(log_count, ins.total_size, finish_log_time, total_log_time);
        LOGD("Callback: %.3f us", finish_log_time/log_count*1e6);
        LOGD("In/Out speed: %.3f / %.3f MBs", ins.total_size / finish_log_time / 0x100000, ins.total_size / total_log_time / 0x100000);
    }
}