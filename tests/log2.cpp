#include <LogMngr.hpp>

extern llt::LogMngr *gpLog;

class Base1
{
public:
    Base1() = default;
    virtual ~Base1() = default;

    void do_smt()
    {
        TRACE_THIS(gpLog, _);
    }

private:

};

class Base2
{
public:
    Base2() = default;
    virtual ~Base2() = default;

    void do_smt()
    {
        TRACE_THIS(gpLog, _);
    }

private:

};


void do_smt(int aLoop, int aThreadNums, int aDelay)
{
    std::thread ts[aThreadNums];
    {
        TRACE(gpLog, THREADING);
        for (auto &t : ts)
            t = std::thread([aLoop,aDelay]()
        {
            // gpLog->reg_ctx("sub do_smt");
            TRACE(gpLog, Thread);
            Base1 b11, b12;
            Base2 b21, b22;
            for(int i=0; i<aLoop; i++)
            {
                LOGI(gpLog, "");
                b11.do_smt();
                b12.do_smt();
                b21.do_smt();
                b22.do_smt();
                std::this_thread::sleep_for(std::chrono::microseconds(aDelay));
            }
        });
    }
        
    for(auto &t : ts) t.join();
}
