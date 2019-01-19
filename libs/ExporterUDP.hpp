#pragma once

#include <Exporter.hpp>

#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <sstream>
#include <string>

#include <cstdarg>
#include <iomanip>

#include <netinet/in.h>
#include <unistd.h> // close

namespace llt {

class EUDPClt : public Exporter
{
public:
    EUDPClt(std::string &&aHost, uint16_t aPort);
    EUDPClt(std::string const &aHost, uint16_t aPort);

    virtual int on_init();
    virtual void on_deinit();
    virtual void on_export(LogInfo const &aLogInfo);

protected:
    std::string _host;
    uint16_t _port;
    int _fd;
    sockaddr_in _svrAddr;
};

class EUDPSvr : public Exporter
{
public:
    EUDPSvr(uint16_t aPort);
    virtual ~EUDPSvr();

    virtual int on_init();
    virtual void on_deinit();
    virtual void on_export(LogInfo const &aLogInfo);

protected:
    uint16_t _port;
    int _fd;
    bool _stop;
    std::vector<sockaddr_in> _cltAddrs;
    std::thread _thrd;
};

} // llt