#include <gtest/gtest.h>

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

struct CCFIFOPrimitiveTypeTest : public ::testing::Test
{
};

TEST_F(CCFIFOPrimitiveTypeTest, PushPop1Byte)
{
    tll::lf::CCFIFO<int8_t> fifo{8};
    int8_t val;
    for(size_t i=0; i < fifo.capacity()*2; i++)
    {
        EXPECT_TRUE(fifo.push((int8_t)i));
        fifo.pop(val);
        EXPECT_EQ(val, (int8_t)i);
    }
}

TEST_F(CCFIFOPrimitiveTypeTest, PushPop2Byte)
{
    tll::lf::CCFIFO<int16_t> fifo{8};
    int16_t val;
    for(size_t i=0; i < fifo.capacity()*2; i++)
    {
        EXPECT_TRUE(fifo.push((int16_t)i));
        LOGD("%d", i);
        fifo.pop(val);
        LOGD("");
        EXPECT_EQ(val, (int16_t)i);
    }
}

TEST_F(CCFIFOPrimitiveTypeTest, PushPop4Byte)
{
    tll::lf::CCFIFO<int32_t> fifo{8};
    int32_t val;
    for(size_t i=0; i < fifo.capacity()*2; i++)
    {
        EXPECT_TRUE(fifo.push((int32_t)i));
        LOGD("%d", i);
        fifo.pop(val);
        LOGD("");
        EXPECT_EQ(val, (int32_t)i);
    }
}

TEST_F(CCFIFOPrimitiveTypeTest, PushPop8Byte)
{
    tll::lf::CCFIFO<int64_t> fifo{8};
    for(size_t i=0; i < fifo.capacity()*2; i++)
    {
        EXPECT_TRUE(fifo.push((int64_t)i));
        int64_t val;
        fifo.pop(val);
        EXPECT_EQ(val, i);
    }
}

int unittests(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}