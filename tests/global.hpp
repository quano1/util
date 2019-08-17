#pragma once

#include <LogMngr.hpp>

extern llt::LogMngr *gpLog;

void global1()
{
	TRACE(gpLog, _);
}

void global2()
{
	TRACE(gpLog, _);
}
