#ifndef LMNGR_HPP_
#define LMNGR_HPP_

// #include "Signal.hpp"
#include <libs/SimpleSignal.hpp>
#include <libs/ThreadPool.hpp>
#include <libs/log.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <tuple>

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h> // close
#include <netdb.h>


class LogMngr;

struct Tracer
{
public:
    Tracer() : _indent(1) {}
    Tracer(Tracer *aObj) : _obj(aObj) 
    {
        _indent = _obj->_indent + 1;
    }

    ~Tracer()
    {
        // _indent--;
        // if(_obj)
    }

    Tracer *_obj;
    int _indent;
};

struct __LogInfo
{
    int _type;
    std::chrono::high_resolution_clock::time_point _now;
    std::string _ctx;
};

enum class Level : uint8_t
{
    INFO=0,
    WARN,
    FATAL,
    TRACE,
};

class Export
{
public:
    virtual void on_handle(__LogInfo aLogInfo, std::string const &)=0;
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
    virtual void on_handle(__LogInfo aLogInfo, std::string const &aBuff);
};

class EFile : public Export
{
public:
    EFile(std::string const &aFile);
    EFile(std::string &&aFile);

    virtual int on_init();
    virtual void on_deinit();
    virtual void on_handle(__LogInfo aLogInfo, std::string const &aBuff);

protected:
    std::ofstream ofs;
    std::string _f;
};

class ENetUDP : public Export
{
public:
    ENetUDP(std::string &&aHost, uint16_t aPort);
    ENetUDP(std::string const &aHost, uint16_t aPort);

    virtual int on_init();
    virtual void on_deinit();
    virtual void on_handle(__LogInfo aLogInfo, std::string const &aBuff);

protected:
    std::string _host;
    uint16_t _port;
    int _fd;
    sockaddr_in _svrAddr;
};

class LogMngr
{

private:
    typedef std::tuple<pid_t, std::thread::id> __key_t;

    struct __key_hash : public std::unary_function<__key_t, std::size_t>
    {
        std::size_t operator()(const __key_t& k) const
        {
            return std::get<0>(k) ^ std::hash<std::thread::id>()(std::get<1>(k));
        }
    };
public:

    LogMngr() = default;
    virtual ~LogMngr();

    virtual void reg_ctx(std::string aProgName, std::string aThreadName);

    virtual void init(size_t=0);
    virtual void deinit();
    virtual void add(Export *aExport);

    virtual void async_wait();

    virtual void log(int aLvl, const char *fmt, ...);
    virtual void log_async(int aLvl, const char *fmt, ...);
    virtual std::string __format(char const *aFormat, va_list &aVars) const;

    inline __key_t get_key()
    {
        return {::getpid(), std::this_thread::get_id()};
    }

protected:
    ThreadPool _pool;

    Simple::Signal<void (__LogInfo, std::string const &)> _onExport;
    Simple::Signal<int ()> _onInit;
    Simple::Signal<void ()> _onDeinit;

    std::vector<Export *> _exportContainer;

    std::unordered_map<__key_t, std::string, __key_hash> _ctx;
    std::unordered_map<__key_t, int, __key_hash> _indents;
};

#endif // LMNGR_HPP_