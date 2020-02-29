#include "exporter.h"
// #include <LogMngr.hpp>

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

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h> // close
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

namespace llt {

std::string const Util::SEPARATOR = ";";

int EConsole::onInit() { if(init_) return 1; init_ = 1; return 0; }
void EConsole::onDeinit() { }
void EConsole::onExport(LogInfo const &log)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << log.to_string(Util::SEPARATOR);
}

// EFile::EFile(std::string const &aFile) : file_ (aFile) {}
// EFile::EFile(std::string &&aFile) : file_ (std::move(aFile)) {}

int EFile::onInit()
{
    if(init_) return 1; 
    init_ = 1;
    ofs_.open(file_, std::ios::out /*| std::ios::app*/ );
    if(!ofs_.is_open()) return 1;
    return 0;
}

void EFile::onDeinit()
{
    if(ofs_.is_open())
    {
        ofs_.flush();
        ofs_.close();
    }
    init_=0;
}

void EFile::onExport(LogInfo const &log)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ofs_ << log.to_string(Util::SEPARATOR);
    // ofs_ << log.toJson();
}

// // ENSClt::ENSClt(std::string const &aSockName) : sock_ (aSockName) {}
// // ENSClt::ENSClt(std::string &&aSockName) : sock_ (std::move(aSockName)) {}

int ENSClt::onInit() 
{ 
    if(init_) return 1; 
    init_ = 1;

    int ret = -1;

    fd_ = socket(AF_UNIX, SOCK_DGRAM, 0);

    memset(&svr_addr_, 0, sizeof(struct sockaddr_un));
    svr_addr_.sun_family = AF_UNIX;
    strncpy(svr_addr_.sun_path, sock_.data(), sizeof(svr_addr_.sun_path) - 1);

    /// set non-blocking io
    if ( fcntl( fd_, F_SETFL, O_NONBLOCK, 1 ) == -1 )
    {
        // LOGD( "failed to make socket non-blocking" );
        return 1;
    }

    return ret;
}

void ENSClt::onDeinit() { if(fd_ > 0) close(fd_); init_=0; }
void ENSClt::onExport(LogInfo const &log)
{
    std::string json = log.toJson();
    /*size_t sent_bytes = */sendto( fd_, json.data(), json.size(), 0, (sockaddr *)&svr_addr_, sizeof(sockaddr_un) );
}

} /// llt