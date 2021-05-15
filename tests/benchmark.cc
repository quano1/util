/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)
#include <limits>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <thread>
#include <utility>

#include <tests/third-party/boostorg/lockfree/include/boost/lockfree/queue.hpp>
#include <tests/third-party/concurrentqueue/concurrentqueue.h>

#include <tll.h>
#include "common.h"

void benchmark_yield()
{
    std::vector<int> thread_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;

    tll::util::CallFuncInSeq<NUM_CPU, 5>( [&](auto index_seq)
    {
        thread_lst.push_back(index_seq.value);
        constexpr size_t num_of_threads = index_seq.value;
        constexpr size_t total_write_size = 250000 * num_of_threads;
        auto do_wait = [](){
            std::this_thread::yield();
        };

        LOGD("=====================================================");
        LOGD("Number of threads: %ld\twrite count: %ld", num_of_threads, total_write_size);

        {
            boost::lockfree::queue<char> fifo{total_write_size * 2};
            auto do_push = [&](int, size_t, size_t, size_t, size_t) -> size_t { return fifo.push((char)1);};
            auto do_pop = [&](int, size_t, size_t, size_t, size_t) -> size_t { char val; return fifo.pop(val);};
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, do_wait, nullptr, total_write_size, time_lst, total_count_lst).first;

            tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, do_wait, nullptr, rts_push, time_lst, total_count_lst);
        }

        {
            moodycamel::ConcurrentQueue<char> fifo{total_write_size * 2};
            auto do_push = [&](int, size_t, size_t, size_t, size_t) -> size_t { return fifo.enqueue((char)1);};
            auto do_pop = [&](int, size_t, size_t, size_t, size_t) -> size_t { char val; return fifo.try_dequeue(val);};
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, do_wait, nullptr, total_write_size, time_lst, total_count_lst).first;
            
            tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, do_wait, nullptr, rts_push, time_lst, total_count_lst);
        }

        {
            tll::lf::ring_buffer_dd<char> fifo{total_write_size * 2, 0x10000};
            auto do_push = [&](int, size_t, size_t, size_t, size_t) -> size_t { return fifo.push((char)1);};
            auto do_pop = [&](int, size_t, size_t, size_t, size_t) -> size_t { char val; return fifo.pop(val);};
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, do_wait, nullptr, total_write_size, time_lst, total_count_lst).first;

            tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, do_wait, nullptr, rts_push, time_lst, total_count_lst);
        }

        {
            tll::lf::ring_queue_dd<char> fifo{total_write_size * 2, 0x10000};
            auto do_push = [&](int, size_t, size_t, size_t, size_t) -> size_t { return fifo.push((char)1);};
            auto do_pop = [&](int, size_t, size_t, size_t, size_t) -> size_t { char val; return fifo.pop(val);};
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, do_wait, nullptr, total_write_size, time_lst, total_count_lst).first;

            tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, do_wait, nullptr, rts_push, time_lst, total_count_lst);
        }

    });

    std::vector<std::string> cols {"boost W", "boost R", "moody W", "moody R", "tllbuffer W", "tllbuffer R", "tllqueue W", "tllqueue R"};
    tll::test::plot_data("benchmark_yield.dat", tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Relative completing time in seconds",
                  cols,
                  thread_lst,
                  time_lst
                  );

}


void benchmark_sleep()
{
    std::vector<int> thread_lst;
    std::vector<double> time_lst;
    std::vector<size_t> total_size_lst, total_count_lst;

    tll::util::CallFuncInSeq<NUM_CPU, 5>( [&](auto index_seq)
    {
        thread_lst.push_back(index_seq.value);
        constexpr size_t num_of_threads = index_seq.value;
        constexpr size_t total_write_size = 250000 * num_of_threads;
        auto do_wait = [](){
            std::this_thread::sleep_for(std::chrono::nanoseconds(0));
        };

        LOGD("=====================================================");
        LOGD("Number of threads: %ld\twrite count: %ld", num_of_threads, total_write_size);

        {
            boost::lockfree::queue<char> fifo{total_write_size * 2};
            auto do_push = [&](int, size_t, size_t, size_t, size_t) -> size_t { return fifo.push((char)1);};
            auto do_pop = [&](int, size_t, size_t, size_t, size_t) -> size_t { char val; return fifo.pop(val);};
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, do_wait, nullptr, total_write_size, time_lst, total_count_lst).first;

            tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, do_wait, nullptr, rts_push, time_lst, total_count_lst);
        }

        {
            moodycamel::ConcurrentQueue<char> fifo{total_write_size * 2};
            auto do_push = [&](int, size_t, size_t, size_t, size_t) -> size_t { return fifo.enqueue((char)1);};
            auto do_pop = [&](int, size_t, size_t, size_t, size_t) -> size_t { char val; return fifo.try_dequeue(val);};
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, do_wait, nullptr, total_write_size, time_lst, total_count_lst).first;
            
            tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, do_wait, nullptr, rts_push, time_lst, total_count_lst);
        }

        {
            tll::lf::ring_buffer_dd<char> fifo{total_write_size * 2, 0x10000};
            auto do_push = [&](int, size_t, size_t, size_t, size_t) -> size_t { return fifo.push((char)1);};
            auto do_pop = [&](int, size_t, size_t, size_t, size_t) -> size_t { char val; return fifo.pop(val);};
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, do_wait, nullptr, total_write_size, time_lst, total_count_lst).first;

            tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, do_wait, nullptr, rts_push, time_lst, total_count_lst);
        }

        {
            tll::lf::ring_queue_dd<char> fifo{total_write_size * 2, 0x10000};
            auto do_push = [&](int, size_t, size_t, size_t, size_t) -> size_t { return fifo.push((char)1);};
            auto do_pop = [&](int, size_t, size_t, size_t, size_t) -> size_t { char val; return fifo.pop(val);};
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t rts_push = tll::test::fifing<num_of_threads>(do_push, nullptr, [](int tid){return true;}, do_wait, nullptr, total_write_size, time_lst, total_count_lst).first;

            tll::test::fifing<num_of_threads>(nullptr, do_pop, [](int tid){return false;}, do_wait, nullptr, rts_push, time_lst, total_count_lst);
        }

    });

    std::vector<std::string> cols {"boost W", "boost R", "moody W", "moody R", "tllbuffer W", "tllbuffer R", "tllqueue W", "tllqueue R"};
    tll::test::plot_data("benchmark_sleep.dat", tll::util::stringFormat("Lower better (CPU: %d)", NUM_CPU), "Relative completing time in seconds",
                  cols,
                  thread_lst,
                  time_lst
                  );

}


int benchmark()
{
    benchmark_yield();
    benchmark_sleep();
    return 0;
}