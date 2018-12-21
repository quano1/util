#ifndef LMNGR_HPP_
#define LMNGR_HPP_

// #include "Signal.hpp"
#include <libs/SimpleSignal.hpp>
#include <libs/ThreadPool.hpp>
#include <libs/log.hpp>

#include <iostream>
#include <fstream>

#include <vector>
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


class LogSync;

static size_t const MAX_BUF_SIZE = 0x1000;

// enum class value_t : uint8_t

class Export
{
public:
    virtual void onHandle(int aLvl, std::string const &)=0;
    virtual int onInit(void *)=0;
    virtual void onDeinit(void *)=0;
};

class EConsole : public Export
{
public:
    virtual int onInit(void *aPtr);
    virtual void onDeinit(void *aPtr);
    virtual void onHandle(int aLvl, std::string const &aBuff);
};

class EFile : public Export
{
public:
    EFile(std::string const &aFile);
    EFile(std::string &&aFile);

    virtual int onInit(void *aPtr);
    virtual void onDeinit(void *aPtr);
    virtual void onHandle(int aLvl, std::string const &aBuff);

protected:
    std::ofstream ofs;
    std::string _f;
};

class ENetUDP : public Export
{
public:
    ENetUDP(std::string &&aHost, uint16_t aPort);
    ENetUDP(std::string const &aHost, uint16_t aPort);

    virtual int onInit(void *aPtr);
    virtual void onDeinit(void *aPtr);
    virtual void onHandle(int aLvl, std::string const &aBuff);

protected:
    std::string _host;
    uint16_t _port;
    int _fd;
    sockaddr_in _svrAddr;
};

class LogSync
{
public:

    LogSync() = default;
    virtual ~LogSync();

    virtual void init(void *aPtr=nullptr);
    virtual void deInit(void *aPtr=nullptr);
    virtual void add(Export *aExport);

    virtual void log(int aLvl, const char *fmt, ...);

    virtual std::string __format(char const *aFormat, va_list &aVars) const;

protected:
    Simple::Signal<void (int, std::string const &)> _onExport;
    Simple::Signal<int (void *)> _onInit;
    Simple::Signal<void (void *)> _onDeinit;

    std::vector<Export *> _exportContainer;
};

class LogAsync : public LogSync
{
public:

    LogAsync() = default;
    virtual ~LogAsync() = default;

    virtual void wait_for_complete();
    virtual void deInit(void *aPtr=nullptr);
    virtual void add(Export *aExport);
    virtual void log(int aLvl, const char *fmt, ...);

private:
    ThreadPool _pool;
};

#endif // LMNGR_HPP_