#ifndef LOG_HPP_
#define LOG_HPP_

// #include <memory>
// #include <iostream>
// #include <string>
// #include <cstdio>
// #include <vector>
// #include <sstream>
// #include <thread>
// #include <iomanip>
// #include <ctime>
// #include <chrono>
// #include <stdarg.h>
// #include <unistd.h>

#include <vector>
#include <string>

#include <stdarg.h>

// #include "Counter.hpp"

// static llt_L s_L;

/* support
{"type":"debug","time_stamp":12345678,"body":{"file":"test.c","function":"ABC","line":100,"content":"this is a log"}}
{"type":"info","time_stamp":12345678,"body":{"context":"isp main","content":"this is a log"}}
{"type":"prof","time_stamp":12345678,"body":{"context":"isp main","duration":987654321}}

Mon Oct 29 22:08:18 2018 906 1 INFO test_log.cpp main 19 hello
Mon Oct 29 22:08:18 2018 906 1 DEBUG test_log.cpp main 20 hello
Mon Oct 29 22:08:18 2018 906 1 WARN test_log.cpp main 21 hello
Mon Oct 29 22:08:18 2018 906 1 FATAL test_log.cpp main 22 hello
Mon Oct 29 22:08:18 2018 906 1 PROF MAIN 69.648600 ms
*/

enum class LogType : size_t
{
    Debug=0,
    Info,
    // Warn,
    Fatal,
    Prof,
    Max,
};

struct Counter;

struct Log /*: public llt_L*/
{
    Log();
    virtual ~Log() = default;
    // void init(...);
    // void setfd(int fd);
    // void set_default_fd(int fd);
    inline void addFd(int fd)
    {
        _fds.push_back(fd);
    }

    inline void setAsyn(bool aAsync) { _async = aAsync; }

    virtual std::string preInit() const;

    // template<typename ... Args>
    // std::string string_format(const std::string &format, Args ... args) const;

    std::string formatStr( const std::string& aFormat, va_list &aVars ) const;

    std::string __log(int aLogType, bool aPre, const std::string& aFormat, va_list &aVars ) const;
    void log(int aLogType, bool aPre, const std::string& aFormat, ... ) const;

    static inline Log *ins()
    {
        static Log __ins;
        return &__ins;
    }

    inline size_t &lvlMask() { return _lvlMask; }

    // std::function<void *(void *)> fps;
    std::vector<int> _fds;

    size_t _lvlMask; // bitmask variable
    int _fd;
    int _async;
    int _file;
    int _sock;
};

#ifdef __cplusplus
extern "C" 
#endif
void llt_clog(int aLogType, bool aPre, const char * aFormat, ...);

inline bool canLog(int aLogType)
{
    return Log::ins()->_lvlMask & (1 << aLogType);
}

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define _LOGD(format, ...) printf("[Dbg] %s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define LOGD(format, ...) Log::ins()->log(static_cast<int>(LogType::Debug), true, "%s %s %d " format "", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOGI(format, ...) Log::ins()->log(static_cast<int>(LogType::Info), true, "%s %s %d " format "", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
// #define LOGW(format, ...) Log::ins()->log(static_cast<int>(LogType::Warn), true, "%s %s %d " format "", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#define LOGF(format, ...) Log::ins()->log(static_cast<int>(LogType::Fatal), true, "%s %s %d " format "", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

#define LOGP(context) Counter __PROF_ ## context( #context )
#define LOGPF() Log::ins()->log(static_cast<int>(LogType::Prof), false, "%s PROF %s", Log::ins()->preInit().data(), __func__); Counter __PROF__FUNC__(__func__)

#endif