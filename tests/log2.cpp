#include <LMngr.hpp>

extern LogMngr *gpLog;

int do_smt(int aLoop, int aThreadNums, int aDelay)
{
    TRACE(THREADING);
    std::thread ts[aThreadNums];
    for (auto &t : ts)
        t = std::thread([aLoop,aDelay]()
    {
        // logger.reg_ctx("do");
        TRACE(Thread);
        for(int i=0; i<aLoop; i++)
        {
            LOGI("");
            std::this_thread::sleep_for(std::chrono::microseconds(aDelay));
        }
    });
    
    for(auto &t : ts) t.join();
}
