#include <algorithm>
#include <vector>
#include <list>
#include <random>
#include <functional>
#define PROF_LOG
#include "../libs/log.h"


int main(int argc, char const *argv[])
{
    // std::list<int> base;
    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<int> dist(1, 1000);
    

    // for(int i=0; i<1000; i++) {
    //     base.push_back(dist(gen));
    // }

    // {
    //     TRACEF();
    //     for (int i=0; i<10; i++)
    //     {
    //         std::list<int> cont{base};
    //         cont.sort([](int a, int b) {
    //             return a < b;
    //         });
    //     }
    // }

    {
        TRACEF();
        for (int i=0; i<100000; i++)
        {
            TLL_GLOGTF();
            // std::list<int> cont{base};
            // cont.sort([](int a, int b) { TLL_GLOGD();
            //     return a < b;
            // });
        }
    }

    // for(auto &el : base) {
    //  printf("%d ", el);
    // }
    // printf("\n");
    // printf("\n");

    // for(auto &el : cont) {
    //  printf("%d ", el);
    // }


    printf("\n");

    tll::log::Node::instance().stop();

    using prof::timer;
    using prof::hdCnt;
    using prof::asyncCnt;
    using prof::dologCnt;

    LOGD("header: %f, %d:%f", timer("header").total().count(), hdCnt, timer("header").total().count()/hdCnt);
    LOGD("node::log::async: %f, %d:%f", timer("node::log::async").total().count(), asyncCnt, timer("node::log::async").total().count()/asyncCnt);

    LOGD("dolog: %f, %d:%f", timer("dolog").total().count(), dologCnt, timer("dolog").total().count()/dologCnt);
    LOGD("open: %f", timer("open").total().count());
    LOGD("close: %f", timer("close").total().count());
    LOGD("total: %f", timer.allTotal().count());

    return 0;
}