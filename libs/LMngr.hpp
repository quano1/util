#ifndef LMNGR_HPP_
#define LMNGR_HPP_

// #include "Signal.hpp"
#include <libs/SimpleSignal.hpp>
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


class LMngr;

static size_t const MAX_BUF_SIZE = 0x1000;

// enum class value_t : uint8_t

class Export
{
public:

    Export()
    {
    }

    Export(Export const &aOther)
    {
    }

    Export(Export &&aOther)
    {
    }

    virtual int onHandle(int aLvl, std::string const &)=0;
    virtual int onInit(void *)=0;
    virtual void onDeinit(void *)=0;
};

class EConsole : public Export
{
public:
    int onInit(void *aPtr) { return 0; }
    void onDeinit(void *aPtr) { }
    int onHandle(int aLvl, std::string const &aBuff) { std::cout<<aBuff; return 0; }
};

class EFile : public Export
{
public:
    EFile(std::string const &aFile) : _f (aFile) {}
    EFile(std::string &&aFile) : _f (std::move(aFile)) {}

    int onInit(void *aPtr)
    {
        ofs.open(_f, std::ios::out | std::ios::app );
        if(!ofs.is_open()) return 1;
        return 0;
    }

    void onDeinit(void *aPtr)
    {
        if(ofs.is_open())
        {
            ofs.flush();
            ofs.close();
        }
    }

    int onHandle(int aLvl, std::string const &aBuff)
    { 
        ofs << aBuff;
    }

private:
    std::ofstream ofs;
    std::string _f;
};

class ENetUDP : public Export
{
public:
    ENetUDP(std::string &&aHost, uint16_t aPort) : _host(std::move(aHost)), _port(aPort) {}

    ENetUDP(std::string const &aHost, uint16_t aPort) : _host(aHost), _port(aPort) {}

    int onInit(void *aPtr)
    {
        _fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        
        if ( _fd <= 0 )
        {
            LOGD("");
            return 1;
        }

        struct hostent *lpSvr;
        lpSvr = gethostbyname(_host.data());

        _svrAddr.sin_family = AF_INET;
        _svrAddr.sin_port = htons( _port );
        bcopy((char *)lpSvr->h_addr, (char *)&_svrAddr.sin_addr.s_addr, lpSvr->h_length);

        // set non-blocking io
        if ( fcntl( _fd, F_SETFL, O_NONBLOCK, 1 ) == -1 )
        {
            LOGD( "failed to make socket non-blocking" );
            return 1;
        }

        return 0;
    }

    void onDeinit(void *aPtr)
    {
        if(_fd > 0) close(_fd);
    }

    int onHandle(int aLvl, std::string const &aBuff)
    {
        size_t sent_bytes = sendto( _fd, aBuff.data(), aBuff.size(), 0, (sockaddr*)&_svrAddr, sizeof(sockaddr_in) );

        if(!sent_bytes) 
        {
            LOGD("");
            return 1;
        }

        return 0;
    }

private:
    std::string _host;
    uint16_t _port;
    int _fd;
    sockaddr_in _svrAddr;
};

class LMngr
{
public:

    LMngr() {}

    virtual ~LMngr() 
    {
        for(auto lp : _exportContainer)
        {
            delete lp;
        }
    }

    virtual void init(void *aPtr=nullptr)
    {
        _onInit.emit(aPtr);
    }

    virtual void deInit(void *aPtr=nullptr)
    {
        _onDeinit.emit(aPtr);
    }

    void add(Export *aExport)
    {
        _exportContainer.push_back(aExport);
        Export *lIns = _exportContainer.back();
        size_t lId = _onExport.connect(Simple::slot(lIns, &Export::onHandle));
        lId = _onInit.connect(Simple::slot(lIns, &Export::onInit));
        lId = _onDeinit.connect(Simple::slot(lIns, &Export::onDeinit));
    }

    void log(int aLvl, const char *fmt, ...);

    std::string __format(char const *aFormat, va_list &aVars) const
    {
        std::string lBuff;
        char str[MAX_BUF_SIZE];
        if (vsnprintf (str, sizeof str, aFormat, aVars) >= 0) lBuff = str;
        return std::move(lBuff);
    }

private:
    Simple::Signal<int (int, std::string const &)> _onExport;
    Simple::Signal<int (void *)> _onInit;
    Simple::Signal<void (void *)> _onDeinit;

    std::vector<Export *> _exportContainer;
};

void LMngr::log(int aLvl, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = __format(fmt, args);
    va_end (args);

    _onExport.emit(aLvl, lBuff);
}

#endif // LMNGR_HPP_