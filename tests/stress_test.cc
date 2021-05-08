#if (HAVE_OPENMP)

// #else

#include <gtest/gtest.h>

#include <omp.h>

#include "common.h"
#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

using namespace tll::lf2;

// static constexpr size_t kCapacity = 0x100000;
static constexpr size_t kWriteCount = 500000;

#define PROFILING true
#define SEQUENCE_EXTEND 1u
#define CONCURRENT_EXTEND 0u
// #define DUMP 3000
/// Should run with --gtest_filter=ccfifoBufferStressTest.* --gtest_repeat=10000
struct StressTestRingBuffer : public ::testing::Test
{
    void SetUp()
    {
        // LOGD("%ld:%ld:%ld", thread_lst.size(), time_lst.size(), total_size_lst.size());
    }

    void TearDown()
    {
        // LOGD("%ld:%ld:%ld", thread_lst.size(), time_lst.size(), total_size_lst.size());
    }

    static void SetUpTestCase()
    {
        LOGD("Should run with --gtest_filter=StressTestRingBuffer.* --gtest_repeat=100000 --gtest_break_on_failure");
    }

    void plotting()
    {
        size_t col_size = total_size_lst.size() * 2 / thread_lst.size();
        tll::test::plot_data(tll::util::stringFormat("%s_time.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Relative completing time in seconds",
                  column_lst,
                  thread_lst,
                  time_lst
                  );

        column_lst.clear();
        thread_lst.clear();
        time_lst.clear();
        total_size_lst.clear();
        total_count_lst.clear();
    }

    // template <typename Stat>
    // void plottingStat(const std::vector<Stat> &stat_lst)
    // {
    //     column_lst.clear();
    //     std::vector<> data;
    //     for(int i=0; i<stat_lst.size(); i++)
    //     {
    //         column_lst.push_back(std::to_string("i"));
    //     }

    //     tll::test::plot_data(tll::util::stringFormat("%s_time.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), "Lower better", "Relative completing time in seconds",
    //               column_lst,
    //               thread_lst,
    //               time_lst
    //               );

    // }

    template <size_t num_of_threads, typename FIFO>
    void SequenceFixedSize(FIFO &fifo,
                     size_t max_pkg_size,
                     size_t total_write_size,
                     const std::function<void()> &resting)
    {
        size_t store_size = total_write_size + ((num_of_threads) * max_pkg_size);
        std::vector<char> store_buff[num_of_threads];
        for(int i=0; i<num_of_threads; i++)
        {
            store_buff[i].resize(store_size);
            memset(store_buff[i].data(), 0xFF, store_buff[i].size());
        }

        auto do_push = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
            size_t ret = fifo.push([](char *el, size_t size, int tid, char val, size_t push_size) {
                memset(el, (char)(tid), size);
            }, max_pkg_size, tid, (char)loop_num, lt_size);
            return ret;
        };

        auto do_pop = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
            size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
                auto dst = store_buff[tid].data() + pop_size;
                auto src = el;
                memcpy(dst, src, size);
            }, max_pkg_size, lt_size);
            return ret;
        };

        size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, resting, [&]{LOGD("%s",fifo.dump().data());}, total_write_size, time_lst, total_count_lst).first;
        /// push >= kTotalWriteSize
        ASSERT_GE(rts_push, total_write_size);
        size_t rts_pop = tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, resting, [&]{LOGD("%s",fifo.dump().data());}, rts_push, time_lst, total_count_lst).second;
        size_t tc_push = total_count_lst[total_count_lst.size() - 2];
        size_t tc_pop = total_count_lst.back();
        /// push == pop
        ASSERT_EQ(rts_push, rts_pop);
        ASSERT_EQ(tc_push, tc_pop);

        if(fifo.isProfilingEnabled())
        {
            auto stats = fifo.statistics();
            tll::cc::dumpStat<1>(stats, time_lst.back());
            // LOGD("%ld\t%s", rts_push, fifo.dump().data());
        }
        // LOGD("Total time: %f (s)", time_lst.back());

        for(int i=0; i<num_of_threads; i++)
        {
            for(int j=0;j<rts_push; j+=max_pkg_size)
            {
                int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, max_pkg_size - 1);
                ASSERT_EQ(cmp, 0);
            }
        }

        if(fifo.isProfilingEnabled())
        {
            auto stats = fifo.statistics();
            tll::cc::dumpStat<2>(stats, time_lst.back());
            // LOGD("%ld\t%s", rts_push, fifo.dump().data());
        }
        // LOGD("Total time: %f (s)", time_lst.back());

        // total_size_lst[total_size_lst.size() - 1] /= index_seq.value;
        // total_size_lst[total_size_lst.size() - 2] /= index_seq.value;
        // time_lst[time_lst.size() - 1] /= index_seq.value;
        // time_lst[time_lst.size() - 2] /= index_seq.value;
        // for(auto &val : total_size_lst) printf("%ld ", val); printf("\n");
        total_size_lst.push_back(rts_push);
    }

    template <size_t num_of_threads, typename FIFO>
    void ConcurrentFixedSize(FIFO &fifo,
                          size_t max_pkg_size, 
                          size_t total_write_size,
                          const std::function<void()> &resting)
    {
        // size_t total_write_size = fifo.capacity() * 2;
        size_t store_size = total_write_size + ((num_of_threads) * max_pkg_size);
        // LOGD("Number of threads: %ld\tpkg_size: %ld:%ld:%ld", num_of_threads, max_pkg_size, total_write_size, total_write_size/max_pkg_size);
        
        std::vector<char> store_buff[num_of_threads];
        for(int i=0; i<num_of_threads; i++)
        {
            store_buff[i].resize(store_size);
            memset(store_buff[i].data(), 0xFF, store_buff[i].size());
        }

        auto do_push = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
            size_t ret = fifo.push([](char *el, size_t size, int tid, char val, size_t push_size) {
                memset(el, (char)(tid), size);
            }, max_pkg_size, tid, (char)loop_num, lt_size);
            return ret;
        };

        auto do_pop_while_doing_push = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
            size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
                auto dst = store_buff[tid / 2].data() + pop_size;
                auto src = el;
                memcpy(dst, src, size);
                // if(size != 3) LOGD("%ld:%ld:%ld\t%s", size, tt_count, tt_size, fifo.dump().data());
            }, max_pkg_size, lt_size);
            // LOGD("tt_count: %ld\ttt_size: %ld", tt_count, tt_size);
            return ret;
        };

        auto rtt_size = tll::test::fifing<num_of_threads * 2>(do_push, do_pop_while_doing_push, [](int tid){ return (tid&1);}, resting, [&]{LOGD("%s",fifo.dump().data());}, total_write_size, time_lst, total_count_lst);
        size_t rts_push = rtt_size.first;
        size_t rts_pop = rtt_size.second;

        if(fifo.isProfilingEnabled())
        {
            auto stats = fifo.statistics();
            tll::cc::dumpStat<>(stats, time_lst.back());
            // LOGD("%ld\t%s", rts_push, fifo.dump().data());
        }

        ASSERT_GE(rts_push, total_write_size);
        ASSERT_EQ(rts_push, rts_pop);
        size_t total_push_count = total_count_lst[total_count_lst.size() - 2];
        size_t total_pop_count = total_count_lst.back();
        ASSERT_EQ(total_push_count, total_pop_count);

        for(int i=0; i<num_of_threads; i++)
        {
            for(int j=0;j<rts_push; j+=max_pkg_size)
            {
                int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, max_pkg_size - 1);
                ASSERT_EQ(cmp, 0);
            }
        }

        total_size_lst.push_back(rts_push);
        // LOGD("Total time: %f (s)", time_lst.back());
    }

    static std::vector<std::string> column_lst;
    static std::vector<int> thread_lst;
    static std::vector<double> time_lst;
    static std::vector<size_t> total_size_lst, total_count_lst;

    // static constexpr size_t kTotalWriteSize = 20 * 0x100000; /// 1 Mbs
    // static constexpr size_t kCapacity = kTotalWriteSize / 4;
    // static constexpr size_t kMaxPkgSize = 0x3;
    // static constexpr size_t kWrap = 16;
}; /// StressTestRingBuffer

std::vector<std::string> StressTestRingBuffer::column_lst;
std::vector<int> StressTestRingBuffer::thread_lst;
std::vector<double> StressTestRingBuffer::time_lst;
std::vector<size_t> StressTestRingBuffer::total_size_lst;
std::vector<size_t> StressTestRingBuffer::total_count_lst;


// TEST_F(StressTestRingBuffer, SlowSequence)
// {
//     /// prepare headers
//     auto resting = [](){
//         std::this_thread::sleep_for(std::chrono::nanoseconds(1));
//     };

//     constexpr int kExtend = 2u;
//     {
//         ring_buffer_dd<char, PROFILING> fifo{kCapacity, 0x100};
//         SequenceFixedSize<kExtend>(fifo, 3, resting);
//     }
//     {
//         ring_buffer_dd<char, PROFILING> fifo{kCapacity, 0x400};
//         SequenceFixedSize<kExtend>(fifo, 3, resting);
//     }
//     {
//         ring_buffer_dd<char, PROFILING> fifo{kCapacity, 0x1000};
//         SequenceFixedSize<kExtend>(fifo, 3, resting);
//     }

//     plotting();
// }

// TEST_F(StressTestRingBuffer, RushSequence)
// {
//     /// prepare headers
//     auto resting = [](){
//         std::this_thread::yield();
//     };

//     constexpr int kExtend = 2u;
//     {
//         ring_buffer_dd<char, PROFILING> fifo{kCapacity, 0x100};
//         SequenceFixedSize<kExtend>(fifo, 3, resting);
//     }
//     {
//         ring_buffer_dd<char, PROFILING> fifo{kCapacity, 0x400};
//         SequenceFixedSize<kExtend>(fifo, 3, resting);
//     }
//     {
//         ring_buffer_dd<char, PROFILING> fifo{kCapacity, 0x1000};
//         SequenceFixedSize<kExtend>(fifo, 3, resting);
//     }

//     // column_lst = {"dd push", "dd pop", "dd2 push", "dd2 pop"};
//     plotting();
// }

// TEST_F(StressTestRingBuffer, SSRushSequence)
// {
//     auto resting = [](){
//         std::this_thread::yield();
//     };

//     constexpr int kExtend = 0u;
//     constexpr size_t kPkgSize = 0x400;

//     {
//         ring_buffer_ss<char, PROFILING> fifo{kCapacity, 0x400};
//         SequenceFixedSize<kExtend>(fifo, kPkgSize, resting, thread_lst, time_lst, total_size_lst, total_count_lst);
//     }
// }

TEST_F(StressTestRingBuffer, DDRushConcurrent)
{
    auto resting = [](){
        std::this_thread::yield();
    };

    constexpr int kExtend = 1u;
    constexpr size_t kPkgSize = 3;
    // size_t write_count = kWriteCount;
    // constexpr size_t kCapacity = kPkgSize * kWriteCount;

    // std::vector<Statistics> stat_lst;
    tll::util::CallFuncInSeq<NUM_CPU, kExtend>( [&](auto index_seq)
    {
        thread_lst.push_back(index_seq.value*2);
        size_t total_write_size = kWriteCount * kPkgSize * index_seq.value;
        size_t capacity = total_write_size / 8;
        LOGD("=====================================================");
        LOGD("Number of threads: %ld\tpkg_size: %ld:%ld:%ld", index_seq.value, kPkgSize, total_write_size, total_write_size / kPkgSize);
        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x400};
            ConcurrentFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x800};
            ConcurrentFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x1000};
            ConcurrentFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x2000};
            ConcurrentFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
    });
    plotting();
}

TEST_F(StressTestRingBuffer, DDRushSequence)
{
    auto resting = [](){
        std::this_thread::yield();
    };

    constexpr int kExtend = 5u;
    constexpr size_t kPkgSize = 3;
    // constexpr size_t kCapacity = kPkgSize * kWriteCount;

    // std::vector<Statistics> stat_lst;
    tll::util::CallFuncInSeq<NUM_CPU, kExtend>( [&](auto index_seq)
    {
        thread_lst.push_back(index_seq.value);
        size_t total_write_size = kWriteCount * kPkgSize * index_seq.value;
        size_t capacity = total_write_size * 2;
        LOGD("=====================================================");
        LOGD("Number of threads: %ld\tpkg_size: %ld:%ld:%ld", index_seq.value, kPkgSize, total_write_size, total_write_size / kPkgSize);
        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x400};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x800};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x1000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x2000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
    });
    plotting();
}

#endif