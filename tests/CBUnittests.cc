#include <gtest/gtest.h>

#include <omp.h>

#include "../libs/util.h"
#include "../libs/counter.h"
#include "../libs/contiguouscircular.h"

struct ccfifoBufferBasicTest : public ::testing::Test
{
    template <typename elem_t, tll::lf2::Mode mode>
    void RWInSequencePrimitive(size_t loop)
    {
        tll::lf2::ccfifo<elem_t> fifo;
        elem_t val = -1;
        size_t ret = -1;
        tll::cc::Callback<elem_t> do_nothing = [](const elem_t*, size_t){};
        auto pushCb = [](elem_t *el, size_t sz, elem_t val)
        {
            for(int i=0; i<sz; i++) *(el+i) = val;
        };

        /// reserve not power of 2
        fifo.reserve(7);
        ASSERT_EQ(fifo.capacity(), 8);
        /// simple push/pop
        for(size_t i=0; i < fifo.capacity()*loop; i++)
        {
            /// https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
            ret = fifo.template push<mode>( (elem_t)i );
            EXPECT_EQ(ret, 1);
            fifo.template pop<mode>(val);
            EXPECT_EQ(val, (elem_t)i);
        }
        fifo.reset();
        /// fill
        for(size_t i=0; i < fifo.capacity(); i++) 
            fifo.template push<mode>((elem_t)i);
        /// overrun
        EXPECT_FALSE(fifo.empty());
        ret = fifo.template push<mode>((elem_t)val);
        EXPECT_EQ(ret, 0);
        /// pop all
        ret = fifo.template pop<mode>(do_nothing, -1);
        EXPECT_EQ(ret, fifo.capacity());
        /// underrun
        EXPECT_TRUE(fifo.empty());
        EXPECT_EQ(fifo.template pop<mode>(val), 0);
        /// push/pop odd size
        fifo.reset();
        constexpr size_t kPushSize = 3;
        for(size_t i=0; i < fifo.capacity()*loop; i++)
        {
            ret = fifo.template push<mode>(pushCb, kPushSize, (elem_t)i);
            EXPECT_EQ(ret, kPushSize);

            ret = fifo.template pop<mode>([i](const elem_t *el, size_t sz){
                EXPECT_EQ(*(el), (elem_t)i);
                EXPECT_EQ(memcmp(el, el + 1, (sz - 1) * sizeof(elem_t)), 0);
            }, -1);
            EXPECT_EQ(ret, kPushSize);
        }
    }

    template <tll::lf2::Mode mode>
    void RWContiguously()
    {
        using namespace tll::lf2;
        typedef int8_t elem_t;
        ccfifo<elem_t, true> fifo{8};
        tll::cc::Callback<elem_t> do_nothing = [](const elem_t*, size_t){};
        size_t capa = fifo.capacity();
        size_t ret = -1;
        elem_t val;

        fifo.push<mode>((elem_t)0);
        fifo.pop<mode>(val);
        /// Now the fifo is empty.
        /// But should not be able to push capacity size
        EXPECT_EQ(fifo.push<mode>(do_nothing, capa), 0);
        /// Can only pushing the reset of the buffer.
        EXPECT_EQ(fifo.push<mode>(do_nothing, capa - 1), capa - 1);
        /// reset everything
        fifo.reset();
        fifo.push<mode>(do_nothing, capa);
        fifo.pop<mode>(do_nothing, capa/2);
        /// wrapped perfectly
        fifo.push<mode>(do_nothing, capa/2);
        /// should having the capacity elems
        EXPECT_EQ(fifo.size(), capa);
        /// Should only be able to pop capa/2 due to wrapped (contiguously)
        EXPECT_EQ(fifo.pop<mode>(do_nothing, -1), capa/2);
        EXPECT_EQ(fifo.pop<mode>(do_nothing, -1), capa/2);
    }
};

TEST_F(ccfifoBufferBasicTest, PrimitiveType)
{
    using namespace tll::lf2;
    constexpr size_t kLoop = 0x10;
    RWInSequencePrimitive<int8_t, mode::low_load>(kLoop);
    RWInSequencePrimitive<int16_t, mode::low_load>(kLoop);
    RWInSequencePrimitive<int32_t, mode::low_load>(kLoop);
    RWInSequencePrimitive<int64_t, mode::low_load>(kLoop);

    RWInSequencePrimitive<int8_t, mode::high_load>(kLoop);
    RWInSequencePrimitive<int16_t, mode::high_load>(kLoop);
    RWInSequencePrimitive<int32_t, mode::high_load>(kLoop);
    RWInSequencePrimitive<int64_t, mode::high_load>(kLoop);
}

TEST_F(ccfifoBufferBasicTest, Contiguously)
{
    using namespace tll::lf2;
    RWContiguously<mode::low_load>();
    RWContiguously<mode::high_load>();
}


#if (HAVE_OPENMP)

/// Should run with --gtest_filter=ccfifoBufferStressTest.* --gtest_repeat=10000
struct ccfifoBufferStressTest : public ::testing::Test
{
    static void SetUpTestCase()
    {
        LOGD("Should run with --gtest_filter=ccfifoBufferStressTest.* --gtest_repeat=100000 --gtest_break_on_failure");
    }

    static constexpr size_t kTotalWriteSize = 4 * 0x100000; /// 1 Mbs
    static constexpr size_t kCapacity = kTotalWriteSize / 4;
    static constexpr size_t kMaxPkgSize = 0x1000;
    // static constexpr size_t kWrap = 16;

};

/// MPMC read write fixed size
TEST_F(ccfifoBufferStressTest, MPMCRWFixedSize)
{
    using namespace tll::lf2;
    tll::util::CallFuncInSeq<NUM_CPU, -1>( [&](auto index_seq)
    {
        static_assert(index_seq.value > 0);
        // if(index_seq.value == 1) return;
        auto constexpr kNumOfThreads = index_seq.value * 2;
        LOGD("Number of threads: %ld", kNumOfThreads);
        constexpr size_t kStoreSize = kTotalWriteSize + ((index_seq.value - 1) * kMaxPkgSize);
        ccfifo<char, true> fifo{kCapacity};
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
                    size_t ret_size = fifo.push<mode::low_load>([i](char *el, size_t size) {
                        memset(el, i, size);
                    }, kMaxPkgSize);
                    if(ret_size)
                    {
                        // LOGD("%ld\t%s", ret_size, fifo.dump().data());
                        total_push_size.fetch_add(ret_size, std::memory_order_relaxed);
                        (++i);
                    }
                    else
                    {
                        /// overrun
                    }
                    std::this_thread::yield();
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
                    size_t ret_size = fifo.pop<mode::low_load>([&store_buff, &pop_size, &fifo, kTid](const char *el, size_t size) {
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
                    }
                    else
                    {
                        /// underrun
                    }
                    std::this_thread::yield();
                }
            }
            // LOGD("%d Done", kTid);
            // LOGD("%s", fifo.dump().data());
        }
        double tt_time = counter.elapse().count();
#ifdef DUMP
        tll::cc::Stat stat = fifo.stat();
        tll::cc::dumpStat<>(stat, tt_time);
        LOGD("Total time: %f (s)", tt_time);
#endif
        LOGD("%s", fifo.dump().data());
        auto ttps = total_push_size.load(std::memory_order_relaxed);
        ASSERT_GE(ttps, (kTotalWriteSize));
        ASSERT_LE(ttps, kStoreSize);
        ASSERT_EQ(fifo.ph(), fifo.pt());
        ASSERT_EQ(fifo.ch(), fifo.ct());
        ASSERT_EQ(fifo.pt(), fifo.ct());
        ASSERT_EQ(ttps, total_pop_size.load(std::memory_order_relaxed));

        for(int i=0; i<index_seq.value; i++)
        {
            for(int j=0;j<store_buff[i].size(); j+=kMaxPkgSize)
                ASSERT_EQ(memcmp(store_buff[i].data()+j, store_buff[i].data()+j+1, kMaxPkgSize - 1), 0);
        }
    });
}


/// MPSC write random size
TEST_F(ccfifoBufferStressTest, MPSCWRandSize)
{
    using namespace tll::lf2;
    tll::util::CallFuncInSeq<NUM_CPU, 2>( [&](auto index_seq)
    {
        static_assert(index_seq.value > 0);
        // if(index_seq.value == 1) return;
        constexpr auto kNumOfThreads = index_seq.value * 2;
        constexpr size_t kMul = kMaxPkgSize / kNumOfThreads;
        constexpr size_t kStoreSize = kTotalWriteSize + ((kNumOfThreads - 1) * kMaxPkgSize);
        LOGD("Number of Prods: %ld", kNumOfThreads - 1);
        tll::lf2::ccfifo<char, true> fifo{kCapacity};
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
                    size_t ws = fifo.push<tll::lf2::Mode::kHL>([&fifo, &total_push_size, kTid](char *el, size_t size) {
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
                    std::this_thread::yield();
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
                    size_t ps = fifo.pop<mode::low_load>([&store_buff, &pop_size, &fifo, kTid](const char *el, size_t size) {
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
                    std::this_thread::yield();
                }
            }
            // LOGD("%d Done", kTid);
            // LOGD("%ld %ld", fifo.stat().push_size, fifo.stat().pop_size);
        }
#ifdef DUMP
        double tt_time = counter.elapse().count();
        tll::cc::Stat stat = fifo.stat();
        tll::cc::dumpStat<>(stat, tt_time);
        LOGD("Total time: %f (s)", tt_time);
#endif
        LOGD("%s", fifo.dump().data());
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

#endif