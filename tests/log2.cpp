#include <LMngr.hpp>

extern LogMngr *gpLog;

int do_smt(int aLoop, int aThreadNums, int aDelay)
{
    gpLog->reg_ctx("do_smt");
    TRACE(*gpLog, THREADING);
    std::thread ts[aThreadNums];
    for (auto &t : ts)
        t = std::thread([aLoop,aDelay]()
    {
        // gpLog->reg_ctx("sub do_smt");
        TRACE(*gpLog, Thread);
        for(int i=0; i<aLoop; i++)
        {
            LOGI(*gpLog, "");
            std::this_thread::sleep_for(std::chrono::microseconds(aDelay));
        }
    });
    
    for(auto &t : ts) t.join();
}
