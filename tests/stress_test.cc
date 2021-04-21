#if (HAVE_OPENMP)

// #else

#include <gtest/gtest.h>

#include <omp.h>

#include "common.h"
#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

using namespace tll::lf2;
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
    static constexpr size_t kCapacity = 0x400 * kMaxPkgSize;
    // static constexpr size_t kWrap = 16;

    template <size_t extend, typename FIFO>
    void RWFixedSize(FIFO &fifo, const std::function<void()> &resting, 
                     std::vector<double> &time_lst, std::vector<size_t> &total_lst)
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
                    // *(int*)el = tid;
                    // memset(el + 4, val, size - 4);
                    memset(el, (char)(tid), size);
                }, kMaxPkgSize, tid, (char)loop_num, local_total);
                return ret;
            };

            auto do_pop = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
                size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
                    auto dst = store_buff[tid].data() + pop_size;
                    auto src = el;
                    memcpy(dst, src, size);
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

            tll::test::fifing<index_seq.value>(do_push, nullptr, [](int tid){return true;}, resting, total_write_size, time_lst, total_lst);
            /// push >= kTotalWriteSize
            total_lst.pop_back(); /// remove pop stat
            ASSERT_GE(total_lst[total_lst.size() - 1], total_write_size);

            tll::test::fifing<index_seq.value>(nullptr, do_pop, [](int tid){return false;}, resting, total_lst.back(), time_lst, total_lst);
            total_lst.erase(total_lst.begin() + (total_lst.size() - 2)); /// remove push stat
            /// push == pop
            ASSERT_EQ(total_lst[total_lst.size() - 1], total_lst[total_lst.size() - 2]);

            for(int i=0; i<index_seq.value; i++)
            {
                for(int j=0;j<store_buff[i].size(); j+=kMaxPkgSize)
                {
                    int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1);
                    ASSERT_EQ(cmp, 0);
                }
            }

            total_lst[total_lst.size() - 1] /= index_seq.value;
            total_lst[total_lst.size() - 2] /= index_seq.value;
            time_lst[time_lst.size() - 1] /= index_seq.value;
            time_lst[time_lst.size() - 2] /= index_seq.value;
            // for(auto &val : total_lst) printf("%ld ", val); printf("\n");

        });
    }

    template <size_t extend, typename FIFO>
    void RWSimulFixedSize(FIFO &fifo, const std::function<void()> &resting,
                          std::vector<double> &time_lst, std::vector<size_t> &total_lst)
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
                    memset(el, (char)(tid), size);
                }, kMaxPkgSize, tid, (char)loop_num, local_total);
                return ret;
            };

            auto do_pop_while_doing_push = [&](int tid, size_t loop_num, size_t local_total) -> size_t {
                size_t ret = fifo.pop([&](const char *el, size_t size, size_t pop_size) {
                    auto dst = store_buff[tid / 2].data() + pop_size;
                    auto src = el;
                    memcpy(dst, src, size);
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

            tll::test::fifing<index_seq.value * 2>(do_push, do_pop_while_doing_push, [](int tid){ return (tid&1);}, resting, total_write_size, time_lst, total_lst);

// #ifdef DUMP
//             running.store(false);
//             dumpt.join();
// #endif

            ASSERT_EQ(total_lst[total_lst.size() - 1], total_lst[total_lst.size() - 2]);
            ASSERT_GE(total_lst[total_lst.size() - 1], total_write_size);

            for(int i=0; i<index_seq.value; i++)
            {
                for(int j=0;j<store_buff[i].size(); j+=kMaxPkgSize)
                {
                    int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1);
                    ASSERT_EQ(cmp, 0);
                }
            }
        });
    }
};

/// MPMC read write fixed size
TEST_F(RingBufferStressTest, PushThenPop)
{
    /// prepare headers
    auto resting = [](){
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
        // std::this_thread::yield();
    };
    std::vector<double> time_lst;
    std::vector<size_t> total_lst;
    LOGD("Dense-Dense");
    {
        ring_fifo_dd<char, false> fifo_dd{kCapacity};
        RWFixedSize<1u>(fifo_dd, resting, time_lst, total_lst);
    // LOGD("%ld %ld", total_lst.size(), time_lst.size());
        RWSimulFixedSize<1u>(fifo_dd, resting, time_lst, total_lst);
    // LOGD("%ld %ld", total_lst.size(), time_lst.size());
    }
    LOGD("Sparse-Sparse");
    {
        ring_fifo_ss<char, false> fifo_ss{kCapacity};
        RWFixedSize<1u>(fifo_ss, resting, time_lst, total_lst);
    // LOGD("%ld %ld", total_lst.size(), time_lst.size());
        RWSimulFixedSize<1u>(fifo_ss, resting, time_lst, total_lst);
    // LOGD("%ld %ld", total_lst.size(), time_lst.size());
    }

    /// flush performance data
}

#endif