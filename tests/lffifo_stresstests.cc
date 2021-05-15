#if (HAVE_OPENMP)

#include <gtest/gtest.h>
#include <omp.h>

#include <tll.h>

#include "common.h"

using namespace tll::lf;

// static constexpr size_t kCapacity = 0x100000;
static constexpr size_t kWriteCount = 500000;

#define PROFILING true
#define SEQUENCE_EXTEND 1u
#define CONCURRENT_EXTEND 0u
// #define DUMP 3000
/// Should run with --gtest_filter=LFFIFOBufferStressTest.* --gtest_repeat=10000
struct LFFIFOStressTest : public ::testing::Test
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
        LOGD("Should run with --gtest_filter=LFFIFOStressTest.* --gtest_repeat=100000 --gtest_break_on_failure");
    }

    void plotting()
    {
        tll::test::plot_data(tll::util::stringFormat("%s_time.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Relative completing time in seconds",
                  column_lst,
                  thread_lst,
                  time_lst
                  );

        // column_lst.clear();
        // thread_lst.clear();
        // time_lst.clear();
        // total_size_lst.clear();
        // total_count_lst.clear();
    }

    void plotting(std::vector<Statistics> stat_lst)
    {
        std::vector<size_t> data;
        for(auto &stat : stat_lst)
        {
            auto &push_stat = stat.first;
            auto &pop_stat = stat.second;
            data.push_back(push_stat.time_try / push_stat.try_count);
            data.push_back(pop_stat.time_try / pop_stat.try_count);
        }
        
        tll::test::plot_data(tll::util::stringFormat("%s_stat_try.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Avg time (ns) of try",
                  column_lst,
                  thread_lst,
                  data
                  );

        data.clear();
        for(auto &stat : stat_lst)
        {
            auto &push_stat = stat.first;
            size_t comp_count = push_stat.try_count - push_stat.error_count;
            data.push_back(push_stat.time_comp / comp_count);

            auto &pop_stat = stat.second;
            comp_count = push_stat.try_count - push_stat.error_count;
            data.push_back(pop_stat.time_comp / comp_count);
        }
        
        tll::test::plot_data(tll::util::stringFormat("%s_push_stat_comp.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Avg time (ns) of comp",
                  column_lst,
                  thread_lst,
                  data
                  );

        data.clear();
        for(auto &stat : stat_lst)
        {
            auto &push_stat = stat.first;
            data.push_back(push_stat.try_miss_count);

            auto &pop_stat = stat.second;
            data.push_back(pop_stat.try_miss_count);
        }
        
        tll::test::plot_data(tll::util::stringFormat("%s_push_stat_try_miss.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Avg time (ns) of try_miss",
                  column_lst,
                  thread_lst,
                  data
                  );

        data.clear();
        for(auto &stat : stat_lst)
        {
            auto &push_stat = stat.first;
            data.push_back(push_stat.comp_miss_count);

            auto &pop_stat = stat.second;
            data.push_back(pop_stat.comp_miss_count);
        }
        
        tll::test::plot_data(tll::util::stringFormat("%s_push_stat_comp_miss.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Avg time (ns) of comp_miss",
                  column_lst,
                  thread_lst,
                  data
                  );
    }

    template <size_t num_of_threads, typename FIFO>
    void SequenceFixedSize(FIFO &fifo,
                     size_t max_pkg_size,
                     size_t total_write_size,
                     const std::function<void()> &resting,
                     bool verification=true)
    {
        size_t store_size = total_write_size + ((num_of_threads) * max_pkg_size);
        std::vector<char> store_buff[num_of_threads];
        if(verification)
            for(int i=0; i<num_of_threads; i++)
            {
                store_buff[i].resize(store_size);
                memset(store_buff[i].data(), 0xFF, store_buff[i].size());
            }

        auto do_push = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
            size_t ret = fifo.push([verification](char *el, size_t size, int tid, char val, size_t push_size) {
                if(!verification) return;

                memset(el, (char)(tid), size);
            }, max_pkg_size, tid, (char)loop_num, lt_size);
            return ret;
        };

        auto do_pop = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
            size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
                if(!verification) return;

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
            fifo.dumpStat(time_lst.back(), 1);
            // LOGD("%ld\t%s", rts_push, fifo.dump().data());
        }
        // LOGD("Total time: %f (s)", time_lst.back());

        if(verification)
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
            fifo.dumpStat(time_lst.back(), 2);
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
            fifo.dumpStat(time_lst.back());
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
}; /// LFFIFOStressTest

std::vector<std::string> LFFIFOStressTest::column_lst;
std::vector<int> LFFIFOStressTest::thread_lst;
std::vector<double> LFFIFOStressTest::time_lst;
std::vector<size_t> LFFIFOStressTest::total_size_lst;
std::vector<size_t> LFFIFOStressTest::total_count_lst;


// TEST_F(LFFIFOStressTest, SlowSequence)
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

// TEST_F(LFFIFOStressTest, RushSequence)
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

TEST_F(LFFIFOStressTest, SequenceTrafficJamDD)
{
    auto resting = [](){
        std::this_thread::sleep_for(std::chrono::nanoseconds(0));
    };

    constexpr int kExtend = 3u;
    constexpr size_t kPkgSize = 5;
    // constexpr size_t kCapacity = kPkgSize * kWriteCount;

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

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x4000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x8000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x10000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");
    });
    column_lst = {
        "W 0x400", "R 0x400",
        "W 0x800", "R 0x800",
        "W 0x1000", "R 0x1000",
        "W 0x2000", "R 0x2000",
        "W 0x4000", "R 0x4000",
        "W 0x8000", "R 0x8000",
        "W 0x10000", "R 0x10000",
    };
    plotting();
} /// SequenceTrafficJamDD

TEST_F(LFFIFOStressTest, SequenceRushDD)
{
    auto resting = [](){
        std::this_thread::yield();
    };

    constexpr int kExtend = 5u;
    constexpr size_t kPkgSize = 5;

    std::vector<Statistics> stat_lst;
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
            stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x800};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x1000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x2000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            stat_lst.push_back(fifo.statistics());
        }

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x4000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x8000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x10000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting);
            stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");
    });
    column_lst = {
        "W 0x400", "R 0x400",
        "W 0x800", "R 0x800",
        "W 0x1000", "R 0x1000",
        "W 0x2000", "R 0x2000",
        "W 0x4000", "R 0x4000",
        "W 0x8000", "R 0x8000",
        "W 0x10000", "R 0x10000",
    };
    plotting();
    plotting(stat_lst);
} /// SequenceRushDD


TEST_F(LFFIFOStressTest, SequenceRushMixed)
{
    auto resting = [](){
        std::this_thread::yield();
    };

    constexpr int kExtend = 9u;
    constexpr size_t kPkgSize = 5;

    // std::vector<Statistics> stat_lst;
    tll::util::CallFuncInSeq<NUM_CPU, kExtend>( [&](auto index_seq)
    {
        thread_lst.push_back(index_seq.value);
        size_t total_write_size = kWriteCount * kPkgSize * index_seq.value;
        size_t capacity = total_write_size * 2;
        LOGD("=====================================================");
        LOGD("Number of threads: %ld\tpkg_size: %ld:%ld:%ld", index_seq.value, kPkgSize, total_write_size, total_write_size / kPkgSize);
        // {
        //     ring_buffer_ss<char, PROFILING> fifo{capacity};
        //     SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting, false);
        //     // stat_lst.push_back(fifo.statistics());
        // }
        // LOGD("-----------------------------------------");

        {
            ring_buffer_dd<char, PROFILING> fifo{capacity, 0x10000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting, false);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");

        // {
        //     ring_queue_ss<char, PROFILING> fifo{capacity};
        //     SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting, false);
        //     // stat_lst.push_back(fifo.statistics());
        // }
        // LOGD("-----------------------------------------");

        {
            ring_queue_dd<char, PROFILING> fifo{capacity, 0x10000};
            SequenceFixedSize<index_seq.value>(fifo, kPkgSize, total_write_size, resting, false);
            // stat_lst.push_back(fifo.statistics());
        }
        LOGD("-----------------------------------------");
    });
    column_lst = {
        // "buffer ss", "buffer ss",
        "buffer dd", "buffer dd",
        // "queue ss", "queue ss",
        "queue dd", "queue dd",
    };
    plotting();
} /// DDRushSequence


TEST_F(LFFIFOStressTest, DISABLED_DDRushConcurrent)
{
    auto resting = [](){
        std::this_thread::yield();
    };

    constexpr int kExtend = 1u;
    constexpr size_t kPkgSize = 5;
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

#endif