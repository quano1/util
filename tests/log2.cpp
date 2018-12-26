#include <LMngr.hpp>

extern LogMngr logger;

int do_smt(int aLoop, int aThreadNums, int aDelay)
{
    TRACE(do_smg, logger);
    std::thread ts[aThreadNums];
    for (auto &t : ts)
        t = std::thread([&logger,aLoop,aDelay]()
    {
        // logger.reg_ctx("do");
        TRACE(LOG_THREADING, logger);
        for(int i=0; i<aLoop; i++)
        {
            LOGI(logger, "");
            std::this_thread::sleep_for(std::chrono::milliseconds(aDelay));
        }
    });
    
    LOGI(logger, "");
    LOGW(logger, "");
    for(auto &t : ts) t.join();
    LOGE(logger, "");
}
