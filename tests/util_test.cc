#include <gtest/gtest.h>

// #include <omp.h>

#include "../libs/util.h"
#include "../libs/counter.h"
// #include "../libs/lffifo.h"
// #include "../libs/log.h"


using namespace tll::util;

struct UtilTest : public ::testing::Test
{
    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

TEST_F(UtilTest, Write)
{
    char var1='h';
    uint8_t var2=2;
    int16_t var3=3;
    uint16_t var4=4;
    int32_t var5=5;
    uint32_t var6=6;
    int64_t var7=7;
    uint64_t var8=8;
    void *var9=nullptr;
    std::string var10{"tll"};
    std::vector<int> var11(3);
    std::vector<float> var12(3);
    std::vector<double> var13(3);

    LOGD("%s", DUMPVAR(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10).data());
    LOGD("%s", DUMPVAR(var11, var12, var13).data());
    // SomeFunc(STRINGIFY(var1, var2, obj.get()));

    // var9 = (void*) (var11.data());

    // Write(printf, "Hello % % %\n", 2015, "hahaha", name);
    // SomeFunc(STRINGIFY(true, false, var1, var2, var3, var4, var5, var6, var7, var8, var9, "oi troi oi"));
    // LOGD("%s", toString(true, false, var1, var2, var3, var4, var5, var6, var7, var8, var9, "oi troi oi").data());
    // LOGD("%s", toString(var11).data());
    // LOGD("%s", toString(var12).data());
    // LOGD("%s", toString(var13).data());
    /// {%c}{%x}{%d}{%x}{%d}{%x}{%d}{%x}{%d}{%x}{%s}
    /// {%ld {}{}{}{}{}{}}
}

