#include <libs/log.hpp>
#include <libs/log.h>
#include <libs/Counter.hpp>
#include <unistd.h>

void test()
{
    LOGT();
    LOGI("");
}

int main()
{
    Log _l;
    // _l.init("abc.txt", "bcd.txt", {"10.207.215.174",1002});
    // _l.deinit();
    LOG_INIT();
    LOGT();
    
    {
        LOGP(LOGI);
        LOGI("hello");
        test();
    }

    test();

    {
        LOGP(LOGS);
        CLOGI("hello");
        // LOGD("hello");
        LOGF("hello");
        test();
    }

    return 0;
}


