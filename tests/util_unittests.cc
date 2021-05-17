#include <gtest/gtest.h>
#include <omp.h>

#include <tll.h>

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

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION __FILE__ " : " S2(__LINE__)

TEST_F(UtilTest, VAR_STR)
{
    char var_char='h';
    uint8_t var_uint8_t=2;
    int16_t var_int16_t=3;
    uint16_t var_uint16_t=4;
    int32_t var_int32_t=5;
    uint32_t var_uint32_t=6;
    int64_t var_int64_t=7;
    uint64_t var_uint64_t=8;
    void *var_void_ptr=&var_uint64_t;
    std::string var_std_string{"tll"};
    std::vector<int> var_std_vector_int(3);
    std::vector<float> var_std_vector_float(3);
    std::vector<double> var_std_vector_double(3);
    std::atomic<int> var_atomic_int{0x100};
    LOGV(var_char, var_uint8_t, var_int16_t, var_uint16_t, var_int32_t, var_uint32_t, var_int64_t, var_uint64_t, (size_t)var_void_ptr);
    LOGV(var_std_string);
    LOGV(var_std_vector_int, var_std_vector_float, var_std_vector_double);
    LOGV(var_atomic_int);
    // const char *file_func = __FUNCTION__ __FILE__;
    LOGV(std::string(LOCATION));
    std::string empty;
    LOGV(empty);
}

TEST_F(UtilTest, Counter)
{
    using namespace tll::time;

    Counter counter;

    {
        struct timespec ts;
        counter.start();
        for(int i=0; i<1e6; i++)
        {
            // tp = rdtsc();
            clock_gettime(CLOCK_MONOTONIC, &ts);
        }
        double elapse = counter.elapse().count();
        LOGD("%.9f", elapse * 1e-6);
        LOGV(elapse);
    }

    {
        std::chrono::steady_clock::time_point tp;
        counter.start();
        for(int i=0; i<1e6; i++)
        {
            tp = std::chrono::steady_clock::now();
        }
        double elapse = counter.elapse().count();
        LOGD("%.9f", elapse * 1e-6);
        LOGV(elapse);
    }

}