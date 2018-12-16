#include <libs/log.hpp>
#include <libs/log.h>
#include <libs/Counter.hpp>
#include <unistd.h>

int main()
{
    Log _l;
    // _l.init("abc.txt", "bcd.txt", {"10.207.215.174",1002});
    // _l.deinit();
    Log::ins();
    LOGPF();
    
    {
        LOGP(LOGI);
        LOGI("hello");
    }

    {
        LOGP(LOGS);
        CLOGI("hello");
        LOGD("hello");
        LOGF("hello");
    }

    return 0;
}


