#if (HAVE_OPENMP)

// #else

#include <gtest/gtest.h>

#include <omp.h>

#include "common.h"
#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

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
    static constexpr size_t kMaxPkgSize = 0x10;
    static constexpr size_t kCapacity = 0x100 * kMaxPkgSize;
    // static constexpr size_t kWrap = 16;

    template <tll::lf2::Mode prod_mode, tll::lf2::Mode cons_mode, size_t extend>
    void RWSimulFixedSize()
    {
        using namespace tll::lf2;
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq)
        {
            // tll::test::fifing<index_seq.value>
        });
    }
};

/// MPMC read write fixed size
TEST_F(RingBufferStressTest, PushThenPop)
{
    using namespace tll::lf2;
    /// prepare headers

    tll::util::CallFuncInSeq<NUM_CPU, 0u>( [&](auto index_seq) {
        LOGD("Number of threads: %ld", index_seq.value);
        ring_fifo_dd<char, true> fifo{kCapacity};
        std::vector<double> time_lst;
        std::vector<size_t> total_lst;
        // constexpr size_t kTotalWriteSize = 20 * kCapacity;
        size_t total_write_size = kCapacity / 2;
        size_t store_size = total_write_size + ((index_seq.value) * kMaxPkgSize);
        std::vector<char> store_buff[index_seq.value];
        for(int i=0; i<index_seq.value; i++)
        {
            store_buff[i].resize(store_size);
            memset(store_buff[i].data(), 0xFF, store_buff[i].size());
        }

#ifdef DUMP
        std::atomic<bool> running{true};
        std::thread dumpt{[&](){
            while(running.load())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                LOGD("%s", fifo.dump().data());
            }
        }};
#endif

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
                // LOGD("%ld\t%s", size, fifo.dump().data());
                // LOGD("%ld", pop_size);
                auto dst = store_buff[tid / 2].data() + pop_size;
                auto src = el;
                memcpy(dst, src, size);
            }, kMaxPkgSize, local_total);
            return ret;
        };

        auto resting = [](){
            std::this_thread::sleep_for(std::chrono::nanoseconds(0));
            // std::this_thread::yield();
        };

        tll::test::fifing<index_seq.value>(do_push, nullptr, [](int tid){return true;}, resting, total_write_size, time_lst, total_lst);
        LOGD("%s", fifo.dump().data());
        /// push >= kTotalWriteSize
        ASSERT_GE(total_lst[0], (total_write_size));
        // LOGD("%s", fifo.dump().data());

        tll::test::fifing<index_seq.value>(nullptr, do_pop, [](int tid){return false;}, resting, total_lst[0], time_lst, total_lst);
        LOGD("%s", fifo.dump().data());
        /// push == pop
        ASSERT_EQ(total_lst[0], total_lst[3]);
        ASSERT_EQ(fifo.ph(), fifo.pt());
        ASSERT_EQ(fifo.ch(), fifo.ct());
        ASSERT_EQ(fifo.pt(), fifo.ct());

        for(int i=0; i<index_seq.value; i++)
        {
            for(int j=0;j<store_buff[i].size(); j+=kMaxPkgSize)
            {
                // int cmp = memcmp(store_buff[i].data()+j+4, store_buff[i].data()+j+5, kMaxPkgSize - 5);
                int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1);
                if(cmp != 0)
                {
                    LOGD("(%d)\t%d:%d", i, j, cmp);
                    for(int t = 0; t<kMaxPkgSize; t++)
                        printf("%2x", store_buff[i][t + j]);
                    printf("\n");
                }
                ASSERT_EQ(cmp, 0);
            }
        }


        /// simul
        fifo.reset();

        total_write_size = kCapacity * 4;
        store_size = total_write_size + ((index_seq.value) * kMaxPkgSize);
        // std::vector<char> store_buff[index_seq.value];
        for(int i=0; i<index_seq.value; i++)
        {
            store_buff[i].resize(store_size);
            memset(store_buff[i].data(), 0xFF, store_buff[i].size());
        }

        tll::test::fifing<index_seq.value * 2>(do_push, do_pop, [](int tid){ return (tid&1);}, resting, total_write_size, time_lst, total_lst);

#ifdef DUMP
        running.store(false);
        dumpt.join();
#endif

        ASSERT_EQ(total_lst[total_lst.size() - 1], total_lst[total_lst.size() - 2]);
        ASSERT_GE(total_lst[total_lst.size() - 1], total_write_size);
        ASSERT_EQ(fifo.ph(), fifo.pt());
        ASSERT_EQ(fifo.ch(), fifo.ct());
        ASSERT_EQ(fifo.pt(), fifo.ct());

        for(int i=0; i<index_seq.value; i++)
        {
            for(int j=0;j<store_buff[i].size(); j+=kMaxPkgSize)
            {
                // int cmp = memcmp(store_buff[i].data()+j+4, store_buff[i].data()+j+5, kMaxPkgSize - 5);
                int cmp = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1);
                if(cmp != 0)
                {
                    LOGD("(%d)\t%d:%d", i, j, cmp);
                    for(int t = 0; t<kMaxPkgSize; t++)
                        printf("%2x", store_buff[i][t + j]);
                    printf("\n");
                }
                ASSERT_EQ(cmp, 0);
            }
        }
    });

    /// flush
}

#endif