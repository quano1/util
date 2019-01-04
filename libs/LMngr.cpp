#include <LMngr.hpp>

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
    _ofs.open(_f, std::ios::out | std::ios::app );
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
}


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
    LOGD("%d", _port);
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


LogMngr::LogMngr(std::vector<Export *> const &aExpList, size_t aWorkers)
{
    _appName = std::to_string((size_t)::getpid());
    for(auto _export : aExpList)
    {
        add(_export);
    }

    init(aWorkers);
}

LogMngr::~LogMngr() 
{
    deinit();
    for(auto lp : _exportContainer)
    {
        delete lp;
    }
}

void LogMngr::init(size_t aWorkers)
{    
    if(aWorkers)
    {
        _pool.add_workers(aWorkers);
    }
    
    _sigInit.emit();
}

void LogMngr::deinit()
{
    _pool.stop(_forceStop);
    _sigDeinit.emit();
}

void LogMngr::add(Export *aExport)
{
    _exportContainer.push_back(aExport);
    Export *lIns = _exportContainer.back();
    size_t lId = _sigExport.connect(Simple::slot(lIns, &Export::on_export));
    lId = _sigInit.connect(Simple::slot(lIns, &Export::on_init));
    lId = _sigDeinit.connect(Simple::slot(lIns, &Export::on_deinit));
}

void LogMngr::log(LogType aLogType, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = Util::format(fmt, args);
    va_end (args);

    std::thread::id lKey = std::this_thread::get_id();
    if(_ctx[lKey].empty()) _ctx[lKey] = _appName + Util::SEPARATOR + Util::to_string(lKey);
    LogInfo lInfo = {aLogType, _indents[lKey], std::chrono::system_clock::now(), _ctx[lKey], std::move(lBuff) };
    _sigExport.emit(lInfo);
}


void LogMngr::log_async(LogType aLogType, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = Util::format(fmt, args);
    va_end (args);
 
    std::thread::id lKey = std::this_thread::get_id();

    if(_ctx[lKey].empty()) _ctx[lKey] = _appName + Util::SEPARATOR + Util::to_string(lKey);
    LogInfo lInfo = {aLogType, _indents[lKey], std::chrono::system_clock::now(), _ctx[lKey], std::move(lBuff) };
    _sigExport.emit_async(_pool, lInfo);
}

void ThreadPool::stop(bool aForce)
{
    if(_stop) return;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        _stop = true;
        if(aForce) tasks = std::queue< std::function<void()> >();
    }
    condition.notify_all();

    for(std::thread &worker: workers)
    {
        worker.join();
    }
}

void ThreadPool::add_workers(size_t threads)
{
    for(size_t i = 0;i<threads;++i)
    {
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->_stop || !this->tasks.empty(); });
                        if(this->_stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
    }
}

// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads)
    :   _stop(false)
{
    add_workers(threads);
}


// the destructor joins all threads
ThreadPool::~ThreadPool()
{
    stop(true);
}
