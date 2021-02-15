/// MIT License
/// Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

#include <fstream>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <future>

#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<netdb.h> //hostent

#include "../libs/log2.h"

int main(int argc, char const *argv[])
{
    TLL_GLOGD("");
    TLL_GLOGI("");
    TLL_GLOGW("");
    TLL_GLOGF("");

    auto &logger = tll::log::Manager::instance();
    TLL_GLOGTF();
    {
        TLL_GLOGT(MAIN);
    }

    logger.stop();
    return 0;
}


