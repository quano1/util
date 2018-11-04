
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

std::string Log::prepare_pre() const
{
    using std::chrono::system_clock;
    std::stringstream buffer;
    std::time_t now = system_clock::to_time_t (system_clock::now());
    std::tm *ptm = std::localtime(&now);
    buffer << std::put_time(ptm, "%D %H:%M:%S ");
    buffer << getpid() << " ";
    buffer << std::this_thread::get_id();

    return buffer.str();
}

Log::Log()
{
    char *lvl_msk = std::getenv("LLT_LOG_MASK");
    fds.push_back(DEFAULT_FD);

    if(lvl_msk) this->lvl_msk = std::stoi(lvl_msk);
    else this->lvl_msk = 0;
}

void Log::add_fd(int fd)
{
    fds.push_back(fd);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
std::string Log::string_format( const std::string& format, va_list &args ) const
{
    std::string buf;
    int bufferSize = MAX_LOG_LENGTH;

    for (;;)
    {
        buf.resize(bufferSize);

        if (buf.empty())
        {
            return "";    // not enough memory
        }
        int ret = vsnprintf(&buf[0], bufferSize - 3, format.data(), args);

        if (ret < 0)
        {
            bufferSize *= 2;
        }
        else
        {
            break;
        }
    }
    return buf;
}
#pragma GCC diagnostic pop

void Log::__log(int log_type, bool pre, const std::string& format, va_list &args ) const
{
    if( !(this->lvl_msk & (1 << log_type)) ) return;
    std::string buf = string_format(format, args);

    if(pre)
    {
	    std::string pre_str = prepare_pre();

	    switch(log_type)
	    {
	        case (int)LogType::Info: { pre_str += " INFO "; break; }
	        case (int)LogType::Debug: { pre_str += " DEBUG "; break; }
	        case (int)LogType::Warn: { pre_str += " WARN "; break; }
	        case (int)LogType::Fatal: { pre_str += " FATAL "; break; }
	        default: { pre_str += " UNKNOWN "; break; }
	    }
	    buf = pre_str + buf;
	}

    if(!buf.empty())
    {
        for(int const &fd : fds)
        {
            write(fd, buf.data(), buf.size());
        }
    }
}

void Log::log(int log_type, bool pre, const std::string& format, ... ) const
{
    if( !(this->lvl_msk & (1 << log_type)) ) return;

    va_list args;
    va_start(args, format);
    __log(log_type, pre, format, args);
    va_end(args);
}

#ifdef __cplusplus
extern "C" 
#endif
void llt_clog(int log_type, bool pre, const char * format, ...)
{
    if( !(Log::ins()->lvl_msk & (1 << log_type)) ) return;

    va_list args;
    va_start(args, format);
    Log::ins()->__log(log_type, pre, format, args);
    va_end(args);
}
