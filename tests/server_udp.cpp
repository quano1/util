#include <LogMngr.hpp>

#include <thread>
#include <iostream>
#include <fstream>

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/select.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h> // close
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
* File server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string>

const std::string SOCKET_NAME = "/tmp/llt.dgram";

struct sockaddr_un name;
int down_flag = 0;
int ret;
int _fd;
int _clt;
int result;
// char buffer[BUFFER_SIZE];

int init()
{
   /*
    * In case the program exited inadvertently on the last run,
    * remove the socket.
    */

   unlink(SOCKET_NAME.data());

   /* Create local socket. */

   _fd = socket(AF_UNIX, SOCK_DGRAM, 0);
   if (_fd == -1) {
       perror("socket");
       exit(EXIT_FAILURE);
   }

   /*
    * For portability clear the whole structure, since some
    * implementations have additional (nonstandard) fields in
    * the structure.
    */

   memset(&name, 0, sizeof(struct sockaddr_un));

   /* Bind socket to socket name. */

   name.sun_family = AF_UNIX;
   strncpy(name.sun_path, SOCKET_NAME.data(), sizeof(name.sun_path) - 1);

   ret = bind(_fd, (const struct sockaddr *) &name,
              sizeof(struct sockaddr_un));
   if (ret == -1) {
       perror("bind");
       exit(EXIT_FAILURE);
   }

   if ( fcntl( _fd, F_SETFL, O_NONBLOCK, 1 ) == -1 )
    {
        LOGD( "failed to make socket non-blocking" );
        return 1;
    }

}

int main()
{
    struct sockaddr_in client;
    int client_address_size = sizeof(client);

    char buf[0x1000];

    init();

    while(1)
    {
      int size = recvfrom(_fd, buf, sizeof(buf), 0, (struct sockaddr *) &client,
            (socklen_t*)&client_address_size);
        if(size >= 0)
        {
          buf[size]=0;
            printf("%s\n",buf);
        }

        std::this_thread::yield();
    }

    return 0;
}