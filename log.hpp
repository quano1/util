#ifndef LOG_HPP_
#define LOG_HPP_

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

static const uint32_t MAX_LOG_LENGTH = 0x1000;
static const int DEFAULT_FD = 1;
// static llt_L s_L;

/* support
{"type":"debug","time_stamp":12345678,"body":{"file":"test.c","function":"ABC","line":100,"content":"this is a log"}}
{"type":"info","time_stamp":12345678,"body":{"context":"isp main","content":"this is a log"}}
{"type":"prof","time_stamp":12345678,"body":{"context":"isp main","duration":987654321}}

"UTC 10:10:10:10:10:2018" 0x123 0x456 DEBUG main.c ABC 100 'this is a log'
"UTC 10:10:10:10:10:2018" 0x123 0x789 INFO main.c ABC 100 'this is a log'
"UTC 10:10:10:10:10:2018" 0x123 0x789 PROF isp main 987654321
*/

enum class LogType : size_t
{
    Info=0,
    Debug,
    Warn,
    Fatal,
    Prof,
    Max,
};

struct Log /*: public llt_L*/
{
    Log();
    virtual ~Log() = default;
    // void init();
    // void setfd(int fd);
    void set_default_fd(int fd);
    void add_fd(int fd);

    virtual std::string prepare_pre(int log_type) const;

    // template<typename ... Args>
    // std::string string_format(const std::string &format, Args ... args) const;

    std::string string_format( const std::string& format, va_list args ) const;

    void __log(int log_type, const std::string& format, va_list args ) const;
    void log(int log_type, const std::string& format, ... ) const;

    static inline Log *ins()
    {
        static Log __ins;
        return &__ins;
    }


    // std::function<void *(void *)> fps;
    std::vector<int> fds;

    size_t lvl_msk; // bitmask variable
    int fd;
};

std::string Log::prepare_pre(int log_type) const
{
    using std::chrono::system_clock;
    std::stringstream buffer;
    // std::string buf = "UNKNOWN ";
    // std::stringstream buffer;
    // std::time_t now = std::time(NULL);
    std::time_t now = system_clock::to_time_t (system_clock::now());
    std::tm *ptm = std::localtime(&now);
    // time_t t;
    // struct tm *tmp;
    // time( &t );
    // tmp = localtime( &t );
    buffer << std::put_time(ptm, "%a %b %d %H:%M:%S %Y ");
    // std::string time = "";
    // std::string prog_id = "";
    buffer << getpid() << " ";
    // std::string thred_id = "";
    buffer << std::this_thread::get_id() << " ";

    // buf = time + prog_id + thred_id + buf;

    switch(log_type)
    {
        case (int)LogType::Info: { buffer << "INFO "; break; }
        case (int)LogType::Debug: { buffer << "DEBUG "; break; }
        case (int)LogType::Warn: { buffer << "WARN "; break; }
        case (int)LogType::Fatal: { buffer << "FATAL "; break; }
        default: buffer << "UNKNOWN "; break;
    }

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
// template<typename ... Args>
// std::string Log::string_format( const std::string& format, Args ... args ) const
// {
//     size_t size = snprintf( nullptr, 0, format.data(), args ... ) + 1; // Extra space for '\0'
//     std::unique_ptr<char[]> buf( new char[ size ] ); 
//     snprintf( buf.get(), size, format.data(), args ... );
//     return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
// }

std::string Log::string_format( const std::string& format, va_list args ) const
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

void Log::__log(int log_type, const std::string& format, va_list args ) const
{
    if( !(this->lvl_msk & (1 << log_type)) ) return;

    std::string pre = prepare_pre(log_type);
    std::string buf = string_format(pre + format, args);

    if(!buf.empty())
    {
        for(int const &fd : fds)
        {
            write(fd, buf.data(), buf.size());
        }
    }
}

void Log::log(int log_type, const std::string& format, ... ) const
{
    if( !(this->lvl_msk & (1 << log_type)) ) return;

    va_list args;
    va_start(args, format);
    __log(log_type, format, args);
    va_end(args);
}

#define LOGI(format, ...) Log::ins()->log(static_cast<int>(LogType::Info), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) Log::ins()->log(static_cast<int>(LogType::Debug), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(format, ...) Log::ins()->log(static_cast<int>(LogType::Warn), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGF(format, ...) Log::ins()->log(static_cast<int>(LogType::Fatal), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGP(format, ...) Log::ins()->log(static_cast<int>(LogType::Prof), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define CLOGI(format, ...) llt_clog(static_cast<int>(LogType::Info), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define CLOGD(format, ...) llt_clog(static_cast<int>(LogType::Debug), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define CLOGW(format, ...) llt_clog(static_cast<int>(LogType::Warn), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define CLOGF(format, ...) llt_clog(static_cast<int>(LogType::Fatal), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define CLOGP(format, ...) llt_clog(static_cast<int>(LogType::Prof), "%s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

extern "C" void llt_clog(int log_type, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    Log::ins()->__log(log_type, format, args);
    va_end(args);
}


#endif