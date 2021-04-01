#include <gtest/gtest.h>

#include <omp.h>

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

struct CCFIFOBufferBasicTest : public ::testing::Test
{
};

TEST_F(CCFIFOBufferBasicTest, RWInSequence)
{
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

    // TEST_F(CCFIFOBufferBasicTest, CB2Bytes)
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

    // TEST_F(CCFIFOBufferBasicTest, CB4Bytes)
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

    // TEST_F(CCFIFOBufferBasicTest, CB8Bytes)
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
}

#if (HAVE_OPENMP)

struct CCFIFOBufferStressTest : public ::testing::Test
{
    static constexpr size_t kTotalWriteSize = 0x1000;
    static constexpr size_t kCapacity = kTotalWriteSize / 4;
    static constexpr size_t kMaxPkgSize = 0x100;
    static constexpr size_t kWrap = 16;
    static constexpr size_t kMul = kMaxPkgSize / kWrap;
};

/// MPMC read write fixed size
TEST_F(CCFIFOBufferStressTest, MPMCRWFixedS)
{
    tll::util::CallFuncInSeq<NUM_CPU, 0>( [&](auto index_seq)
    {
        static_assert(index_seq.value > 0);
        if(index_seq.value == NUM_CPU) return;
        auto constexpr kNumOfThreads = index_seq.value * 2;
        LOGD("Number of threads: %ld", kNumOfThreads);
        constexpr size_t kStoreSize = kTotalWriteSize + ((index_seq.value - 1) * kMaxPkgSize);
        tll::lf::CCFIFO<char, true> fifo{kCapacity};
        std::vector<char> store_buff[index_seq.value];

        for(int i=0; i<index_seq.value; i++)
        {
            store_buff[i].resize(kStoreSize);
            memset(store_buff[i].data(), 0xFF, store_buff[i].size());
        }
        std::atomic<int> prod_completed{0};
        std::atomic<size_t> total_push_size{0}, total_pop_size{0};
        tll::time::Counter<> counter;
        #pragma omp parallel num_threads ( kNumOfThreads ) shared(fifo, store_buff)
        {
            int tid = omp_get_thread_num();
            if(!(tid & 1)) /// Prod
            {
                // LOGD("Producer: %d, cpu: %d", tid, sched_getcpu());
                char i=0;
                for(;total_push_size.load(std::memory_order_relaxed) < (kTotalWriteSize);)
                {
                    size_t ws = fifo.push([&fifo, i](size_t id, size_t size) {
                        auto dst = fifo.elemAt(id);
                        memset(dst, i, size);
                    }, kMaxPkgSize);
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
                // LOGD("Consumer: %d, cpu: %d", tid, sched_getcpu());
                size_t pop_size=0;
                for(;prod_completed.load(std::memory_order_relaxed) < index_seq.value /*- (thread_num + 1) / 2*/
                    || total_push_size.load(std::memory_order_relaxed) > total_pop_size.load(std::memory_order_relaxed);)
                {
                    size_t ps = fifo.pop([&fifo, &store_buff, &pop_size, tid](size_t id, size_t size) {
                        auto dst = store_buff[tid/2].data() + pop_size;
                        auto src = fifo.elemAt(id);
                        memcpy(dst, src, size);
                    }, kMaxPkgSize);

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
            // LOGD("%d Done", tid);
        }
#ifdef DUMP
        double tt_time = counter.elapse().count();
        tll::cc::Stat stat = fifo.stat();
        tll::cc::dumpStat<>(stat, tt_time);
        LOGD("Total time: %f (s)", tt_time);
#endif
        ASSERT_TRUE(total_push_size.load(std::memory_order_relaxed) >= (kTotalWriteSize));
        ASSERT_EQ(total_push_size.load(std::memory_order_relaxed), total_pop_size.load(std::memory_order_relaxed));

        for(int i=0; i<index_seq.value; i++)
        {
            for(int j=0;j<store_buff[i].size(); j+=kMaxPkgSize)
                ASSERT_EQ(memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1), 0);
        }
    });
}

/// MPSC write random size
TEST_F(CCFIFOBufferStressTest, MPSCWRandS)
{
    tll::util::CallFuncInSeq<NUM_CPU, 0>( [&](auto index_seq)
    {
        static_assert(index_seq.value > 0);
        if(index_seq.value == 1 || index_seq.value >= NUM_CPU) return;
        // if(index_seq.value >= NUM_CPU) return;
        auto constexpr kNumOfThreads = index_seq.value * 2;
        constexpr size_t kStoreSize = kTotalWriteSize * 2;
        LOGD("Number of Prods: %ld", kNumOfThreads - 1);
        tll::lf::CCFIFO<char, true> fifo{kCapacity};
        std::vector<char> store_buff;
        store_buff.resize(kStoreSize);
        memset(store_buff.data(), 0xFF, store_buff.size());
        std::atomic<int> prod_completed{0};
        std::atomic<size_t> total_push_size{0}, total_pop_size{0};
        tll::time::Counter<> counter;
        #pragma omp parallel num_threads ( kNumOfThreads ) shared(fifo, store_buff)
        {
            int tid = omp_get_thread_num();
            if(tid > 0) /// Prod
            {
                // LOGD("Producer: %d, cpu: %d", tid, sched_getcpu());
                uint8_t i=0;
                for(;total_push_size.load(std::memory_order_relaxed) < (kTotalWriteSize);)
                {
                    size_t ws = fifo.push([&fifo, i](size_t id, size_t size) {
                        fifo[id] = '{';
                        memset(&fifo[id + 1], i, size - 2);
                        fifo[id + size - 1] = '}';
                    }, (i+1) * (kMul) + 2);
                    if(ws)
                    {
                        total_push_size.fetch_add(ws, std::memory_order_relaxed);
                        i = (i+1) & (kWrap-1);
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
                // LOGD("Consumer: %d, cpu: %d", tid, sched_getcpu());
                size_t pop_size=0;
                for(;prod_completed.load(std::memory_order_relaxed) < (kNumOfThreads - 1)
                    || total_push_size.load(std::memory_order_relaxed) > total_pop_size.load(std::memory_order_relaxed);)
                {
                    size_t ps = fifo.pop([&fifo, &store_buff, &pop_size, tid](size_t id, size_t size) {
                        auto dst = store_buff.data() + pop_size;
                        auto src = fifo.elemAt(id);
                        memcpy(dst, src, size);
                    }, fifo.capacity());

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
            // LOGD("%d Done", tid);
        }
#ifdef DUMP
        double tt_time = counter.elapse().count();
        tll::cc::Stat stat = fifo.stat();
        tll::cc::dumpStat<>(stat, tt_time);
        LOGD("Total time: %f (s)", tt_time);
#endif
        ASSERT_EQ(total_push_size.load(std::memory_order_relaxed), total_pop_size.load(std::memory_order_relaxed));
        auto ttps = total_push_size.load(std::memory_order_relaxed);
        for(size_t i=0; i<ttps;)
        {
            size_t val = (uint8_t)store_buff[i+1];
            size_t size = (val + 1) * kMul;
            // ASSERT_EQ(memcmp(store_buff.data()+i+1, store_buff.data()+i+2, size - 1), 0);
            if(memcmp(store_buff.data()+i+1, store_buff.data()+i+2, size - 1))
            {
                LOGD("[%ld/%ld] %ld %ld", i, ttps, val, size);
                for(int i_=0; i_<ttps; i_++)
                {
                    if(store_buff[i_] == '{') printf("{");
                    else if(store_buff[i_] == '}') printf("}");
                    else printf("%1x", (uint8_t)store_buff[i_]);
                }
                printf("\n");
                ASSERT_TRUE(false);
            }
            i += size + 2;
        }
    });
}

#endif