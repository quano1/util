#pragma once

#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <sstream>
#include <string>

#include <cstdarg>
#include <iomanip>

#include <netinet/in.h>
#include <unistd.h> // close
#include <sys/socket.h>
#include <sys/un.h>

#include "utils.h"

#ifndef STATIC_LIB
#define UTIL_INLINE
#else
#define UTIL_INLINE inline
#endif


namespace llt {


struct LogInfo;
template <typename T> class Exporter;

enum class LogType : uint8_t
{
    INFO=0,
    WARN,
    ERROR,
    TRACE,
};

struct Util
{
    static size_t const MAX_BUF_SIZE = 0x1000;
    static const std::string SEPARATOR;

    template <typename T>
    static inline std::string to_string(T const &aVal)
    {
        std::ostringstream ss;
        ss << aVal;
        return std::string(ss.str());
    }

    template <typename T>
    static inline std::string to_string_hex(T const &aVal)
    {
        std::ostringstream ss;
        ss << "0x" << std::hex << aVal;
        return std::string(ss.str());
    }

    static inline std::string to_string(LogType aLogType)
    {
        switch(aLogType)
        {
            case LogType::INFO:    return "INFO";
            case LogType::WARN:    return "WARN";
            case LogType::ERROR:     return "ERR!";
            case LogType::TRACE:   return "TRAC";
            default: return "----";
        }
    }

    template <class Duration>
    static inline std::string to_string(std::chrono::system_clock::time_point aTp)
    {
        std::stringstream lRet;
        lRet << std::chrono::time_point_cast<Duration>(aTp).time_since_epoch().count();
        return lRet.str();
    }

    static inline std::string to_string(std::string const &aFmt, std::chrono::system_clock::time_point aTp)
    {
        std::stringstream lRet;
        std::time_t lNow = std::chrono::system_clock::to_time_t(aTp);
        std::tm *lpTm = std::localtime(&lNow);
        lRet << std::put_time(lpTm, aFmt.data());
        return lRet.str();
    }

    static inline std::string format(char const *aFormat, va_list &aVars)
    {
        // std::string lBuff;
        char str[MAX_BUF_SIZE];
        if (vsnprintf (str, sizeof str, aFormat, aVars) >= 0) return str;
        return "";
        // return std::move(lBuff);
    }

}; // class Util


struct LogInfo
{
    LogType _type;
    int _level;
    std::chrono::system_clock::time_point _now;
    std::string _file;
    std::string _function;
    int _line;
    std::string _context;
    std::string _content;
    std::string _appName;
    void const *_this;

    inline std::string to_string(std::string aSepa="\t") const
    {
        std::string lRet;
        lRet = Util::to_string<std::chrono::microseconds>(_now) + aSepa \
                + _appName + aSepa \
                + _context + aSepa \
                + Util::to_string_hex(reinterpret_cast<uintptr_t>(_this)) + aSepa \
                + Util::to_string(_type) + aSepa  \
                + std::to_string(_level) + aSepa \
                + _file + aSepa \
                + _function + aSepa \
                + Util::to_string(_line) + aSepa \
                + /*std::string(_level * 2, aSepa) +*/ _content + "\n";
        return lRet;
    }

    inline std::string toJson() const
    {
        std::string lRet;
        lRet = "{";
        lRet += "\"now\":" + Util::to_string<std::chrono::microseconds>(_now) + ",";
        lRet += "\"type\":" + std::to_string((uint8_t)_type) + ",";
        lRet += "\"indent\":" + std::to_string(_level) + ",";
        lRet += "\"appName\":\"" + _appName + "\",";
        lRet += "\"context\":\"" + _context + "\",";
        lRet += "\"content\":\"" + _content + "\"";
        lRet += "},\n";
        return lRet;
    }

};


template <typename T>
class Exporter : public crtp<T, Exporter>
{
public:
    // ~Exporter()=default;
    void onExport(LogInfo const &log) { return this->underlying().onExport(log); }
    int onInit() { return this->underlying().onInit(); }
    void onDeinit() { return this->underlying().onDeinit(); }
protected:
    std::mutex mutex_;
    uint8_t init_=0;
};

class EConsole : public Exporter<EConsole>
{
public:
    UTIL_INLINE int onInit();
    UTIL_INLINE void onDeinit();
    UTIL_INLINE void onExport(LogInfo const&);
};

class EFile : public Exporter<EFile>
{
public:
    EFile(std::string file_name) : file_ (std::move(file_name)) {}
    ~EFile() {onDeinit();}

    UTIL_INLINE int onInit();
    UTIL_INLINE void onDeinit();
    UTIL_INLINE void onExport(LogInfo const&);

protected:
    std::ofstream ofs_;
    std::string file_;
};

class ENSClt : public Exporter<ENSClt>
{
public:
    ENSClt(std::string sock_name) : sock_(std::move(sock_name)) {} ;
    ~ENSClt() { onDeinit(); }

    UTIL_INLINE int onInit();
    UTIL_INLINE void onDeinit();
    UTIL_INLINE void onExport(LogInfo const&);

protected:
    int fd_;
    std::string sock_;
    sockaddr_un svr_addr_;
};


} // llt


#ifndef STATIC_LIB
#include "exporter.cc"
#endif