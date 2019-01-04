#include <LogMngr.hpp>

#include <thread>
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

#include <netinet/in.h>
#include <arpa/inet.h>

int _fd;
std::string _host = "localhost";
uint16_t _port = 65530;
sockaddr_in _svrAddr;
int namelen;

int init()
{
    _fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if ( _fd <= 0 )
    {
        LOGD("");
        return 1;
    }

    _svrAddr.sin_family = AF_INET;
    _svrAddr.sin_addr.s_addr = INADDR_ANY;
    _svrAddr.sin_port = htons( _port );

    if ( ::bind( _fd, (const sockaddr*) &_svrAddr, sizeof(_svrAddr) ) < 0 )
    {
        LOGD("");
        // m_error = SOCKET_ERROR_BIND_IPV4_FAILED;
        return 1;
    }

    /* Find out what port was really assigned and print it */
    namelen = sizeof(_svrAddr);
    if (getsockname(_fd, (struct sockaddr *) &_svrAddr, (socklen_t *)&namelen) < 0)
    {
        LOGD("");
        return 1;
    }

    printf("host: %s - Port %d\n", inet_ntoa(_svrAddr.sin_addr), ntohs(_svrAddr.sin_port));

    // set non-blocking io
    if ( fcntl( _fd, F_SETFL, O_NONBLOCK, 1 ) == -1 )
    {
        LOGD( "failed to make socket non-blocking" );
        return 1;
    }
}

int main()
{
    init();

    struct sockaddr_in client;
    int client_address_size = sizeof(client);

    char buf[0x1000];

    while(1)
    {
        if(recvfrom(_fd, buf, sizeof(buf), 0, (struct sockaddr *) &client,
            (socklen_t*)&client_address_size) >= 0)
        {
            printf("Received message %s from domain %s port %d internet\
                address %s\n",
                buf,
                (client.sin_family == AF_INET?"AF_INET":"UNKNOWN"),
            ntohs(client.sin_port),
            inet_ntoa(client.sin_addr));
        }

        std::this_thread::yield();
    }

    return 0;
}
