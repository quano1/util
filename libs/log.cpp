
#include "log.hpp"

#include <memory>
#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <sstream>
#include <thread>
#include <iomanip>
#include <ctime>
#include <chrono>

#include <stdarg.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <fcntl.h>

struct Counter;

static const uint32_t MAX_LOG_LENGTH = 0x1000;
// static const int DEFAULT_FD = 2;

std::string Log::preInit() const
{
    using std::chrono::system_clock;
    std::stringstream lBuf;
    std::time_t lNow = system_clock::to_time_t (system_clock::now());
    std::tm *lpTm = std::localtime(&lNow);
    lBuf << std::put_time(lpTm, "%D %H:%M:%S ");
    lBuf << getpid() << " ";
    lBuf << std::this_thread::get_id();

    return lBuf.str();
}

Log::Log()
{
    char *lMask = std::getenv("LLT_LOG_MASK");
    char *lFile = std::getenv("LLT_LOG_FILE");
    // char *lHost = std::getenv("LLT_LOG_HOST");
    // char *lPort = std::getenv("LLT_LOG_PORT");
    // fds.push_back(DEFAULT_FD);
    // _fd = 1;
    _async = false;
    _lvlMask = 0;

    if(lMask) this->_lvlMask = std::stoi(lMask);
    if(lFile && (this->_lvlMask & 0xF0)) this->_fds.push_back( open(lFile, O_RDWR | O_APPEND | O_CREAT) );
    // if(lHost) this->_hostName = std::string(lHost);
    // if(lPort) this->_port = std::stoi(lPort);

    // else this->_lvlMask = 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
std::string Log::formatStr( const std::string& aFormat, va_list &aVars ) const
{
    std::string lStr;
    int lBufSize = MAX_LOG_LENGTH;

    for (;;)
    {
        lStr.resize(lBufSize);

        if (lStr.empty())
        {
            return "";    // not enough memory
        }
        int ret = vsnprintf(&lStr[0], lBufSize - 3, aFormat.data(), aVars);

        if (ret < 0)
        {
            lBufSize *= 2;
        }
        else
        {
            break;
        }
    }
    return lStr;
}
#pragma GCC diagnostic pop

std::string Log::__log(int aLogType, bool aPre, const std::string& aFormat, va_list &aVars ) const
{
    // if( !(this->_lvlMask & (1 << aLogType)) ) return "";
    std::string lBuf = formatStr(aFormat, aVars);

    if(aPre)
    {
	    std::string lPreStr = preInit();

	    switch(aLogType)
	    {
	        case (int)LogType::Info: { lPreStr += " INFO "; break; }
	        // case (int)LogType::Debug: { lPreStr += " DEBUG "; break; }
	        case (int)LogType::Warn: { lPreStr += " WARN "; break; }
	        case (int)LogType::Fatal: { lPreStr += " FATAL "; break; }
	        default: { lPreStr += " UNKNOWN "; break; }
	    }
	    lBuf = lPreStr + lBuf;
	}

    return std::move(lBuf);

    // if(!lBuf.empty())
    // {
    //     if(_fd) printf("%s", lBuf.data());
        
    //     for(int const &lFd : _fds)
    //     {
    //         write(lFd, lBuf.data(), lBuf.size());
    //     }
    // }
}

void Log::log(int aLogType, bool aPre, const std::string& aFormat, ... ) const
{
    if( !(this->_lvlMask & (1 << aLogType)) ) return;

    va_list lVars;
    va_start(lVars, aFormat);
    std::string lBuf = __log(aLogType, aPre, aFormat, lVars);
    printf("%s\n", lBuf.data());

    for ( int i=0; i<_fds.size(); i++)
    {
        if( this->_lvlMask & (1 << (aLogType + (static_cast<int>(LogType::Max) * (i + 1))) ) )
        {
            write(_fds[i], lBuf.data(), lBuf.size());
        }
    }
    va_end(lVars);
}

#ifdef __cplusplus
extern "C" 
#endif
void llt_clog(int aLogType, bool aPre, const char * aFormat, ...)
{
    if( !(Log::ins()->_lvlMask & (1 << aLogType)) ) return;

    va_list lVars;
    va_start(lVars, aFormat);
    std::string lBuf = Log::ins()->__log(aLogType, aPre, aFormat, lVars);
    printf("%s", lBuf.data());
    va_end(lVars);
}
