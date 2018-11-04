#include <libs/log.hpp>
#include <libs/Counter.hpp>

int main()
{
    Log _l;
    // _l.init("abc.txt", "bcd.txt", {"10.207.215.174",1002});
    // _l.deinit();

    LOGP(MAIN);

    LOGI("hello");
    CLOGI("hello");
    LOGD("hello");
    LOGW("hello");
    LOGF("hello");

    

    return 0;
}

