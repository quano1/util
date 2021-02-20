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
    auto &logger = tll::log::Manager::instance();
    // tll::time::Counter<> counter;
    for (int i=0; i<std::stoi(argv[1]); i++)
    {
        TLL_GLOGTF();
        // LOGD("");
    }

    logger.stop();

    // LOGD("%.9f", counter.elapse().count());
    return 0;
}


