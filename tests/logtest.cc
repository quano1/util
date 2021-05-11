#include <gtest/gtest.h>

#include <omp.h>

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/lffifo.h"
#include "../libs/log2.h"


struct LoggerTest : public ::testing::Test
{

};

TEST_F(LoggerTest, Log)
{
	TLL_GLOGD("");
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
