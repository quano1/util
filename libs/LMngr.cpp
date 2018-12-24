#include <libs/LMngr.hpp>

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

static size_t const MAX_BUF_SIZE = 0x1000;

int EConsole::on_init() { return 0; }
void EConsole::on_deinit() { }
void EConsole::on_handle(__LogInfo aLogInfo, std::string const &aBuff)
{ 
    std::lock_guard<std::mutex> lock(_mutex);
    std::cout << aBuff; 
}

EFile::EFile(std::string const &aFile) : _f (aFile) {}
EFile::EFile(std::string &&aFile) : _f (std::move(aFile)) {}

int EFile::on_init()
{
    ofs.open(_f, std::ios::out | std::ios::app );
    if(!ofs.is_open()) return 1;
    return 0;
}

void EFile::on_deinit()
{
    if(ofs.is_open())
    {
        ofs.flush();
        ofs.close();
    }
}

void EFile::on_handle(__LogInfo aLogInfo, std::string const &aBuff)
{
    std::lock_guard<std::mutex> lock(_mutex);
    ofs << aBuff;
}



ENetUDP::ENetUDP(std::string &&aHost, uint16_t aPort) : _host(std::move(aHost)), _port(aPort) {}
ENetUDP::ENetUDP(std::string const &aHost, uint16_t aPort) : _host(aHost), _port(aPort) {}

int ENetUDP::on_init()
{
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

    return 0;
}

void ENetUDP::on_deinit()
{
    if(_fd > 0) close(_fd);
}

void ENetUDP::on_handle(__LogInfo aLogInfo, std::string const &aBuff)
{
    std::stringstream lBuf;
    // std::time_t lNow = std::chrono::system_clock::to_time_t(aLogInfo._now);
    // std::tm *lpTm = std::localtime(&lNow);
    // lBuf << std::put_time(lpTm, "%D %H:%M:%S ");
    auto now_ms = std::chrono::time_point_cast<std::chrono::seconds>(aLogInfo._now).time_since_epoch().count();
    lBuf << now_ms << " ";
    lBuf << aLogInfo._pid << " " << aLogInfo._tid << " ";
    std::string lTmp = lBuf.str() + aBuff;
    size_t sent_bytes = sendto( _fd, lTmp.data(), lTmp.size(), 0, (sockaddr*)&_svrAddr, sizeof(sockaddr_in) );
}

LogMngr::~LogMngr() 
{
    for(auto lp : _exportContainer)
    {
        delete lp;
    }
}

void LogMngr::init(size_t aThreads)
{
    if(aThreads)
    {
        _pool.add_workers(aThreads);
    }
    _onInit.emit();
}

void LogMngr::deinit()
{
    _pool.force_stop();
    _onDeinit.emit();
}

void LogMngr::add(Export *aExport)
{
    _exportContainer.push_back(aExport);
    Export *lIns = _exportContainer.back();
    size_t lId = _onExport.connect(Simple::slot(lIns, &Export::on_handle));
    lId = _onInit.connect(Simple::slot(lIns, &Export::on_init));
    lId = _onDeinit.connect(Simple::slot(lIns, &Export::on_deinit));
}

std::string LogMngr::__format(char const *aFormat, va_list &aVars) const
{
    std::string lBuff;
    char str[MAX_BUF_SIZE];
    if (vsnprintf (str, sizeof str, aFormat, aVars) >= 0) lBuff = str;
    return std::move(lBuff);
}

void LogMngr::log(int aLvl, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = __format(fmt, args);
    va_end (args);
    __LogInfo lInfo = {aLvl, std::chrono::system_clock::now(), _processes[::getpid()], _threads[std::this_thread::get_id()]};
    _onExport.emit(lInfo, lBuff);
}

void LogMngr::async_wait()
{
    _pool.wait_for_complete();
}

void LogMngr::log_async(int aLvl, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = __format(fmt, args);
    va_end (args);
    __LogInfo lInfo = {aLvl, std::chrono::system_clock::now(), _processes[::getpid()], _threads[std::this_thread::get_id()]};
    _onExport.emit_async(_pool, lInfo, lBuff);
}

void LogMngr::reg_ctx(std::string aProgName, std::string aThreadName)
{
    _processes[::getpid()] = aProgName;
    _threads[std::this_thread::get_id()] = aThreadName;
}