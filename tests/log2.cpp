#include <LogMngr.hpp>
#include <global.hpp>

extern llt::LogMngr *gpLog;

class Base1
{
public:
    Base1() = default;
    virtual ~Base1() = default;

    void do_smt()
    {
        TRACE_THIS(gpLog, _);
        ::global1();
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
        ::global2();
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
            Base1 b1;
            Base2 b2;
            for(int i=0; i<aLoop; i++)
            {
                LOGI(gpLog, "");
                b1.do_smt();
                b2.do_smt();
                std::this_thread::sleep_for(std::chrono::microseconds(aDelay));
            }
        });
    }
        
    for(auto &t : ts) t.join();
}
