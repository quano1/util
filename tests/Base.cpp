#include <LogMngr.hpp>
#include <global.hpp>
#include <Base.hpp>

extern llt::LogMngr *gpLog;

void Base1::do_smt()
{
    TRACE_THIS(gpLog, _);
    ::global1();
}

void Base2::do_smt()
{
    TRACE_THIS(gpLog, _);
    ::global2();
}
