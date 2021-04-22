#if (HAVE_OPENMP)

// #else

#include <gtest/gtest.h>

#include <omp.h>

#include "common.h"
#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

using namespace tll::lf2;

#define NOP_LOOP() for(int i__=0; i__<0x400; i__++) __asm__("nop")
#define PROFILING false
#define SINGLE_EXTEND 1u
#define SIMUL_EXTEND 0u
// #define DUMP
/// Should run with --gtest_filter=ccfifoBufferStressTest.* --gtest_repeat=10000
struct RingBufferStressTest : public ::testing::Test
{
    static void SetUpTestCase()
    {
        LOGD("Should run with --gtest_filter=RingBufferStressTest.* --gtest_repeat=100000 --gtest_break_on_failure");
    }

    // static constexpr size_t kTotalWriteSize = 20 * 0x100000; /// 1 Mbs
    // static constexpr size_t kCapacity = kTotalWriteSize / 4;
    static constexpr size_t kMaxPkgSize = 0x2;
    static constexpr size_t kCapacity = 0x4000 * kMaxPkgSize;
    // static constexpr size_t kWrap = 16;

    template <size_t extend, typename FIFO>
    void RWFixedSize(FIFO &fifo, const std::function<void()> &resting, 
                     std::vector<int> &threads_lst, 
                     std::vector<double> &time_lst, 
                     std::vector<size_t> &total_size_lst,
                     std::vector<size_t> &total_count_lst)
    {
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq)
        {
            LOGD("Number of threads: %ld", index_seq.value);
            fifo.reserve(kCapacity * index_seq.value);
            size_t total_write_size = fifo.capacity() / 2;
            size_t store_size = total_write_size + ((index_seq.value) * kMaxPkgSize);
            
            std::vector<char> store_buff[index_seq.value];
            for(int i=0; i<index_seq.value; i++)
            {
                store_buff[i].resize(store_size);
                memset(store_buff[i].data(), 0xFF, store_buff[i].size());
            }

            auto do_push = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
                size_t ret = fifo.push([](char *el, size_t size, int tid, char val, size_t push_size) {
                    NOP_LOOP();
                    memset(el, (char)(tid), size);
                }, kMaxPkgSize, tid, (char)loop_num, local_total);
                return ret;
            };

            auto do_pop = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
                size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
                    auto dst = store_buff[tid].data() + pop_size;
                    auto src = el;
                    memcpy(dst, src, size);
                    NOP_LOOP();
                }, kMaxPkgSize, local_total);
                return ret;
            };

            // auto do_pop_while_doing_push = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
            //     size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
            //         auto dst = store_buff[tid / 2].data() + pop_size;
            //         auto src = el;
            //         memcpy(dst, src, size);
            //     }, kMaxPkgSize, local_total);
            //     return ret;
            // };

            auto real_total_write_size = tll::test::fifing<index_seq.value>(do_push, nullptr, [](int tid){return true;}, resting, total_write_size, threads_lst, time_lst, total_count_lst);
            /// push >= kTotalWriteSize
            ASSERT_GE(real_total_write_size, total_write_size);
            threads_lst.pop_back();
            tll::test::fifing<index_seq.value>(nullptr, do_pop, [](int tid){return false;}, resting, real_total_write_size, threads_lst, time_lst, total_count_lst);
            /// push == pop
            ASSERT_EQ(total_count_lst[total_count_lst.size() - 1], total_count_lst[total_count_lst.size() - 2]);

            if(fifo.isProfilingEnabled())
            {
                auto stats = fifo.statistics();
                tll::cc::dumpStat<1>(stats, time_lst.back());
                // LOGD("%s", fifo.dump().data());
            }
            LOGD("Total time: %f (s)", time_lst.back());

            for(int i=0; i<index_seq.value; i++)
            {
                for(int j=0;j<store_buff[i].size(); j+=kMaxPkgSize)
                {
                    int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1);
                    ASSERT_EQ(cmp, 0);
                }
            }

            if(fifo.isProfilingEnabled())
            {
                auto stats = fifo.statistics();
                tll::cc::dumpStat<2>(stats, time_lst.back());
                // LOGD("%s", fifo.dump().data());
            }
            LOGD("Total time: %f (s)", time_lst.back());

            // total_size_lst[total_size_lst.size() - 1] /= index_seq.value;
            // total_size_lst[total_size_lst.size() - 2] /= index_seq.value;
            // time_lst[time_lst.size() - 1] /= index_seq.value;
            // time_lst[time_lst.size() - 2] /= index_seq.value;
            // for(auto &val : total_size_lst) printf("%ld ", val); printf("\n");
            total_size_lst.push_back(real_total_write_size);
        });
    }

    template <size_t extend, typename FIFO>
    void RWSimulFixedSize(FIFO &fifo, const std::function<void()> &resting,
                          std::vector<int> &threads_lst, 
                          std::vector<double> &time_lst, 
                          std::vector<size_t> &total_size_lst,
                          std::vector<size_t> &total_count_lst)
    {
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq) {
            LOGD("Number of threads: %ld", index_seq.value * 2);
            fifo.reserve(kCapacity * index_seq.value);
            size_t total_write_size = fifo.capacity() * 4;
            size_t store_size = total_write_size + ((index_seq.value) * kMaxPkgSize);
            
            std::vector<char> store_buff[index_seq.value];
            for(int i=0; i<index_seq.value; i++)
            {
                store_buff[i].resize(store_size);
                memset(store_buff[i].data(), 0xFF, store_buff[i].size());
            }

            auto do_push = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
                size_t ret = fifo.push([](char *el, size_t size, int tid, char val, size_t push_size) {
                    NOP_LOOP();
                    memset(el, (char)(tid), size);
                }, kMaxPkgSize, tid, (char)loop_num, local_total);
                return ret;
            };

            auto do_pop_while_doing_push = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
                size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
                    auto dst = store_buff[tid / 2].data() + pop_size;
                    auto src = el;
                    memcpy(dst, src, size);
                    NOP_LOOP();
                }, kMaxPkgSize, local_total);
                return ret;
            };

            // total_write_size = total_write_size * 4;
            // store_size = total_write_size + ((index_seq.value) * kMaxPkgSize);
            // std::vector<char> store_buff[index_seq.value];
            // for(int i=0; i<index_seq.value; i++)
            // {
            //     store_buff[i].resize(store_size);
            //     memset(store_buff[i].data(), 0xFF, store_buff[i].size());
            // }

// #ifdef DUMP
//         std::atomic<bool> running{true};
//         std::thread dumpt{[&](){
//             while(running.load())
//             {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                 LOGD("%s", fifo.dump().data());
//             }
//         }};
// #endif

            size_t real_total_write_size = tll::test::fifing<index_seq.value * 2>(do_push, do_pop_while_doing_push, [](int tid){ return (tid&1);}, resting, total_write_size, threads_lst, time_lst, total_count_lst);

// #ifdef DUMP
//             running.store(false);
//             dumpt.join();
// #endif

            ASSERT_EQ(total_count_lst[total_count_lst.size() - 1], total_count_lst[total_count_lst.size() - 2]);
            ASSERT_GE(real_total_write_size, total_write_size);

            for(int i=0; i<index_seq.value; i++)
            {
                for(int j=0;j<store_buff[i].size(); j+=kMaxPkgSize)
                {
                    int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1);
                    ASSERT_EQ(cmp, 0);
                }
            }

            if(fifo.isProfilingEnabled())
            {
                auto stats = fifo.statistics();
                tll::cc::dumpStat<>(stats, time_lst.back());
                // LOGD("%s", fifo.dump().data());
            }
            total_size_lst.push_back(real_total_write_size);
            LOGD("Total time: %f (s)", time_lst.back());
        });
    }
};


TEST_F(RingBufferStressTest, SlowSingleDDOnly)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    };

    std::vector<int> threads_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x400};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x1000};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x2000};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x4000};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    threads_lst.clear();
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x8000};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    std::vector<double> speed_lst;
    std::vector<std::string> columns_lst = {"dd 0x400", "dd 0x1000", "dd 0x2000", "ss 0x4000", "ss 0x8000"};
    speed_lst.resize(columns_lst.size() * (5 + SINGLE_EXTEND));
    for(int i=0; i<total_size_lst.size(); i++)
    {
        speed_lst[i*2] = (total_size_lst[i] * 1.f / time_lst[i*2]) / 0x100000; /// push
        speed_lst[i*2+1] = (total_size_lst[i] * 1.f / time_lst[i*2+1]) / 0x100000; /// pop
    }

    tll::test::plot_data("slow_single_dd_speed.dat", "Higher better", "Speed in Mbs",
              columns_lst,
              threads_lst, 
              speed_lst 
              );

    /// calculate relative time
    for(int i=0; i<time_lst.size(); i++)
    {
        time_lst[i] /= threads_lst[i/columns_lst.size()];
    }

    tll::test::plot_data("slow_single_dd_time.dat", "Lower better", "Relative completing time in seconds",
              columns_lst,
              threads_lst,
              time_lst
              );
}

TEST_F(RingBufferStressTest, RushSingleDDOnly)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::yield();
    };

    std::vector<int> threads_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x400};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x1000};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x2000};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x4000};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    threads_lst.clear();
    {
        ring_fifo_dd<char, PROFILING> fifo{kCapacity, 0x8000};
        RWFixedSize<SINGLE_EXTEND + 2>(fifo, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    std::vector<double> speed_lst;
    std::vector<std::string> columns_lst = {"dd 0x400", "dd 0x1000", "dd 0x2000", "dd 0x4000", "dd 0x8000"};
    speed_lst.resize(columns_lst.size() * (5 + SINGLE_EXTEND));
    for(int i=0; i<total_size_lst.size(); i++)
    {
        speed_lst[i*2] = (total_size_lst[i] * 1.f / time_lst[i*2]) / 0x100000; /// push
        speed_lst[i*2+1] = (total_size_lst[i] * 1.f / time_lst[i*2+1]) / 0x100000; /// pop
    }

    tll::test::plot_data("rush_single_dd_speed.dat", "Higher better", "Speed in Mbs",
              columns_lst,
              threads_lst, 
              speed_lst 
              );

    /// calculate relative time
    for(int i=0; i<time_lst.size(); i++)
    {
        time_lst[i] /= threads_lst[i/columns_lst.size()];
    }

    tll::test::plot_data("rush_single_dd_time.dat", "Lower better", "Relative completing time in seconds",
              columns_lst,
              threads_lst,
              time_lst
              );
}

TEST_F(RingBufferStressTest, SlowSingle)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    };

    std::vector<int> threads_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;
    {
        ring_fifo_dd<char, PROFILING> fifo_dd{kCapacity};
        RWFixedSize<SINGLE_EXTEND>(fifo_dd, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    threads_lst.clear();
    {
        ring_fifo_ss<char, PROFILING> fifo_ss{kCapacity};
        RWFixedSize<SINGLE_EXTEND>(fifo_ss, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    std::vector<double> speed_lst;
    std::vector<std::string> columns_lst = {"dd push", "dd pop", "ss push", "ss pop"};
    speed_lst.resize(columns_lst.size() * (3 + SINGLE_EXTEND));
    for(int i=0; i<total_size_lst.size(); i++)
    {
        speed_lst[i*2] = (total_size_lst[i] * 1.f / time_lst[i*2]) / 0x100000; /// push
        speed_lst[i*2+1] = (total_size_lst[i] * 1.f / time_lst[i*2+1]) / 0x100000; /// pop
    }

    tll::test::plot_data("slow_single_speed.dat", "Higher better", "Speed in Mbs",
              columns_lst,
              threads_lst, 
              speed_lst 
              );

    /// calculate relative time
    for(int i=0; i<time_lst.size(); i++)
    {
        time_lst[i] /= threads_lst[i/columns_lst.size()];
    }

    tll::test::plot_data("slow_single_time.dat", "Lower better", "Relative completing time in seconds",
              columns_lst,
              threads_lst,
              time_lst
              );

}

TEST_F(RingBufferStressTest, RushSingle)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::yield();
    };

    std::vector<int> threads_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;
    {
        ring_fifo_dd<char, PROFILING> fifo_dd{kCapacity};
        RWFixedSize<SINGLE_EXTEND>(fifo_dd, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    threads_lst.clear();
    {
        ring_fifo_ss<char, PROFILING> fifo_ss{kCapacity};
        RWFixedSize<SINGLE_EXTEND>(fifo_ss, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    std::vector<double> speed_lst;
    std::vector<std::string> columns_lst = {"dd push", "dd pop", "ss push", "ss pop"};
    speed_lst.resize(columns_lst.size() * (3 + SINGLE_EXTEND));
    for(int i=0; i<total_size_lst.size(); i++)
    {
        speed_lst[i*2] = (total_size_lst[i] * 1.f / time_lst[i*2]) / 0x100000; /// push
        speed_lst[i*2+1] = (total_size_lst[i] * 1.f / time_lst[i*2+1]) / 0x100000; /// pop
    }

    tll::test::plot_data("rush_single_speed.dat", "Higher better", "Speed in Mbs",
              columns_lst,
              threads_lst, 
              speed_lst 
              );

    /// calculate relative time
    for(int i=0; i<time_lst.size(); i++)
    {
        time_lst[i] /= threads_lst[i/columns_lst.size()];
    }

    tll::test::plot_data("rush_single_time.dat", "Lower better", "Relative completing time in seconds",
              columns_lst,
              threads_lst,
              time_lst
              );

}


TEST_F(RingBufferStressTest, SlowSimultaneously)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    };

    std::vector<int> threads_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;
    {
        ring_fifo_dd<char, PROFILING> fifo_dd{kCapacity};
        RWSimulFixedSize<SIMUL_EXTEND>(fifo_dd, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    threads_lst.clear();
    {
        ring_fifo_ss<char, PROFILING> fifo_ss{kCapacity};
        RWSimulFixedSize<SIMUL_EXTEND>(fifo_ss, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    std::vector<double> speed_lst;
    std::vector<std::string> columns_lst = {"dd", "ss"};
    speed_lst.resize(columns_lst.size() * (3 + SIMUL_EXTEND));
    // LOGD("%ld %ld", speed_lst.size(), total_size_lst.size());
    for(int i=0; i<total_size_lst.size(); i++)
    {
        speed_lst[i] = ((total_size_lst[i] * 1.f / time_lst[i]) / 0x100000) / threads_lst[i/columns_lst.size()];
    }

    tll::test::plot_data("slow_simul_speed.dat", "Higher better", "Speed in Mbs",
              columns_lst,
              threads_lst, 
              speed_lst 
              );

    /// calculate relative time
    for(int i=0; i<time_lst.size(); i++)
    {
        time_lst[i] /= threads_lst[i/columns_lst.size()];
    }

    tll::test::plot_data("slow_simul_time.dat", "Lower better", "Relative completing time in seconds",
              columns_lst,
              threads_lst,
              time_lst
              );
}

TEST_F(RingBufferStressTest, RushSimultaneously)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::yield();
    };

    std::vector<int> threads_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;
    {
        ring_fifo_dd<char, PROFILING> fifo_dd{kCapacity};
        RWSimulFixedSize<SIMUL_EXTEND>(fifo_dd, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }
    threads_lst.clear();
    {
        ring_fifo_ss<char, PROFILING> fifo_ss{kCapacity};
        RWSimulFixedSize<SIMUL_EXTEND>(fifo_ss, resting, threads_lst, time_lst, total_size_lst, total_count_lst);
    }

    // for(auto t : threads_lst) printf("%d ", t); printf("\n");

/// Speed in Mbs
/// Completing time in seconds
    std::vector<double> speed_lst;
    std::vector<std::string> columns_lst = {"dd", "ss"};
    speed_lst.resize(columns_lst.size() * (3 + SIMUL_EXTEND));
    for(int i=0; i<total_size_lst.size(); i++)
    {
        speed_lst[i] = ((total_size_lst[i] * 1.f / time_lst[i]) / 0x100000) / threads_lst[i/columns_lst.size()];
    }

    tll::test::plot_data("rush_simul_speed.dat", "Higher better", "Speed in Mbs",
              columns_lst,
              threads_lst, 
              speed_lst 
              );

    /// calculate relative time
    for(int i=0; i<time_lst.size(); i++)
    {
        time_lst[i] /= threads_lst[i/columns_lst.size()];
    }

    tll::test::plot_data("rush_simul_time.dat", "Lower better", "Relative completing time in seconds",
              columns_lst,
              threads_lst,
              time_lst
              );
}
#endif