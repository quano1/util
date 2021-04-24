#include <gtest/gtest.h>

#include <omp.h>

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

struct ccfifoBufferBasicTest : public ::testing::Test
{
    template <typename elem_t, tll::lf2::Mode prod_mode, tll::lf2::Mode cons_mode>
    void RWInSequencePrimitive(size_t loop)
    {
        // LOGD("[%ld] sizeof elem_t: %ld", loop, sizeof(elem_t));
        tll::lf2::ccfifo<elem_t, prod_mode, cons_mode> fifo;
        elem_t val = -1;
        size_t ret = -1;
        tll::cc::Callback<elem_t> do_nothing = [](const elem_t*, size_t){};
        auto pushCb = [](elem_t *el, size_t sz, elem_t val)
        {
            for(int i=0; i<sz; i++) *(el+i) = val;
        };
        // LOGD("reserve not power of 2");
        fifo.reserve(7, 0x400);
        ASSERT_EQ(fifo.capacity(), 8);
        // LOGD("simple push/pop");
        for(size_t i=0; i < fifo.capacity()*loop; i++)
        {
            /// https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
            ret = fifo.push2( (elem_t)i );
            EXPECT_EQ(ret, 1);
            fifo.pop(val);
            EXPECT_EQ(val, (elem_t)i);
        }
        fifo.reset();
        // LOGD("fill");
        for(size_t i=0; i < fifo.capacity(); i++) 
            fifo.push2((elem_t)i);
        // LOGD("overrun");
        EXPECT_FALSE(fifo.empty());
        ret = fifo.push2((elem_t)val);
        EXPECT_EQ(ret, 0);
        // LOGD("pop all");
        ret = fifo.pop(do_nothing, -1);
        EXPECT_EQ(ret, fifo.capacity());
        // LOGD("underrun");
        EXPECT_TRUE(fifo.empty());
        EXPECT_EQ(fifo.pop(val), 0);
        // LOGD("push/pop odd size");
        fifo.reset();
        constexpr size_t kPushSize = 3;
        for(size_t i=0; i < fifo.capacity()*loop; i++)
        {
            ret = fifo.push2(pushCb, kPushSize, (elem_t)i);
            EXPECT_EQ(ret, kPushSize);

            ret = fifo.pop([i](const elem_t *el, size_t sz){
                EXPECT_EQ(*(el), (elem_t)i);
                EXPECT_EQ(memcmp(el, el + 1, (sz - 1) * sizeof(elem_t)), 0);
            }, -1);
            EXPECT_EQ(ret, kPushSize);
        }
    }

    template <tll::lf2::Mode prod_mode, tll::lf2::Mode cons_mode>
    void RWContiguously()
    {
        using namespace tll::lf2;
        typedef int8_t elem_t;
        ccfifo<elem_t, prod_mode, cons_mode, true> fifo{8};
        tll::cc::Callback<elem_t> do_nothing = [](const elem_t*, size_t){};
        size_t capa = fifo.capacity();
        size_t ret = -1;
        elem_t val;

        fifo.push2((elem_t)0);
        fifo.pop(val);
        /// Now the fifo is empty.
        /// But should not be able to push capacity size
        EXPECT_EQ(fifo.push2(do_nothing, capa), 0);
        /// Can only pushing the reset of the buffer.
        EXPECT_EQ(fifo.push2(do_nothing, capa - 1), capa - 1);
        /// reset everything
        fifo.reset();
        fifo.push2(do_nothing, capa);
        fifo.pop(do_nothing, capa/2);
        /// wrapped perfectly
        fifo.push2(do_nothing, capa/2);
        /// should having the capacity elems
        EXPECT_EQ(fifo.size(), capa);
        /// Should only be able to pop capa/2 due to wrapped (contiguously)
        EXPECT_EQ(fifo.pop(do_nothing, -1), capa/2);
        EXPECT_EQ(fifo.pop(do_nothing, -1), capa/2);
    }
};

TEST_F(ccfifoBufferBasicTest, PrimitiveTypeSS)
{
    using namespace tll::lf2;
    constexpr size_t kLoop = 0x10;
    RWInSequencePrimitive<int8_t, mode::sparse, mode::sparse>(kLoop);
    RWInSequencePrimitive<int16_t, mode::sparse, mode::sparse>(kLoop);
    RWInSequencePrimitive<int32_t, mode::sparse, mode::sparse>(kLoop);
    RWInSequencePrimitive<int64_t, mode::sparse, mode::sparse>(kLoop);
}

TEST_F(ccfifoBufferBasicTest, PrimitiveTypeDD)
{
    using namespace tll::lf2;
    constexpr size_t kLoop = 0x10;

    RWInSequencePrimitive<int8_t, mode::dense, mode::dense>(kLoop);
    RWInSequencePrimitive<int16_t, mode::dense, mode::dense>(kLoop);
    RWInSequencePrimitive<int32_t, mode::dense, mode::dense>(kLoop);
    RWInSequencePrimitive<int64_t, mode::dense, mode::dense>(kLoop);
}

TEST_F(ccfifoBufferBasicTest, PrimitiveTypeSD)
{
    using namespace tll::lf2;
    constexpr size_t kLoop = 0x10;

    RWInSequencePrimitive<int8_t, mode::sparse, mode::dense>(kLoop);
    RWInSequencePrimitive<int16_t, mode::sparse, mode::dense>(kLoop);
    RWInSequencePrimitive<int32_t, mode::sparse, mode::dense>(kLoop);
    RWInSequencePrimitive<int64_t, mode::sparse, mode::dense>(kLoop);
}

TEST_F(ccfifoBufferBasicTest, PrimitiveTypeDS)
{
    using namespace tll::lf2;
    constexpr size_t kLoop = 0x10;
    RWInSequencePrimitive<int8_t, mode::dense, mode::sparse>(kLoop);
    RWInSequencePrimitive<int16_t, mode::dense, mode::sparse>(kLoop);
    RWInSequencePrimitive<int32_t, mode::dense, mode::sparse>(kLoop);
    RWInSequencePrimitive<int64_t, mode::dense, mode::sparse>(kLoop);
}

TEST_F(ccfifoBufferBasicTest, Contiguously)
{
    using namespace tll::lf2;

    RWContiguously<mode::sparse, mode::sparse>();
    RWContiguously<mode::dense, mode::dense>();
    RWContiguously<mode::dense, mode::sparse>();
    RWContiguously<mode::sparse, mode::dense>();
}


#if (HAVE_OPENMP)

/// Should run with --gtest_filter=ccfifoBufferStressTest.* --gtest_repeat=10000
struct ccfifoBufferStressTest : public ::testing::Test
{
    static void SetUpTestCase()
    {
        LOGD("Should run with --gtest_filter=ccfifoBufferStressTest.* --gtest_repeat=100000 --gtest_break_on_failure");
    }

    // static constexpr size_t kTotalWriteSize = 20 * 0x100000; /// 1 Mbs
    // static constexpr size_t kCapacity = kTotalWriteSize / 4;
    static constexpr size_t kCapacity = 0x100; /// 1 Mbs
    static constexpr size_t kTotalWriteSize = 0x10 * kCapacity;
    static constexpr size_t kMaxPkgSize = 0x3;
    // static constexpr size_t kWrap = 16;

    template <tll::lf2::Mode prod_mode, tll::lf2::Mode cons_mode, size_t extend>
    void RWSimulFixedSize()
    {
        using namespace tll::lf2;
        tll::util::CallFuncInSeq<NUM_CPU, extend>( [&](auto index_seq)
        {
            static_assert(index_seq.value > 0);
            // if(index_seq.value == 1) return;
            auto constexpr kNumOfThreads = index_seq.value * 2;
            LOGD("Number of threads: %ld", kNumOfThreads);
            constexpr size_t kStoreSize = kTotalWriteSize + ((index_seq.value) * kMaxPkgSize);
            ccfifo<char, prod_mode, cons_mode, true> fifo{kCapacity};
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
                const int kTid = omp_get_thread_num();
                if(!(kTid & 1)) /// Prod
                {
                    // LOGD("Producer: %s", tll::util::str_tidcpu().data());
                    char i=0;
                    for(;total_push_size.load(std::memory_order_relaxed) < (kTotalWriteSize);)
                    {
                        size_t ret_size = fifo.push2([](char *el, size_t size, char val) {
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
                    size_t pop_size=0;
                    for(;prod_completed.load(std::memory_order_relaxed) < index_seq.value /*- (thread_num + 1) / 2*/
                        || total_push_size.load(std::memory_order_relaxed) > total_pop_size.load(std::memory_order_relaxed);)
                    {
                        size_t ret_size = fifo.pop([&store_buff, &pop_size, &fifo, kTid](const char *el, size_t size) {
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
                // LOGD("%d Done", kTid);
                // LOGD("%s", fifo.dump().data());
            }
            double tt_time = counter.elapse().count();
            auto ttps = total_push_size.load(std::memory_order_relaxed);
#define DUMP
#ifdef DUMP
            auto stats = fifo.statistics();
            tll::cc::dumpStat<>(stats, tt_time);
            LOGD("Total time: %f (s)", tt_time);
#endif
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
TEST_F(ccfifoBufferStressTest, MPMCRWFixedSizeDD)
{
    using namespace tll::lf2;
    RWSimulFixedSize<mode::dense, mode::dense, 0u>();
}

/// MPMC read write fixed size
TEST_F(ccfifoBufferStressTest, MPMCRWFixedSizeDS)
{
    using namespace tll::lf2;
    RWSimulFixedSize<mode::dense, mode::sparse, -1u>();
}

/// MPMC read write fixed size
TEST_F(ccfifoBufferStressTest, MPMCRWFixedSizeSD)
{
    using namespace tll::lf2;
    RWSimulFixedSize<mode::sparse, mode::dense, -1u>();
}

/// MPMC read write fixed size
TEST_F(ccfifoBufferStressTest, MPMCRWFixedSizeSS)
{
    using namespace tll::lf2;
    RWSimulFixedSize<mode::sparse, mode::sparse, -1u>();
}

/// MPSC write random size
TEST_F(ccfifoBufferStressTest, MPSCWRandSize)
{
    using namespace tll::lf2;
    tll::util::CallFuncInSeq<NUM_CPU, 0u>( [&](auto index_seq)
    {
        static_assert(index_seq.value > 0);
        // if(index_seq.value == 1) return;
        constexpr auto kNumOfThreads = index_seq.value * 2;
        constexpr size_t kMul = kMaxPkgSize / kNumOfThreads;
        constexpr size_t kStoreSize = kTotalWriteSize + ((kNumOfThreads - 1) * kMaxPkgSize);
        LOGD("Number of Prods: %ld", kNumOfThreads - 1);
        ring_fifo_ds<char, true> fifo{kCapacity};
        std::vector<char> store_buff;
        store_buff.resize(kStoreSize);
        memset(store_buff.data(), 0xFF, store_buff.size());
        std::atomic<int> prod_completed{0};
        std::atomic<size_t> total_push_size{0}, total_pop_size{0};
        tll::time::Counter<> counter;
        #pragma omp parallel num_threads ( kNumOfThreads ) shared(fifo, store_buff)
        {
            const int kTid = omp_get_thread_num();
            if(kTid > 0) /// Prod
            {
                // LOGD("Producer: %d, cpu: %d", kTid, sched_getcpu());
                // uint8_t i=0;
                // size_t local_push=0;
                for(;total_push_size.load(std::memory_order_relaxed) < (kTotalWriteSize);)
                {
                    size_t ws = fifo.push2([&fifo, &total_push_size, kTid](char *el, size_t size) {
                        *el = '{';
                        *(el + size - 1) = '}';
                        memset(el + 1, (uint8_t)kTid, size - 2);
                        // LOGD("\t>%4ld/%4ld\t{%s}", size, total_push_size.load(std::memory_order_relaxed), fifo.dump().data());
                    }, (kTid * kMul) + 2);

                    if(ws)
                    {
                        total_push_size.fetch_add(ws, std::memory_order_relaxed);
                        // local_push+=ws;
                        // LOGD("pop\t%ld", ws);
                        // i = (i+1) % (kNumOfThreads-1);
                    }
                    else
                    {
                        /// overrun
                        // LOGD("\tOVERRUN");
                    }
                    // std::this_thread::yield();
                    std::this_thread::sleep_for(std::chrono::nanoseconds(0));
                }
                // LOGD("%ld", local_push);
                prod_completed.fetch_add(1, std::memory_order_relaxed);
            }
            else /// Cons
            {
                // LOGD("Consumer: %d, cpu: %d", kTid, sched_getcpu());
                size_t pop_size=0;
                for(;prod_completed.load(std::memory_order_relaxed) < (kNumOfThreads - 1)
                    || total_push_size.load(std::memory_order_relaxed) > total_pop_size.load(std::memory_order_relaxed);)
                {
                    size_t ps = fifo.pop([&store_buff, &pop_size, &fifo, kTid](const char *el, size_t size) {
                        // LOGD("\t-<%4ld\t{%s}", size, fifo.dump().data());
                        auto dst = store_buff.data() + pop_size;
                        auto src = el;
                        memcpy(dst, src, size);
                    }, -1);

                    if(ps > 0)
                    {
                        pop_size += ps;
                        total_pop_size.fetch_add(ps, std::memory_order_relaxed);
                        // LOGD("pop\t%ld", ps);
                    }
                    else
                    {
                        /// underrun
                        // LOGD("\tUNDERRUN");
                    }
                    // std::this_thread::yield();
                    std::this_thread::sleep_for(std::chrono::nanoseconds(0));
                }
            }
            // LOGD("%d Done", kTid);
            // LOGD("%ld %ld", fifo.stat().push_size, fifo.stat().pop_size);
        }
#ifdef DUMP
        double tt_time = counter.elapse().count();
        auto stats = fifo.statistics();
        tll::cc::dumpStat<>(stats, tt_time);
        LOGD("Total time: %f (s)", tt_time);
        LOGD("%s", fifo.dump().data());
#endif
        auto ttps = total_push_size.load(std::memory_order_relaxed);
        // LOGD("%ld %ld", fifo.stat().push_size, fifo.stat().pop_size);
        // LOGD("%s", fifo.dump().data());
        ASSERT_GE(ttps, (kTotalWriteSize));
        ASSERT_LE(ttps, kStoreSize);
        ASSERT_EQ(fifo.ph(), fifo.pt());
        ASSERT_EQ(fifo.ch(), fifo.ct());
        ASSERT_EQ(fifo.pt(), fifo.ct());
        ASSERT_EQ(ttps, total_pop_size.load(std::memory_order_relaxed));
        for(size_t i=0; i<ttps;)
        {
            size_t val = (uint8_t)store_buff[i+1];
            size_t size = (val * kMul);
            // ASSERT_EQ(memcmp(store_buff.data()+i+1, store_buff.data()+i+2, size - 1), 0);
            if(!val || memcmp(store_buff.data()+i+1, store_buff.data()+i+2, size - 1))
            {
                LOGD("%s", fifo.dump().data());
                LOGD("[%ld/%ld] %ld %ld", i, ttps, val, size);
                for(int i_=0; i_<ttps; i_++)
                // for(int i_=0; i_<store_buff.size(); i_++)
                {
                    if(i_ == i) printf("_");

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


struct ccfifoQueueStressTest : public ::testing::Test
{
    static void SetUpTestCase()
    {
        LOGD("Should run with --gtest_filter=ccfifoQueueStressTest.* --gtest_repeat=100000 --gtest_break_on_failure");
    }

    static constexpr size_t kCapacity = 0x100000; /// 1 Mbs
    static constexpr size_t kTotalWriteSize = 4 * kCapacity; 
    static constexpr size_t kPkgSize = 0x1000;
    static constexpr size_t kTotalWriteCount = kTotalWriteSize / kPkgSize;
    // static constexpr size_t kWrap = 16;

};

/// MPMC queue
TEST_F(ccfifoQueueStressTest, MPMC)
{
    using namespace tll::lf2;
    tll::util::CallFuncInSeq<NUM_CPU, 2>( [&](auto index_seq)
    {
        static_assert(index_seq.value > 0);
        // if(index_seq.value == 1) return;
        auto constexpr kNumOfThreads = index_seq.value * 2;
        LOGD("Number of threads: %ld", kNumOfThreads);
        constexpr size_t kStoreSize = kTotalWriteSize + ((index_seq.value) * kPkgSize);
        ring_fifo_dd<std::vector<char>, false> fifo{kCapacity / kPkgSize};
        std::vector<char> store_buff[index_seq.value];

        for(int i=0; i<index_seq.value; i++)
        {
            store_buff[i].resize(kStoreSize);
            memset(store_buff[i].data(), 0xFF, store_buff[i].size());
        }
        std::atomic<int> prod_completed{0};
        std::atomic<size_t> total_push_count{0}, total_pop_count{0};
#ifdef DUMP
        std::thread dumpt{[&fifo, &prod_completed, index_seq](){
        	while(prod_completed.load() < index_seq.value)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(100));
        		LOGD("%s", fifo.dump().data());
        	}
        }};
#endif
        tll::time::Counter<> counter;
        #pragma omp parallel num_threads ( kNumOfThreads ) shared(fifo, store_buff)
        {
            const int kTid = omp_get_thread_num();
            if(!(kTid & 1)) /// Prod
            {
                // LOGD("Producer: %s", tll::util::str_tidcpu().data());
                char i=0;
                for(;total_push_count.load(std::memory_order_relaxed) < (kTotalWriteCount);)
                {
                    bool ret = fifo.enQueue(std::vector<char>(kPkgSize, i));
                    if(ret)
                    {
                        total_push_count.fetch_add(1, std::memory_order_relaxed);
                        (++i);
                    }
                    else
                    {
                        /// overrun
                    }
                    // std::this_thread::yield();
                    std::this_thread::sleep_for(std::chrono::nanoseconds(0));
                }

                prod_completed.fetch_add(1, std::memory_order_relaxed);
            }
            else /// Cons
            {
                // LOGD("Consumer: %s", tll::util::str_tidcpu().data());
                size_t pop_count=0;
                for(;prod_completed.load(std::memory_order_relaxed) < index_seq.value /*- (thread_num + 1) / 2*/
                    || total_push_count.load(std::memory_order_relaxed) > total_pop_count.load(std::memory_order_relaxed);)
                {
                    bool ret = fifo.deQueue([&store_buff, &fifo, kTid](const std::vector<char> *el, size_t pc) {
                        // LOGD("%ld\t%s", size, fifo.dump().data());
                        auto dst = store_buff[kTid/2].data() + (pc * kPkgSize);
                        const auto src = el->data();
                        memcpy(dst, src, kPkgSize);
                        // LOGD("%ld", pc);
                    }, pop_count);

                    if(ret > 0)
                    {
                        // LOGD("%ld", ps);
                        // LOGD("%ld\t%s", ret_size, fifo.dump().data());
                        pop_count++;
                        total_pop_count.fetch_add(1, std::memory_order_relaxed);
                    }
                    else
                    {
                        /// underrun
                    }
                    // std::this_thread::yield();
                    std::this_thread::sleep_for(std::chrono::nanoseconds(0));
                }
            }
            // LOGD("%d Done", kTid);
            // LOGD("%s", fifo.dump().data());
        }
#ifdef DUMP
        double tt_time = counter.elapse().count();
        dumpt.join();
        // auto stats = fifo.statistics();
        // tll::cc::dumpStat<>(stats, tt_time);
        LOGD("Total time: %f (s)", tt_time);
        LOGD("%s", fifo.dump().data());
#endif
        auto ttps = total_push_count.load(std::memory_order_relaxed);
        ASSERT_GE(ttps, (kTotalWriteCount));
        // ASSERT_LE(ttps, kStoreSize);
        ASSERT_EQ(fifo.ph(), fifo.pt());
        ASSERT_EQ(fifo.ch(), fifo.ct());
        ASSERT_EQ(fifo.pt(), fifo.ct());
        ASSERT_EQ(ttps, total_pop_count.load(std::memory_order_relaxed));

        for(int i=0; i<index_seq.value; i++)
        {
            for(int j=0;j<store_buff[i].size(); j+=kPkgSize)
                ASSERT_EQ(memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kPkgSize - 1), 0);
        }
    }); /// CallFuncInSeq
}

#endif

int unittests(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}