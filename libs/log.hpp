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

static const uint32_t MAX_LOG_LENGTH = 0x1000;
static const int DEFAULT_FD = 1;
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
    Warn,
    Fatal,
    Prof,
    Max,
};

struct Log /*: public llt_L*/
{
    Log();
    virtual ~Log() = default;
    // void init(...);
    // void setfd(int fd);
    void set_default_fd(int fd);
    void add_fd(int fd);

    virtual std::string prepare_pre() const;

    // template<typename ... Args>
    // std::string string_format(const std::string &format, Args ... args) const;

    std::string string_format( const std::string& format, va_list &args ) const;

    void __log(int log_type, bool pre, const std::string& format, va_list &args ) const;
    void log(int log_type, bool pre, const std::string& format, ... ) const;

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

#ifdef __cplusplus
extern "C" 
#endif
void llt_clog(int log_type, bool pre, const char * format, ...);


#define LOGI(format, ...) Log::ins()->log(static_cast<int>(LogType::Info), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) Log::ins()->log(static_cast<int>(LogType::Debug), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOGW(format, ...) Log::ins()->log(static_cast<int>(LogType::Warn), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOGF(format, ...) Log::ins()->log(static_cast<int>(LogType::Fatal), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define CLOGI(format, ...) llt_clog(static_cast<int>(LogType::Info), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define CLOGD(format, ...) llt_clog(static_cast<int>(LogType::Debug), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define CLOGW(format, ...) llt_clog(static_cast<int>(LogType::Warn), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define CLOGF(format, ...) llt_clog(static_cast<int>(LogType::Fatal), true, "%s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define LOGP(context) Counter __PROF ## context(#context)

#endif