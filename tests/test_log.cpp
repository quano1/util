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
    LOGP(MAIN);
    
    {
        LOGP(LOGI);
        LOGI("hello");
    }

    {
        std::string lBuf;
        lBuf = Log::ins()->preInit();
        lBuf += "\n";
        {
            LOGP(RAW1);
            printf(lBuf.data());
        }
        {
            LOGP(RAW2);
            write(2, lBuf.data(), lBuf.size());
        }
    }

    CLOGI("hello");
    LOGD("hello");
    LOGW("hello");
    LOGF("hello");

    return 0;
}


