#ifndef LMNGR_HPP_
#define LMNGR_HPP_

// #include "Signal.hpp"
#include <libs/SimpleSignal.hpp>
#include <libs/ThreadPool.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <tuple>
#include <sstream>

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iomanip>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h> // close
#include <netdb.h>

typedef std::tuple<pid_t, std::thread::id> ctx_key_t;

class LogMngr;
struct __LogInfo;

enum class _LogType : uint8_t
{
    INFO=0,
    WARN,
    FATAL,
    TRACE,
};

class Export
{
public:
    virtual void on_export(__LogInfo const &aLogInfo)=0;
    virtual int on_init()=0;
    virtual void on_deinit()=0;
protected:
    std::mutex _mutex;
};

class EConsole : public Export
{
public:
    virtual int on_init();
    virtual void on_deinit();
    virtual void on_export(__LogInfo const &aLogInfo);
};

class EFile : public Export
{
public:
    EFile(std::string const &aFile);
    EFile(std::string &&aFile);

    virtual int on_init();
    virtual void on_deinit();
    virtual void on_export(__LogInfo const &aLogInfo);

protected:
    std::ofstream _ofs;
    std::string _f;
};

class EUDPClt : public Export
{
public:
    EUDPClt(std::string &&aHost, uint16_t aPort);
    EUDPClt(std::string const &aHost, uint16_t aPort);

    virtual int on_init();
    virtual void on_deinit();
    virtual void on_export(__LogInfo const &aLogInfo);

protected:
    std::string _host;
    uint16_t _port;
    int _fd;
    sockaddr_in _svrAddr;
};

class EUDPSvr : public Export
{
public:
    EUDPSvr(uint16_t aPort);
    virtual ~EUDPSvr();

    virtual int on_init();
    virtual void on_deinit();
    virtual void on_export(__LogInfo const &aLogInfo);

protected:
    uint16_t _port;
    int _fd;
    bool _stop;
    std::vector<sockaddr_in> _cltAddrs;
    std::thread _thrd;
};

struct Util
{
    static size_t const MAX_BUF_SIZE = 0x1000;

    static inline ctx_key_t make_key()
    {
        return {::getpid(), std::this_thread::get_id()};
    }

    static inline std::string to_string(_LogType aLogType)
    {
        switch(aLogType)
        {
            case _LogType::INFO:    return "INFOR";
            case _LogType::WARN:    return "WARNG";
            case _LogType::FATAL:   return "FATAL";
            case _LogType::TRACE:   return "TRACE";
            default: return "_____";
        }
    }

    template <class Duration>
    static inline std::string to_string(std::chrono::high_resolution_clock::time_point aTp)
    {
        std::stringstream lRet;
        lRet << std::chrono::time_point_cast<Duration>(aTp).time_since_epoch().count();
        return lRet.str();
    }

    static inline std::string to_string(std::string const &aFmt, std::chrono::high_resolution_clock::time_point aTp)
    {
        std::stringstream lRet;
        std::time_t lNow = std::chrono::system_clock::to_time_t(aTp);
        std::tm *lpTm = std::localtime(&lNow);
        lRet << std::put_time(lpTm, aFmt.data());
        return lRet.str();
    }

    static inline std::string format(char const *aFormat, va_list &aVars)
    {
        std::string lBuff;
        char str[MAX_BUF_SIZE];
        if (vsnprintf (str, sizeof str, aFormat, aVars) >= 0) lBuff = str;
        return std::move(lBuff);
    }
};

struct __LogInfo
{
    _LogType _type;
    int _indent;
    std::chrono::high_resolution_clock::time_point _now;
    std::string _ctx;
    std::string _content;

    inline std::string to_string() const
    {
        std::string lRet;
        lRet = Util::to_string<std::chrono::microseconds>(_now) + " " + _ctx + " " + std::to_string(_indent) + std::string(_indent * 2, ' ') + " " + Util::to_string(_type) + " " + _content + "\n";
        return lRet;
    }

    inline std::string to_json() const
    {
        std::string lRet;
        lRet = "{";
        lRet += "\"type\":" + std::to_string((uint8_t)_type) + ",";
        lRet += "\"indent\":" + std::to_string(_indent) + ",";
        lRet += "\"now\":" + Util::to_string<std::chrono::microseconds>(_now) + ",";
        lRet += "\"content\":\"" + _ctx + "\",";
        lRet += "\"content\":\"" + _content + "\"";
        lRet += "}";
        return lRet;
    }
};

class LogMngr
{

private:
    struct __key_hash : public std::unary_function<ctx_key_t, std::size_t>
    {
        std::size_t operator()(const ctx_key_t& k) const
        {
            return std::get<0>(k) ^ std::hash<std::thread::id>()(std::get<1>(k));
        }
    };
public:

    LogMngr() = default;
    LogMngr(std::vector<Export *> const &);
    virtual ~LogMngr();

    virtual void reg_ctx(std::string aProgName, std::string aThreadName);

    virtual void init(size_t=0);
    virtual void deinit();
    virtual void add(Export *aExport);

    virtual void async_wait();

    virtual void log(_LogType aLogType, const char *fmt, ...);
    virtual void log_async(_LogType aLogType, const char *fmt, ...);

    inline void inc_indent()
    {
        _indents[Util::make_key()]++;
    }

    inline void dec_indent()
    {
        _indents[Util::make_key()]--;
    }

protected:
    ThreadPool _pool;

    Simple::Signal<void (__LogInfo const &)> _sigExport;
    Simple::Signal<int ()> _sigInit;
    Simple::Signal<void ()> _sigDeinit;

    std::vector<Export *> _exportContainer;

    std::unordered_map<ctx_key_t, std::string, __key_hash> _ctx;
    std::unordered_map<ctx_key_t, int, __key_hash> _indents;
};

struct Tracer
{
public:
    Tracer(std::string const &aName, LogMngr &aLogger) : _l(aLogger), _name(aName)
    {
        _l.inc_indent();
    }

    ~Tracer()
    {
        _l.dec_indent();
        _l.log_async(_LogType::TRACE, "~%s", _name.data());
    }

    std::string _name;
    LogMngr &_l;
};

#define LOGD(format, ...) printf("[Dbg] %s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define TRACE(FUNC, logger) logger.log_async(_LogType::TRACE, #FUNC " %s %d", __FUNCTION__, __LINE__); Tracer __##FUNC(#FUNC, logger)

#define LOGI(logger, fmt, ...) logger.log_async(_LogType::INFO, "%s %s %d" fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(logger, fmt, ...) logger.log_async(_LogType::WARN, "%s %s %d" fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGF(logger, fmt, ...) logger.log_async(_LogType::FATAL, "%s %s %d" fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#endif // LMNGR_HPP_