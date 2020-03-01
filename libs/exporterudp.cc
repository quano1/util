#include "exporterudp.h"
// #include <LogMngr.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <algorithm>

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

namespace llt {

// LUDPClt::LUDPClt(std::string &&aHost, uint16_t aPort) : host_(std::move(aHost)), _port(aPort) {}
// LUDPClt::LUDPClt(std::string const &aHost, uint16_t aPort) : host_(aHost), _port(aPort) {}

int LUDPClt::onInit()
{
    if(init_) return 1; 
    fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if ( fd_ <= 0 )
    {
        return 1;    
    }

    struct hostent *lpSvr;
    lpSvr = gethostbyname(host_.data());

    svr_addr_.sin_family = AF_INET;
    svr_addr_.sin_port = htons( port_ );
    bcopy((char *)lpSvr->h_addr, (char *)&svr_addr_.sin_addr.s_addr, lpSvr->h_length);

    // set non-blocking io
    if ( fcntl( fd_, F_SETFL, O_NONBLOCK, 1 ) == -1 )
    {
        // LOGD( "failed to make socket non-blocking" );
        return 1;
    }

    init_ = 1;
    return 0;
}

void LUDPClt::onDeinit()
{
    if(fd_ > 0) close(fd_);
    init_=0;
}

void LUDPClt::onExport(LogInfo const &log)
{
    std::string json = log.toJson();
    size_t sent_bytes = sendto( fd_, json.data(), json.size(), 0, (sockaddr*)&svr_addr_, sizeof(sockaddr_in) );
}


// LUDPSvr::LUDPSvr(uint16_t aPort) : _port(aPort) {}
// LUDPSvr::~LUDPSvr()
// {
//     onDeinit();
// }

int LUDPSvr::onInit()
{
    if(init_) return 1;

    fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if ( fd_ <= 0 )
    {
        return 1;
    }

    sockaddr_in svr_addr;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons( port_ );
    
    if ( ::bind( fd_, (const sockaddr*) &svr_addr, sizeof(svr_addr) ) < 0 )
    {
        // LOGD("");
        // m_error = SOCKET_ERROR_BIND_IPV4_FAILED;
        return 1;
    }

    thrd_ = std::thread([this]()
    {
        struct sockaddr_in client;
        int client_address_size = sizeof(client);

        char buf;

        while(is_running_)
        {
            struct timeval timeout = {0, 50000}; // make select() return once per second

            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(fd_, &readSet);

            if (select(fd_+1, &readSet, NULL, NULL, &timeout) >= 0)
            {
                if (FD_ISSET(fd_, &readSet) && is_running_)
                {
                    if(recvfrom(fd_, &buf, sizeof(buf), 0, (struct sockaddr *) &client,
                        (socklen_t*)&client_address_size) >= 0)
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        printf("Recv %c from %s:%d\n", buf, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

                        switch(buf)
                        {
                            case 'B': { sendto( fd_, "OK", 2, 0, (sockaddr*)&client, sizeof(sockaddr_in) ); break; } // broadcast
                            case 'C': { clt_addrs_.push_back(client); break; } // connect
                            case 'D': { clt_addrs_.erase(std::find_if(clt_addrs_.begin(), clt_addrs_.end(), // disconnect
                                [client](sockaddr_in aClt) {
                                    return ((aClt.sin_addr.s_addr == client.sin_addr.s_addr) && (aClt.sin_port == client.sin_port));
                                })); break; }
                            default: { /*LOGD("Unkown code: %d", buf);*/ break; }
                        }
                    }
                }
            }
            // std::this_thread::yield();
            // std::this_thread::sleep_for(std::chrono::microseconds(5));
        }
    });

    init_ = 1;
    return 0;
}

void LUDPSvr::onDeinit()
{
    if(!is_running_) return;
    is_running_ = false;
    thrd_.join();
    if(fd_ > 0) close(fd_);
    init_=0;
}

void LUDPSvr::onExport(LogInfo const &log)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string json = log.toJson();

    for(auto &clt_addr : clt_addrs_)
    {
        sendto( fd_, json.data(), json.size(), 0, (sockaddr*)&clt_addr, sizeof(sockaddr_in) );
    }
}

} /// llt