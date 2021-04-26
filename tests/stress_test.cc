#if (HAVE_OPENMP)

// #else

#include <gtest/gtest.h>

#include <omp.h>

#include "common.h"
#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

using namespace tll::lf2;

static constexpr size_t kCapacity = 0x10000;

#define PROFILING true
#define SEQUENCE_EXTEND 1u
#define CONCURRENT_EXTEND 0u
// #define DUMP 3000
/// Should run with --gtest_filter=ccfifoBufferStressTest.* --gtest_repeat=10000
struct StressTestRingBuffer : public ::testing::Test
{
    void SetUp()
    {
        column_lst.clear();
        threads_lst.clear();
        time_lst.clear();
        total_size_lst.clear();
        total_count_lst.clear();
    }

    void TearDown()
    {
        // plotting();
        LOGD("");
    }

    static void SetUpTestCase()
    {
        LOGD("Should run with --gtest_filter=StressTestRingBuffer.* --gtest_repeat=100000 --gtest_break_on_failure");
    }

    void plotting()
    {
        std::vector<double> speed_lst;
        speed_lst.resize(time_lst.size());
        for(int i=0; i<total_size_lst.size(); i++)
        {
            speed_lst[i*2] = (total_size_lst[i] * 1.f / time_lst[i*2]) / 0x100000; /// push
            speed_lst[i*2+1] = (total_size_lst[i] * 1.f / time_lst[i*2+1]) / 0x100000; /// pop
        }

        tll::test::plot_data(tll::util::stringFormat("%s_speed.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), tll::util::stringFormat("Higher better (CPU: %d)", NUM_CPU), "Speed in Mbs",
                  column_lst,
                  threads_lst, 
                  speed_lst 
                  );
        /// calculate relative time
        for(int i=0; i<time_lst.size(); i++)
        {
            time_lst[i] /= threads_lst[i/column_lst.size()];
        }
        tll::test::plot_data(tll::util::stringFormat("%s_time.dat", ::testing::UnitTest::GetInstance()->current_test_info()->name()), tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Relative completing time in seconds",
                  column_lst,
                  threads_lst,
                  time_lst
                  );
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
    //               threads_lst,
    //               time_lst
    //               );

    // }

    template <size_t extend, typename FIFO>
    void SequenceFixedSize(FIFO &fifo,
                     size_t max_pkg_size,
                     const std::function<void()> &resting)
    {
        LOGD("=====================================================");
        threads_lst.clear();
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq)
        {
            fifo.reserve(kCapacity * index_seq.value);
            size_t total_write_size = fifo.capacity() / 2;
            size_t store_size = total_write_size + ((index_seq.value) * max_pkg_size);
            LOGD("-----------------------------------------");
            LOGD("Number of threads: %ld\tpkg_size: %ld:%ld:%ld", index_seq.value, max_pkg_size, total_write_size, total_write_size/max_pkg_size);
            
            std::vector<char> store_buff[index_seq.value];
            for(int i=0; i<index_seq.value; i++)
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

            // auto do_pop_while_doing_push = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
            //     size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
            //         auto dst = store_buff[tid / 2].data() + pop_size;
            //         auto src = el;
            //         memcpy(dst, src, size);
            //     }, max_pkg_size, local_total);
            //     return ret;
            // };

            size_t rts_push = tll::test::fifing<index_seq.value>(do_push, nullptr, [](int tid){return true;}, resting, [&]{LOGD("%s",fifo.dump().data());}, total_write_size, threads_lst, time_lst, total_count_lst).first;
            /// push >= kTotalWriteSize
            ASSERT_GE(rts_push, total_write_size);
            threads_lst.pop_back();
            size_t rts_pop = tll::test::fifing<index_seq.value>(nullptr, do_pop, [](int tid){return false;}, resting, [&]{LOGD("%s",fifo.dump().data());}, rts_push, threads_lst, time_lst, total_count_lst).second;
            size_t tc_push = total_count_lst[total_count_lst.size() - 2];
            size_t tc_pop = total_count_lst.back();
            /// push == pop
            ASSERT_EQ(rts_push, rts_pop);
            ASSERT_EQ(tc_push, tc_pop);

            if(fifo.isProfilingEnabled())
            {
                auto stats = fifo.statistics();
                tll::cc::dumpStat<1>(stats, time_lst.back());
                LOGD("%ld\t%s", rts_push, fifo.dump().data());
            }
            // LOGD("Total time: %f (s)", time_lst.back());

            for(int i=0; i<index_seq.value; i++)
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
                // LOGD("%s", fifo.dump().data());
            }
            // LOGD("Total time: %f (s)", time_lst.back());

            // total_size_lst[total_size_lst.size() - 1] /= index_seq.value;
            // total_size_lst[total_size_lst.size() - 2] /= index_seq.value;
            // time_lst[time_lst.size() - 1] /= index_seq.value;
            // time_lst[time_lst.size() - 2] /= index_seq.value;
            // for(auto &val : total_size_lst) printf("%ld ", val); printf("\n");
            total_size_lst.push_back(rts_push);
        });
    }

    template <size_t extend, typename FIFO>
    void SequenceFixedSize2(FIFO &fifo,
                     size_t max_pkg_size,
                     const std::function<void()> &resting)
    {
        LOGD("=====================================================");
        threads_lst.clear();
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq)
        {
            fifo.reserve(kCapacity * index_seq.value);
            size_t total_write_size = fifo.capacity() / 2;
            size_t store_size = total_write_size + ((index_seq.value) * max_pkg_size);
            LOGD("-----------------------------------------");
            LOGD("Number of threads: %ld\tpkg_size: %ld:%ld:%ld", index_seq.value, max_pkg_size, total_write_size, total_write_size/max_pkg_size);
            
            std::vector<char> store_buff[index_seq.value];
            for(int i=0; i<index_seq.value; i++)
            {
                store_buff[i].resize(store_size);
                memset(store_buff[i].data(), 0xFF, store_buff[i].size());
            }

            auto do_push = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
                size_t ret = fifo.push2([](char *el, size_t size, int tid, char val, size_t push_size) {
                    memset(el, (char)(tid), size);
                }, max_pkg_size, tid, (char)loop_num, lt_size);
                return ret;
            };

            auto do_pop = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
                size_t ret = fifo.pop2([&](const char *el, size_t size, size_t pop_size) {
                    auto dst = store_buff[tid].data() + pop_size;
                    auto src = el;
                    memcpy(dst, src, size);
                }, max_pkg_size, lt_size);
                return ret;
            };

            // auto do_pop_while_doing_push = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
            //     size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
            //         auto dst = store_buff[tid / 2].data() + pop_size;
            //         auto src = el;
            //         memcpy(dst, src, size);
            //     }, max_pkg_size, local_total);
            //     return ret;
            // };
// #ifdef DUMP
//         std::atomic<bool> running{false};
//         std::condition_variable cv;
//         std::mutex cv_m;
//         std::thread dumpt = std::thread{[&](){
//             std::unique_lock<std::mutex> lk(cv_m);
//             while(!cv.wait_for(lk, std::chrono::milliseconds(DUMP), [&]{return !running.load();}))
//             {
//                 LOGD("%s", fifo.dump().data());
//             }
//         }};
// #endif
            size_t rts_push = tll::test::fifing<index_seq.value>(do_push, nullptr, [](int tid){return true;}, resting, [&]{LOGD("%s",fifo.dump().data());}, total_write_size, threads_lst, time_lst, total_count_lst).first;
// #ifdef DUMP
//             running.store(false);
//             cv.notify_all();
//             dumpt.join();
// #endif
            /// push >= kTotalWriteSize
            ASSERT_GE(rts_push, total_write_size);
            threads_lst.pop_back();
// #ifdef DUMP
//         running.store(true);
//         dumpt = std::thread{[&](){
//             std::unique_lock<std::mutex> lk(cv_m);
//             while(!cv.wait_for(lk, std::chrono::milliseconds(DUMP), [&]{return !running.load();}))
//             {
//                 LOGD("%s", fifo.dump().data());
//             }
//         }};
// #endif
            size_t rts_pop = tll::test::fifing<index_seq.value>(nullptr, do_pop, [](int tid){return false;}, resting, [&]{LOGD("%s",fifo.dump().data());}, rts_push, threads_lst, time_lst, total_count_lst).second;
// #ifdef DUMP
//             running.store(false);
//             cv.notify_all();
//             dumpt.join();
// #endif
            size_t tc_push = total_count_lst[total_count_lst.size() - 2];
            size_t tc_pop = total_count_lst.back();
            /// push == pop
            ASSERT_EQ(rts_push, rts_pop);
            ASSERT_EQ(tc_push, tc_pop);

            if(fifo.isProfilingEnabled())
            {
                auto stats = fifo.statistics();
                tll::cc::dumpStat<1>(stats, time_lst.back());
                LOGD("%ld\t%s", rts_push, fifo.dump().data());
            }
            // LOGD("Total time: %f (s)", time_lst.back());

            for(int i=0; i<index_seq.value; i++)
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
                // LOGD("%s", fifo.dump().data());
            }
            // LOGD("Total time: %f (s)", time_lst.back());

            // total_size_lst[total_size_lst.size() - 1] /= index_seq.value;
            // total_size_lst[total_size_lst.size() - 2] /= index_seq.value;
            // time_lst[time_lst.size() - 1] /= index_seq.value;
            // time_lst[time_lst.size() - 2] /= index_seq.value;
            // for(auto &val : total_size_lst) printf("%ld ", val); printf("\n");
            total_size_lst.push_back(rts_push);
        });
    }

    template <size_t extend, typename FIFO>
    void ConcurrentFixedSize(FIFO &fifo,
                          size_t max_pkg_size,
                          const std::function<void()> &resting)
    {
        threads_lst.clear();
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq) {
            fifo.reserve(kCapacity * index_seq.value);
            size_t total_write_size = fifo.capacity() * 2;
            size_t store_size = total_write_size + ((index_seq.value) * max_pkg_size);
            LOGD("Number of threads: %ld\tpkg_size: %ld:%ld:%ld", index_seq.value, max_pkg_size, total_write_size, total_write_size/max_pkg_size);
            
            std::vector<char> store_buff[index_seq.value];
            for(int i=0; i<index_seq.value; i++)
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

// #ifdef DUMP
//         std::atomic<bool> running{false};
//         std::condition_variable cv;
//         std::mutex cv_m;
//         std::thread dumpt = std::thread{[&](){
//             std::unique_lock<std::mutex> lk(cv_m);
//             while(!cv.wait_for(lk, std::chrono::milliseconds(DUMP), [&]{return !running.load();}))
//             {
//                 LOGD("%s", fifo.dump().data());
//             }
//         }};
// #endif
            auto rtt_size = tll::test::fifing<index_seq.value * 2>(do_push, do_pop_while_doing_push, [](int tid){ return (tid&1);}, resting, [&]{LOGD("%s",fifo.dump().data());}, total_write_size, threads_lst, time_lst, total_count_lst);
            size_t rts_push = rtt_size.first;
            size_t rts_pop = rtt_size.second;

// #ifdef DUMP
//             running.store(false);
//             cv.notify_all();
//             dumpt.join();
// #endif

            fifo.dumpStat(time_lst.back());
            LOGD("%ld:%ld\t%s", rts_push, rts_pop, fifo.dump().data());

            ASSERT_GE(rts_push, total_write_size);
            ASSERT_EQ(rts_push, rts_pop);
            size_t total_push_count = total_count_lst[total_count_lst.size() - 2];
            size_t total_pop_count = total_count_lst.back();
            ASSERT_EQ(total_push_count, total_pop_count);

            for(int i=0; i<index_seq.value; i++)
            {
                for(int j=0;j<rts_push; j+=max_pkg_size)
                {
                    int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, max_pkg_size - 1);
                    ASSERT_EQ(cmp, 0);
                }
            }

            total_size_lst.push_back(rts_push);
            // LOGD("Total time: %f (s)", time_lst.back());
        });
    }

    template <size_t extend, typename FIFO>
    void ConcurrentFixedSize2(FIFO &fifo,
                          size_t max_pkg_size,
                          const std::function<void()> &resting)
    {
        threads_lst.clear();
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq) {
            fifo.reserve(kCapacity * index_seq.value);
            size_t total_write_size = fifo.capacity() * 2 * index_seq.value;
            size_t store_size = total_write_size + ((index_seq.value) * max_pkg_size);
            LOGD("Number of threads: %ld\tpkg_size: %ld:%ld:%ld", index_seq.value, max_pkg_size, total_write_size, total_write_size/max_pkg_size);
            
            std::vector<char> store_buff[index_seq.value];
            for(int i=0; i<index_seq.value; i++)
            {
                store_buff[i].resize(store_size);
                memset(store_buff[i].data(), 0xFF, store_buff[i].size());
            }

            auto do_push = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
                size_t ret = fifo.push2([](char *el, size_t size, int tid, char val, size_t push_size) {
                    memset(el, (char)(tid), size);
                }, max_pkg_size, tid, (char)loop_num, lt_size);
                return ret;
            };

            auto do_pop_while_doing_push = [&](int tid, size_t loop_num, size_t lt_size, size_t tt_count, size_t tt_size) -> size_t {
                size_t ret = fifo.pop2([&](const char *el, size_t size, size_t pop_size) {
                    auto dst = store_buff[tid / 2].data() + pop_size;
                    auto src = el;
                    memcpy(dst, src, size);
                    // if(size != 3) LOGD("%ld:%ld:%ld\t%s", size, tt_count, tt_size, fifo.dump().data());
                }, max_pkg_size, lt_size);
                // LOGD("tt_count: %ld\ttt_size: %ld", tt_count, tt_size);
                return ret;
            };

// #ifdef DUMP
//         std::atomic<bool> running{false};
//         std::condition_variable cv;
//         std::mutex cv_m;
//         std::thread dumpt = std::thread{[&](){
//             std::unique_lock<std::mutex> lk(cv_m);
//             while(!cv.wait_for(lk, std::chrono::milliseconds(DUMP), [&]{return !running.load();}))
//             {
//                 LOGD("%s", fifo.dump().data());
//             }
//         }};
// #endif

            auto rtt_size = tll::test::fifing<index_seq.value * 2>(do_push, do_pop_while_doing_push, [](int tid){ return (tid&1);}, resting, [&]{LOGD("%s",fifo.dump().data());}, total_write_size, threads_lst, time_lst, total_count_lst);
            size_t rts_push = rtt_size.first;
            size_t rts_pop = rtt_size.second;

// #ifdef DUMP
//             running.store(false);
//             cv.notify_all();
//             dumpt.join();
// #endif

            fifo.dumpStat(time_lst.back());
            LOGD("%ld:%ld\t%s", rts_push, rts_pop, fifo.dump().data());

            ASSERT_GE(rts_push, total_write_size);
            ASSERT_EQ(rts_push, rts_pop);
            size_t total_push_count = total_count_lst[total_count_lst.size() - 2];
            size_t total_pop_count = total_count_lst.back();
            ASSERT_EQ(total_push_count, total_pop_count);

            for(int i=0; i<index_seq.value; i++)
            {
                for(int j=0;j<rts_push; j+=max_pkg_size)
                {
                    int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, max_pkg_size - 1);
                    ASSERT_EQ(cmp, 0);
                }
            }

            total_size_lst.push_back(rts_push);
            // LOGD("Total time: %f (s)", time_lst.back());
        });
    }

    std::vector<std::string> column_lst;
    std::vector<int> threads_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;


    // static constexpr size_t kTotalWriteSize = 20 * 0x100000; /// 1 Mbs
    // static constexpr size_t kCapacity = kTotalWriteSize / 4;
    // static constexpr size_t kMaxPkgSize = 0x3;
    // static constexpr size_t kWrap = 16;
};


// TEST_F(StressTestRingBuffer, SlowSingleDDOnly)
// {
//     auto resting = [](){
//         std::this_thread::sleep_for(std::chrono::nanoseconds(1));
//     };

//     constexpr int kExtend = 3;
//     constexpr size_t kPkgSize = 0x400;
//     {
//         ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x400};
//         SequenceFixedSize<kExtend>(fifo, kPkgSize, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
//     }
//     {
//         ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x1000};
//         SequenceFixedSize<kExtend>(fifo, kPkgSize, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
//     }
//     {
//         ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x2000};
//         SequenceFixedSize<kExtend>(fifo, kPkgSize, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
//     }
//     threads_lst.clear();
//     {
//         ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x4000};
//         SequenceFixedSize<kExtend>(fifo, kPkgSize, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
//     }

//     // for(auto t : threads_lst) printf("%d ", t); printf("\n");

// /// Speed in Mbs
// /// Completing time in seconds
//     column_lst = {"W 0x400", "R 0x400", "W 0x1000", "R 0x1000", "W 0x2000", "R 0x2000", "W 0x4000", "R 0x4000"};
//     plotting();
// }


// TEST_F(StressTestRingBuffer, SlowSingle)
// {
//     /// prepare headers
//     auto resting = [](){
//         std::this_thread::sleep_for(std::chrono::nanoseconds(1));
//     };

//     {
//         ring_fifo_dd<char, PROFILING> fifo_dd{kCapacity};
//         SequenceFixedSize<SEQUENCE_EXTEND>(fifo_dd, 3, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
//     }
//     threads_lst.clear();
//     {
//         ring_fifo_ss<char, PROFILING> fifo_ss{kCapacity};
//         SequenceFixedSize<SEQUENCE_EXTEND>(fifo_ss, 3, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
//     }

//     // for(auto t : threads_lst) printf("%d ", t); printf("\n");

// /// Speed in Mbs
// /// Completing time in seconds
//     column_lst = {"dd push", "dd pop", "ss push", "ss pop"};
// }

TEST_F(StressTestRingBuffer, RushSequence)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::yield();
    };

    constexpr int kExtend = 1u;
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x20};
        SequenceFixedSize<kExtend>(fifo, 3, resting);
        fifo.reset();
        SequenceFixedSize2<kExtend>(fifo, 3, resting);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x40};
        SequenceFixedSize<kExtend>(fifo, 3, resting);
        fifo.reset();
        SequenceFixedSize2<kExtend>(fifo, 3, resting);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x80};
        SequenceFixedSize<kExtend>(fifo, 3, resting);
        fifo.reset();
        SequenceFixedSize2<kExtend>(fifo, 3, resting);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x100};
        SequenceFixedSize<kExtend>(fifo, 3, resting);
        fifo.reset();
        SequenceFixedSize2<kExtend>(fifo, 3, resting);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x200};
        SequenceFixedSize<kExtend>(fifo, 3, resting);
        fifo.reset();
        SequenceFixedSize2<kExtend>(fifo, 3, resting);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x400};
        SequenceFixedSize<kExtend>(fifo, 3, resting);
        fifo.reset();
        SequenceFixedSize2<kExtend>(fifo, 3, resting);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x800};
        SequenceFixedSize<kExtend>(fifo, 3, resting);
        fifo.reset();
        SequenceFixedSize2<kExtend>(fifo, 3, resting);
    }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    // column_lst = {"dd push", "dd pop", "dd2 push", "dd2 pop"};
    plotting();
}

// TEST_F(StressTestRingBuffer, SSRushSequence)
// {
//     auto resting = [](){
//         std::this_thread::yield();
//     };

//     constexpr int kExtend = 0u;
//     constexpr size_t kPkgSize = 0x400;

//     {
//         ring_fifo_ss<char, PROFILING> fifo{kCapacity, 0x400};
//         SequenceFixedSize<kExtend>(fifo, kPkgSize, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
//     }
// }

TEST_F(StressTestRingBuffer, DDRushConcurrent)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::yield();
    };

    constexpr int kExtend = 0u;
    constexpr size_t kPkgSize = 3;

    // {
    //     ring_fifo_dd<char, PROFILING> fifo{kCapacity};
    //     ConcurrentFixedSize<CONCURRENT_EXTEND>(fifo, kPkgSize, resting);
    // }

    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x100};
        ConcurrentFixedSize<kExtend>(fifo, kPkgSize, resting);
    }

    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x200};
        ConcurrentFixedSize<kExtend>(fifo, kPkgSize, resting);
    }

    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x400};
        ConcurrentFixedSize<kExtend>(fifo, kPkgSize, resting);
    }
    // {
    //     ring_fifo_ss<char, PROFILING> fifo_ss{kCapacity};
    //     ConcurrentFixedSize<CONCURRENT_EXTEND>(fifo_ss, 3, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    // }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    // column_lst = {"con", "con2"};
    plotting();
}

TEST_F(StressTestRingBuffer, DDRushSequence)
{
    auto resting = [](){
        std::this_thread::yield();
    };

    constexpr int kExtend = 1u;
    constexpr size_t kPkgSize = 2;

    std::vector<Statistics> stat_lst;

    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x40};
        SequenceFixedSize2<kExtend>(fifo, kPkgSize, resting);
        stat_lst.push_back(fifo.statistics());
    }

    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x80};
        SequenceFixedSize2<kExtend>(fifo, kPkgSize, resting);
        stat_lst.push_back(fifo.statistics());
    }

    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x100};
        SequenceFixedSize2<kExtend>(fifo, kPkgSize, resting);
        stat_lst.push_back(fifo.statistics());
    }

    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x200};
        SequenceFixedSize2<kExtend>(fifo, kPkgSize, resting);
        stat_lst.push_back(fifo.statistics());
    }

    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x400};
        SequenceFixedSize2<kExtend>(fifo, kPkgSize, resting);
        stat_lst.push_back(fifo.statistics());
    }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    // column_lst = {"W 0x400", "R 0x400", "W 0x1000", "R 0x1000", "W 0x2000", "R 0x2000", "W 0x4000", "R 0x4000"};
    // column_lst = {"seq W", "seq R", "seq2 W", "seq2 R"};
    // plottingStat(stat_lst);
}

#endif