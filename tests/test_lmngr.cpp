#include <libs/LMngr.hpp>
#include <libs/log.hpp>

int main()
{
    LMngr log;
    log.add(new EConsole());
    log.add(new EFile("log.txt"));
    log.add(new ENetUDP("localhost", 65530));

    log.init(nullptr);

    log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    log.deInit();
    LOGD("");
    return 0;
}