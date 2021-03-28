#include <gtest/gtest.h>

#include <omp.h>

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

struct CCFIFOTest : public ::testing::Test
{
};

TEST_F(CCFIFOTest, CBOneByte)
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

TEST_F(CCFIFOTest, CBTwoByte)
{
    tll::lf::CCFIFO<int16_t> fifo{8};
    int16_t val;
    for(size_t i=0; i < fifo.capacity()*2; i++)
    {
        EXPECT_TRUE(fifo.push((int16_t)i));
        fifo.pop(val);
        EXPECT_EQ(val, (int16_t)i);
    }
}

TEST_F(CCFIFOTest, CBFourByte)
{
    tll::lf::CCFIFO<int32_t> fifo{8};
    int32_t val;
    for(size_t i=0; i < fifo.capacity()*2; i++)
    {
        EXPECT_TRUE(fifo.push((int32_t)i));
        fifo.pop(val);
        EXPECT_EQ(val, (int32_t)i);
    }
}

TEST_F(CCFIFOTest, CBEightByte)
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

#if (HAVE_OPENMP)

struct CCFIFOStressTest : public ::testing::Test
{
};

TEST_F(CCFIFOStressTest, MPMCBuffer)
{
    tll::util::CallFuncInSeq<NUM_CPU, 0>( [&](auto index_seq)
    {
        if(index_seq.value == NUM_CPU) return;
        LOGD("%ld", index_seq.value);
        auto constexpr kNumOfThreads = index_seq.value * 2;
        constexpr size_t kTotalWriteSize = 0x40000000; /// 1Gb
        constexpr size_t kCapacity = kTotalWriteSize / 4;
        constexpr size_t kPkgSize = 4096;

        tll::lf::CCFIFO<char, true> fifo{kCapacity};
        std::vector<char> store_buff[index_seq.value];

        for(int i=0; i<index_seq.value; i++)
        {
            store_buff[i].resize(kTotalWriteSize * (index_seq.value));
            memset(store_buff[i].data(), 0, store_buff[i].size());
        }
        std::atomic<int> prod_completed{0};
        std::atomic<size_t> total_push_size{0}, total_pop_size{0};
        tll::time::Counter<> counter;
        #pragma omp parallel num_threads ( kNumOfThreads ) shared(fifo, store_buff)
        {
            int tid = omp_get_thread_num();
            if(!(tid & 1)) /// Prod
            {
                LOGD("Producer: %d, cpu: %d", tid, sched_getcpu());
                char i=0;
                for(;total_push_size.load(std::memory_order_relaxed) < (kTotalWriteSize);)
                {
                    size_t ws = fifo.push([&fifo, i](size_t id, size_t size) {
                        auto dst = fifo.elemAt(id);
                        memset(dst, i, size);
                    }, kPkgSize);
                    if(ws)
                    {
                        total_push_size.fetch_add(ws, std::memory_order_relaxed);
                        (++i);
                    }
                    else
                    {
                        /// overrun
                        std::this_thread::yield();
                    }
                }

                prod_completed.fetch_add(1, std::memory_order_relaxed);
            }
            else /// Cons
            {
                LOGD("Consumer: %d, cpu: %d", tid, sched_getcpu());
                size_t pop_size=0;
                for(;prod_completed.load(std::memory_order_relaxed) < index_seq.value /*- (thread_num + 1) / 2*/
                    || total_push_size.load(std::memory_order_relaxed) > total_pop_size.load(std::memory_order_relaxed);)
                {
                    size_t ps = fifo.pop([&fifo, &store_buff, &pop_size, tid](size_t id, size_t size) {
                        auto dst = store_buff[tid/2].data() + pop_size;
                        auto src = fifo.elemAt(id);
                        memcpy(dst, src, size);
                    }, kPkgSize);

                    if(ps > 0)
                    {
                        pop_size += ps;
                        total_pop_size.fetch_add(ps, std::memory_order_relaxed);
                    }
                    else
                    {
                        /// underrun
                        std::this_thread::yield();
                    }
                }
            }
        }

        tll::cc::dumpStat<>(fifo.stat(), counter.elapse().count());
        tll::cc::Stat stat = fifo.stat();
        ASSERT_EQ(total_push_size.load(std::memory_order_relaxed), total_pop_size.load(std::memory_order_relaxed));

        for(int i=0; i<index_seq.value; i++)
        {
            for(int j=0;j<store_buff[i].size(); j+=kPkgSize)
                ASSERT_TRUE(memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kPkgSize - 1) == 0);
        }
    });
}

// TEST_F(CCFIFOStressTest, MPSCBuffer)
// {
//     tll::lf::CCFIFO<char, true> fifo{0x100000};
//     auto constexpr num_of_threads = index_seq.value;
//     std::vector<char> store_buff[num_of_threads];

//     #pragma omp parallel num_threads ( num_of_threads ) shared(fifo, store_buff, temp_data)
//     {
//         int tid = omp_get_thread_num();
//         if(tid < num_of_threads) /// Prod
//         {
//         }
//         else /// Cons
//         {
//         }
//     }
// }

// TEST_F(CCFIFOStressTest, MPMCBuffer)
// {
//     tll::util::CallFuncInSeq<NUM_CPU, 0>( [&](auto index_seq)
//     {
//         LOGD("%ld", index_seq.value);
//         auto constexpr kNumOfThreads = index_seq.value * 2;
//         constexpr size_t kTotalWriteSize = 0x1000000;
//         constexpr size_t kCapacity = kTotalWriteSize / 4;
//         constexpr size_t kPkgSize = 4096;

//         tll::lf::CCFIFO<char, true> fifo{kCapacity};
//         std::vector<char> store_buff[index_seq.value];

//         for(int i=0; i<index_seq.value; i++)
//         {
//             store_buff[i].resize(kTotalWriteSize * (index_seq.value));
//             memset(store_buff[i].data(), 0, store_buff[i].size());
//         }
//         std::atomic<int> prod_completed{0};
//         std::atomic<size_t> total_push_size{0}, total_pop_size{0};

//         #pragma omp parallel num_threads ( kNumOfThreads ) shared(fifo, store_buff)
//         {
//             int tid = omp_get_thread_num();
//             if(!(tid & 1)) /// Prod
//             {
//                 LOGD("Producer: %d, cpu: %d", tid, sched_getcpu());
//                 char i=0;
//                 for(;total_push_size.load(std::memory_order_relaxed) < (kTotalWriteSize);)
//                 {
//                     bool ws = fifo.enQueue([&fifo, &store_buff, &i](size_t id, size_t size) {
//                         std::vector<char> *dst = fifo.elemAt(id);
//                         *dst = store_buff[i];
//                     });

//                     if(ws)
//                     {
//                         total_push_size.fetch_add(kPkgSize, std::memory_order_relaxed);
//                         (++i);
//                     }
//                     else
//                     {
//                         /// overrun
//                         std::this_thread::yield();
//                     }
//                 }

//                 prod_completed.fetch_add(1, std::memory_order_relaxed);
//             }
//             else /// Cons
//             {
//                 LOGD("Consumer: %d, cpu: %d", tid, sched_getcpu());
//                 size_t pop_size=0;
//                 for(;prod_completed.load(std::memory_order_relaxed) < index_seq.value /*- (thread_num + 1) / 2*/
//                     || total_push_size.load(std::memory_order_relaxed) > total_pop_size.load(std::memory_order_relaxed);)
//                 {
//                     size_t ps = fifo.pop([&fifo, &store_buff, &pop_size, tid](size_t id, size_t size) {
//                         auto dst = store_buff[tid/2].data() + pop_size;
//                         auto src = fifo.elemAt(id);
//                         memcpy(dst, src, size);
//                     }, kPkgSize);

//                     if(ps > 0)
//                     {
//                         pop_size += kPkgSize;
//                         total_pop_size.fetch_add(kPkgSize, std::memory_order_relaxed);
//                     }
//                     else
//                     {
//                         /// underrun
//                         std::this_thread::yield();
//                     }
//                 }
//             }
//         }

//         tll::cc::dumpStat<>(fifo.stat(), 0);
//         tll::cc::Stat stat = fifo.stat();
//         ASSERT_EQ(total_push_size.load(std::memory_order_relaxed), total_pop_size.load(std::memory_order_relaxed));

//         for(int i=0; i<index_seq.value; i++)
//         {
//             for(int j=0;j<store_buff[i].size(); j+=kPkgSize)
//                 ASSERT_TRUE(memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kPkgSize - 1) == 0);
//         }
//     });
// }
#endif

int unittests(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}