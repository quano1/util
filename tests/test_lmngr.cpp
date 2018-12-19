#include <libs/LMngr.hpp>
#include <libs/log.hpp>

#include <iostream>
#include <fstream>

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

class EConsole : public Export
{
public:
    int onInit(void *aPtr) { return 0; }
    void onDeinit(void *aPtr) { }
    int onHandle(std::string const &aBuff) { std::cout<<aBuff; return 0; }
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

    int onHandle(std::string const &aBuff)
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

    int onHandle(std::string const &aBuff)
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

int main()
{
    LMngr log;
    log.add(new EConsole());
    log.add(new EFile("log.txt"));
    log.add(new ENetUDP("localhost", 65530));

    log.init(nullptr);

    log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    log.deInit();
    LOGD("");
    return 0;
}