#include <Exporter.hpp>
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

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h> // close
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

using namespace llt;

std::string Util::SEPARATOR = ";";

int EConsole::on_init() { if(_init) return 1; _init = 1; return 0; }
void EConsole::on_deinit() { }
void EConsole::on_export(LogInfo const &aLogInfo)
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::cout << aLogInfo.to_string(Util::SEPARATOR);
}

EFile::EFile(std::string const &aFile) : _f (aFile) {}
EFile::EFile(std::string &&aFile) : _f (std::move(aFile)) {}

int EFile::on_init()
{
    if(_init) return 1; 
    _init = 1;
    _ofs.open(_f, std::ios::out /*| std::ios::app*/ );
    if(!_ofs.is_open()) return 1;
    return 0;
}

void EFile::on_deinit()
{
    if(_ofs.is_open())
    {
        _ofs.flush();
        _ofs.close();
    }
}

void EFile::on_export(LogInfo const &aLogInfo)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _ofs << aLogInfo.to_string(Util::SEPARATOR);
    // _ofs << aLogInfo.to_json();
}

ENSClt::ENSClt(std::string const &aSockName) : _sockName (aSockName) {}
ENSClt::ENSClt(std::string &&aSockName) : _sockName (std::move(aSockName)) {}

int ENSClt::on_init() 
{ 
    if(_init) return 1; 
    _init = 1;

    int ret = -1;

    _fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    memset(&_svrAddr, 0, sizeof(struct sockaddr_un));
    _svrAddr.sun_family = AF_UNIX;
    strncpy(_svrAddr.sun_path, _sockName.data(), sizeof(_svrAddr.sun_path) - 1);

    // set non-blocking io
    if ( fcntl( _fd, F_SETFL, O_NONBLOCK, 1 ) == -1 )
    {
        LOGD( "failed to make socket non-blocking" );
        return 1;
    }

    return ret;
}

ENSClt::~ENSClt()
{
    if(_fd > 0) close(_fd);
}

void ENSClt::on_deinit() { if(_fd > 0) close(_fd); }
void ENSClt::on_export(LogInfo const &aLogInfo)
{
    std::string lTmp = aLogInfo.to_json();
    size_t sent_bytes = sendto( _fd, lTmp.data(), lTmp.size(), 0, (sockaddr *)&_svrAddr, sizeof(sockaddr_un) );
}