/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#include <gtest/gtest.h>
// #include <tests/third-party/boostorg/lockfree/include/boost/lockfree/queue.hpp>
#include <tests/third-party/concurrentqueue/concurrentqueue.h>
#include <omp.h>

#include <tll.h>

#if 0
#define UT_LOGD     LOGD
#else
#define UT_LOGD(...)
#endif

using namespace tll::lf;


struct RingBufferTest : public ::testing::Test
{
    template <typename FIFO>
    void RWSequence(size_t loop)
    {
        using namespace tll::lf;
        using Elem_t = typename FIFO::Elem_t;
        FIFO fifo;
        LOGVB("[%ld] sizeof Elem_t: %ld", loop, sizeof(Elem_t));
        Elem_t val = -1;
        size_t ret = -1;
        Callback<Elem_t> do_nothing = [](const Elem_t*, size_t){};
        auto pushCb = [](Elem_t *el, size_t sz, Elem_t val)
        {
            for(int i=0; i<sz; i++) *(el+i) = val;
        };
        LOGVB("reserve not power of 2");
        fifo.reserve(7, 0x400);
        ASSERT_EQ(fifo.capacity(), 8);
        LOGVB("%s", fifo.dump().data());
        LOGVB("simple push/pop: %ld", fifo.capacity()*loop);
        for(size_t i=0; i < fifo.capacity()*loop; i++)
        {
            /// https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
            ret = fifo.push( (Elem_t)i );
            EXPECT_EQ(ret, 1);
            fifo.pop(val);
            EXPECT_EQ(val, (Elem_t)i);
        }
        LOGVB("%s", fifo.dump().data());
        LOGVB("fill");
        fifo.reset();
        std::vector<Elem_t> buff(fifo.capacity(), 0);
        for(size_t i=0; i < fifo.capacity(); i++)
            buff[i] = i;

        fifo.push(buff.data(), buff.size());

        LOGVB("%s", fifo.dump().data());
        LOGVB("overrun");
        EXPECT_FALSE(fifo.empty());
        ret = fifo.push((Elem_t)val);
        EXPECT_EQ(ret, 0);
        LOGVB("%s", fifo.dump().data());
        LOGVB("pop all");
        ret = fifo.popCb(do_nothing, -1);
        EXPECT_EQ(ret, fifo.capacity());
        LOGVB("%s", fifo.dump().data());
        LOGVB("underrun");
        EXPECT_TRUE(fifo.empty());
        EXPECT_EQ(fifo.pop(val), 0);
        LOGVB("%s", fifo.dump().data());
        LOGVB("push/pop odd size");
        fifo.reset();
        constexpr size_t kPushSize = 3;
        for(size_t i=0; i < fifo.capacity()*loop; i++)
        {
            // ret = fifo.pushCb(pushCb, kPushSize, (Elem_t)i);
            std::vector<Elem_t> outbuff(kPushSize, 0);
            ret = fifo.push(buff.data(), kPushSize);
            EXPECT_EQ(ret, kPushSize);

            // ret = fifo.popCb([&](const Elem_t *el, size_t sz){
            //     ASSERT_EQ(sz, kPushSize);
            //     EXPECT_EQ(*(el), (Elem_t)i);
            //     EXPECT_EQ(memcmp(el, el + 1, (sz - 1) * sizeof(Elem_t)), 0);
            // }, -1);
            ret = fifo.pop(outbuff.data(), kPushSize);
            EXPECT_EQ(memcmp(outbuff.data(), buff.data(), kPushSize), 0);
            EXPECT_EQ(ret, kPushSize);
        }
        LOGVB("%s", fifo.dump().data());
    }

    template <typename FIFO>
    void RWContiguously()
    {
        using namespace tll::lf;
        typedef int8_t Elem_t;
        FIFO fifo{8};
        Callback<Elem_t> do_nothing = [](const Elem_t*, size_t){};
        size_t capa = fifo.capacity();
        size_t ret = -1;
        Elem_t val;

        fifo.push((Elem_t)0);
        fifo.pop(val);
        /// Now the fifo is empty.
        /// But should not be able to push capacity size
        EXPECT_EQ(fifo.pushCb(do_nothing, capa), 0);
        /// Can only pushing the reset of the buffer.
        EXPECT_EQ(fifo.pushCb(do_nothing, capa - 1), capa - 1);
        /// reset everything
        fifo.reset();
        fifo.pushCb(do_nothing, capa);
        fifo.popCb(do_nothing, capa/2);
        /// wrapped perfectly
        fifo.pushCb(do_nothing, capa/2);
        /// should having the capacity elems
        EXPECT_EQ(fifo.size(), capa);
        /// Should only be able to pop capa/2 due to wrapped (contiguously)
        EXPECT_EQ(fifo.popCb(do_nothing, -1), capa/2);
        EXPECT_EQ(fifo.popCb(do_nothing, -1), capa/2);
    }
}; /// class RingBufferTest

TEST_F(RingBufferTest, RWSequenceSS)
{
    using namespace tll::lf;
    constexpr size_t kLoop = 0x10;
    RWSequence<RingBufferSS<int8_t>>(kLoop);
    RWSequence<RingBufferSS<int16_t>>(kLoop);
    RWSequence<RingBufferSS<int32_t>>(kLoop);
    RWSequence<RingBufferSS<int64_t>>(kLoop);
}

TEST_F(RingBufferTest, RWSequenceDD)
{
    using namespace tll::lf;
    constexpr size_t kLoop = 0x10;
    RWSequence<RingBufferDD<int8_t>>(kLoop);
    RWSequence<RingBufferDD<int16_t>>(kLoop);
    RWSequence<RingBufferDD<int32_t>>(kLoop);
    RWSequence<RingBufferDD<int64_t>>(kLoop);
}

TEST_F(RingBufferTest, RWSequenceSD)
{
    using namespace tll::lf;
    constexpr size_t kLoop = 0x10;
    RWSequence<RingBufferSD<int8_t>>(kLoop);
    RWSequence<RingBufferSD<int16_t>>(kLoop);
    RWSequence<RingBufferSD<int32_t>>(kLoop);
    RWSequence<RingBufferSD<int64_t>>(kLoop);
}

TEST_F(RingBufferTest, RWSequenceDS)
{
    using namespace tll::lf;
    constexpr size_t kLoop = 0x10;
    RWSequence<RingBufferDS<int8_t>>(kLoop);
    RWSequence<RingBufferDS<int16_t>>(kLoop);
    RWSequence<RingBufferDS<int32_t>>(kLoop);
    RWSequence<RingBufferDS<int64_t>>(kLoop);
}

TEST_F(RingBufferTest, RWContiguously)
{
    using namespace tll::lf;

    RWContiguously<RingBufferSS<int8_t>>();
    RWContiguously<RingBufferDD<int8_t>>();
    RWContiguously<RingBufferDS<int8_t>>();
    RWContiguously<RingBufferSD<int8_t>>();
}

///

struct RingQueueTest : public ::testing::Test
{
    template <typename FIFO>
    void RWSequence(size_t loop)
    {
        using namespace tll::lf;
        using Elem_t = typename FIFO::Elem_t;
        FIFO fifo;
        LOGVB("[%ld] sizeof Elem_t: %ld", loop, sizeof(Elem_t));
        Elem_t val = -1;
        size_t ret = -1;
        tll::lf::Callback<Elem_t> do_nothing = [&fifo](const Elem_t*, size_t){};

        LOGVB("reserve not power of 2");
        fifo.reserve(7, 0x400);
        ASSERT_EQ(fifo.capacity(), 8);
        LOGVB("%s", fifo.dump().data());
        LOGVB("simple push/pop: %ld", fifo.capacity()*loop);
        for(size_t i=0; i < fifo.capacity()*loop; i++)
        {
            /// https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
            ret = fifo.push( (Elem_t)i );
            EXPECT_EQ(ret, 1);
            fifo.pop(val);
            EXPECT_EQ(val, (Elem_t)i);
        }
        LOGVB("%s", fifo.dump().data());
        LOGVB("fill");
        fifo.reset();
        for(size_t i=0; i < fifo.capacity(); i++)
        {
            fifo.push((Elem_t)i);
        }
        LOGVB("%s", fifo.dump().data());
        LOGVB("overrun");
        EXPECT_FALSE(fifo.empty());
        ret = fifo.push((Elem_t)val);
        EXPECT_EQ(ret, 0);
        LOGVB("%s", fifo.dump().data());
        LOGVB("pop all");
        ret = fifo.popCb(do_nothing, -1);
        EXPECT_EQ(ret, fifo.capacity());
        LOGVB("%s", fifo.dump().data());
        LOGVB("underrun");
        EXPECT_TRUE(fifo.empty());
        EXPECT_EQ(fifo.pop(val), 0);
        LOGVB("%s", fifo.dump().data());

        LOGVB("push/pop odd size");
        fifo.reset();
        constexpr size_t kPushSize = 3;
        auto pushCb = [](Elem_t *el, size_t el_left, Elem_t val)
        {
            *el = val;
        };
        for(size_t i=0; i < fifo.capacity()*loop; i++)
        {
            ret = fifo.pushCb(pushCb, kPushSize, (Elem_t)i);
            EXPECT_EQ(ret, kPushSize);

            ret = fifo.popCb([&](const Elem_t *el, size_t el_left){
                EXPECT_EQ(*(el), (Elem_t)i);
            }, -1);
            EXPECT_EQ(ret, kPushSize);
        }
        LOGVB("%s", fifo.dump().data());
    }

    template <typename FIFO>
    void RWWrapped()
    {
        using namespace tll::lf;
        typedef int8_t Elem_t;
        FIFO fifo{8};
        tll::lf::Callback<Elem_t> do_nothing = [](const Elem_t*, size_t){};
        size_t capa = fifo.capacity();
        size_t ret = -1;
        Elem_t val;

        fifo.push((Elem_t)0);
        fifo.pop(val);
        /// Now the fifo is empty.
        /// Should be able to push capacity size
        EXPECT_EQ(fifo.pushCb(do_nothing, capa), capa);
        /// reset everything
        fifo.reset();
        fifo.pushCb(do_nothing, capa);
        fifo.popCb(do_nothing, capa/2);
        /// wrapped perfectly
        fifo.pushCb(do_nothing, capa/2);
        /// should having the capacity elems
        EXPECT_EQ(fifo.size(), capa);
        /// Should be able to pop capa even wrapped
        EXPECT_EQ(fifo.popCb(do_nothing, -1), capa);
    }
}; /// class RingQueueTest

TEST_F(RingQueueTest, RWSequenceSS)
{
    using namespace tll::lf;
    constexpr size_t kLoop = 0x10;
    RWSequence<RingQueueSS<int8_t>>(kLoop);
    RWSequence<RingQueueSS<int16_t>>(kLoop);
    RWSequence<RingQueueSS<int32_t>>(kLoop);
    RWSequence<RingQueueSS<int64_t>>(kLoop);
}

TEST_F(RingQueueTest, RWSequenceDD)
{
    using namespace tll::lf;
    constexpr size_t kLoop = 0x10;
    RWSequence<RingQueueDD<int8_t>>(kLoop);
    RWSequence<RingQueueDD<int16_t>>(kLoop);
    RWSequence<RingQueueDD<int32_t>>(kLoop);
    RWSequence<RingQueueDD<int64_t>>(kLoop);
}

TEST_F(RingQueueTest, RWSequenceSD)
{
    using namespace tll::lf;
    constexpr size_t kLoop = 0x10;
    RWSequence<RingQueueSD<int8_t>>(kLoop);
    RWSequence<RingQueueSD<int16_t>>(kLoop);
    RWSequence<RingQueueSD<int32_t>>(kLoop);
    RWSequence<RingQueueSD<int64_t>>(kLoop);
}

TEST_F(RingQueueTest, RWSequenceDS)
{
    using namespace tll::lf;
    constexpr size_t kLoop = 0x10;
    RWSequence<RingQueueDS<int8_t>>(kLoop);
    RWSequence<RingQueueDS<int16_t>>(kLoop);
    RWSequence<RingQueueDS<int32_t>>(kLoop);
    RWSequence<RingQueueDS<int64_t>>(kLoop);
}

TEST_F(RingQueueTest, RWWrapped)
{
    using namespace tll::lf;

    RWWrapped<RingQueueSS<int8_t>>();
    RWWrapped<RingQueueDD<int8_t>>();
    RWWrapped<RingQueueDS<int8_t>>();
    RWWrapped<RingQueueSD<int8_t>>();
}

#if (HAVE_OPENMP)

/// Should run with --gtest_filter=RingBufferConcurrentTest.* --gtest_repeat=10000
struct RingBufferConcurrentTest : public ::testing::Test
{
    static void SetUpTestCase()
    {
        LOGD("Should run with --gtest_filter=RingBufferConcurrentTest.* --gtest_repeat=100000 --gtest_break_on_failure");
    }

    // static constexpr size_t kTotalWriteSize = 20 * 0x100000; /// 1 Mbs
    // static constexpr size_t kCapacity = kTotalWriteSize / 4;
    static constexpr size_t kCapacity = 0x100; /// 1 Mbs
    static constexpr size_t kTotalWriteSize = 0x10 * kCapacity;
    static constexpr size_t kMaxPkgSize = 0x3;
    // static constexpr size_t kWrap = 16;

    template <tll::lf::Mode prod_mode, tll::lf::Mode cons_mode, int extend>
    void RWSimulFixedSize()
    {
        using namespace tll::lf;
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq)
        {
            static_assert(index_seq.value > 0);
            // if(index_seq.value == 1) return;
            auto constexpr kNumOfThreads = index_seq.value * 2;
            LOGD("Number of threads: %ld", kNumOfThreads);
            constexpr size_t kStoreSize = kTotalWriteSize + ((index_seq.value) * kMaxPkgSize);
            Fifo<char, prod_mode, cons_mode, true, true> fifo{kCapacity};
            std::vector<char> store_buff[index_seq.value];

            for(int i=0; i<index_seq.value; i++)
            {
                store_buff[i].resize(kStoreSize);
                memset(store_buff[i].data(), 0xFF, store_buff[i].size());
            }
            std::atomic<int> prod_completed{0};
            std::atomic<size_t> total_push_size{0}, total_pop_size{0};
            tll::util::Counter<> counter;
            #pragma omp parallel num_threads ( kNumOfThreads ) shared(fifo, store_buff)
            {
                const int kTid = omp_get_thread_num();
                if(!(kTid & 1)) /// Prod
                {
                    // LOGD("Producer: %s", tll::util::str_tidcpu().data());
                    LOGVB("Producer: %s", tll::util::str_tidcpu().data());
                    char i=0;
                    for(;total_push_size.load(std::memory_order_relaxed) < (kTotalWriteSize);)
                    {
                        size_t ret_size = fifo.pushCb([](char *el, size_t size, char val) {
                            memset(el, val, size);
                        }, kMaxPkgSize, i);
                        if(ret_size)
                        {
                            // LOGD("%ld\t%s", ret_size, fifo.dump().data());
                            total_push_size.fetch_add(ret_size, std::memory_order_relaxed);
                            (++i);
                            std::this_thread::yield();
                        }
                        else
                        {
                            /// overrun
                        }
                        // std::this_thread::sleep_for(std::chrono::nanoseconds(0));
                    }

                    prod_completed.fetch_add(1, std::memory_order_relaxed);
                }
                else /// Cons
                {
                    // LOGD("Consumer: %s", tll::util::str_tidcpu().data());
                    LOGVB("Consumer: %s", tll::util::str_tidcpu().data());
                    size_t pop_size=0;
                    for(;prod_completed.load(std::memory_order_relaxed) < index_seq.value /*- (thread_num + 1) / 2*/
                        || total_push_size.load(std::memory_order_relaxed) > total_pop_size.load(std::memory_order_relaxed);)
                    {
                        size_t ret_size = fifo.popCb([&store_buff, &pop_size, &fifo, kTid](const char *el, size_t size) {
                            // LOGD("%ld\t%s", size, fifo.dump().data());
                            auto dst = store_buff[kTid/2].data() + pop_size;
                            auto src = el;
                            memcpy(dst, src, size);
                        }, kMaxPkgSize);

                        if(ret_size > 0)
                        {
                            // LOGD("%ld", ps);
                            // LOGD("%ld\t%s", ret_size, fifo.dump().data());
                            pop_size += ret_size;
                            total_pop_size.fetch_add(ret_size, std::memory_order_relaxed);
                            std::this_thread::yield();
                        }
                        else
                        {
                            /// underrun
                        }
                        // std::this_thread::sleep_for(std::chrono::nanoseconds(0));
                    }
                }
                LOGVB("  - Done: %s", tll::util::str_tidcpu().data());
            }
            double tt_time = counter.elapse().count();
            auto ttps = total_push_size.load(std::memory_order_relaxed);
            if(gVerbose)
            {
                fifo.dumpStat(tt_time);
                LOGVB("Total time: %f (s)", tt_time);
            }
            LOGD("%ld:%ld\t%s", total_pop_size.load(), kStoreSize, fifo.dump().data());
            ASSERT_GE(ttps, (kTotalWriteSize));
            ASSERT_LE(ttps, kStoreSize);
            ASSERT_EQ(fifo.ph(), fifo.pt());
            ASSERT_EQ(fifo.ch(), fifo.ct());
            ASSERT_EQ(fifo.pt(), fifo.ct());
            ASSERT_EQ(ttps, total_pop_size.load(std::memory_order_relaxed));

            for(int i=0; i<index_seq.value; i++)
            {
                for(int j=0;j<store_buff[i].size()-1; j+=kMaxPkgSize)
                {
                    int ret = memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1);
                    if(ret != 0)
                    {
                        LOGD("[%d:%d]", i, j);
                        for(int _i=j; _i<j+kMaxPkgSize; _i++) printf("%2x:", store_buff[i][_i]);
                            printf("\n");
                        ASSERT_EQ(ret, 0);
                    }
                }
            }
        });
    }
};

/// MPMC read write fixed size
TEST_F(RingBufferConcurrentTest, MPMCRWFixedSizeDD)
{
    using namespace tll::lf;
    RWSimulFixedSize<mode::multi_dense, mode::multi_dense, 0u>();
}

/// MPMC read write fixed size
TEST_F(RingBufferConcurrentTest, MPMCRWFixedSizeDS)
{
    using namespace tll::lf;
    RWSimulFixedSize<mode::multi_dense, mode::multi_sparse, -1>();
}

/// MPMC read write fixed size
TEST_F(RingBufferConcurrentTest, MPMCRWFixedSizeSD)
{
    using namespace tll::lf;
    RWSimulFixedSize<mode::multi_sparse, mode::multi_dense, -1>();
}

/// MPMC read write fixed size
TEST_F(RingBufferConcurrentTest, MPMCRWFixedSizeSS)
{
    using namespace tll::lf;
    RWSimulFixedSize<mode::multi_sparse, mode::multi_sparse, -1>();
}

#endif /// #if (HAVE_OPENMP)