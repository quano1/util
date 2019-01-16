#include <ExporterUDP.hpp>
#include <LogMngr.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <chrono>

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iomanip>

#include <sys/select.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h> // close
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace llt;

EUDPClt::EUDPClt(std::string &&aHost, uint16_t aPort) : _host(std::move(aHost)), _port(aPort) {}
EUDPClt::EUDPClt(std::string const &aHost, uint16_t aPort) : _host(aHost), _port(aPort) {}

int EUDPClt::on_init()
{
    if(_init) return 1; 
    _fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if ( _fd <= 0 )
    {
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

    _init = 1;
    return 0;
}

void EUDPClt::on_deinit()
{
    if(_fd > 0) close(_fd);
}

void EUDPClt::on_export(LogInfo const &aLogInfo)
{
    std::string lTmp = aLogInfo.to_json();
    size_t sent_bytes = sendto( _fd, lTmp.data(), lTmp.size(), 0, (sockaddr*)&_svrAddr, sizeof(sockaddr_in) );
}


EUDPSvr::EUDPSvr(uint16_t aPort) : _port(aPort) {}
EUDPSvr::~EUDPSvr()
{
    on_deinit();
}

int EUDPSvr::on_init()
{
    if(_init) return 1;

    _fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if ( _fd <= 0 )
    {
        return 1;
    }

    sockaddr_in _svrAddr;
    _svrAddr.sin_family = AF_INET;
    _svrAddr.sin_addr.s_addr = INADDR_ANY;
    _svrAddr.sin_port = htons( _port );
    
    if ( ::bind( _fd, (const sockaddr*) &_svrAddr, sizeof(_svrAddr) ) < 0 )
    {
        LOGD("");
        // m_error = SOCKET_ERROR_BIND_IPV4_FAILED;
        return 1;
    }

    _thrd = std::thread([this]()
    {
        struct sockaddr_in client;
        int client_address_size = sizeof(client);

        char buf;

        while(!_stop)
        {
            struct timeval timeout = {1, 0}; // make select() return once per second

            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(_fd, &readSet);

            if (select(_fd+1, &readSet, NULL, NULL, &timeout) >= 0)
            {
                if (FD_ISSET(_fd, &readSet) && !_stop)
                {
                    if(recvfrom(_fd, &buf, sizeof(buf), 0, (struct sockaddr *) &client,
                        (socklen_t*)&client_address_size) >= 0)
                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        printf("Recv %c from %s:%d\n", buf, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

                        switch(buf)
                        {
                            case 'B': { sendto( _fd, "OK", 2, 0, (sockaddr*)&client, sizeof(sockaddr_in) ); break; } // broadcast
                            case 'C': { _cltAddrs.push_back(client); break; } // connect
                            case 'D': { _cltAddrs.erase(std::remove_if(_cltAddrs.begin(), _cltAddrs.end(), // disconnect
                                [client](sockaddr_in aClt) {
                                    return ((aClt.sin_addr.s_addr == client.sin_addr.s_addr) && (aClt.sin_port == client.sin_port));
                                })); break; }
                            default: { LOGD("Unkown code: %d", buf); break; }
                        }
                    }
                }
            }
            // std::this_thread::yield();
            // std::this_thread::sleep_for(std::chrono::microseconds(5));
        }
    });

    _init = 1;
    return 0;
}

void EUDPSvr::on_deinit()
{
    if(_stop) return;
    _stop = true;
    _thrd.join();
    if(_fd > 0) close(_fd);
}

void EUDPSvr::on_export(LogInfo const &aLogInfo)
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string lTmp = aLogInfo.to_json();

    for(auto &cltAddr : _cltAddrs)
    {
        sendto( _fd, lTmp.data(), lTmp.size(), 0, (sockaddr*)&cltAddr, sizeof(sockaddr_in) );
    }
}

