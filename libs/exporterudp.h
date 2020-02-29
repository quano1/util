#pragma once

#include "exporter.h"

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

class EUDPClt : public Exporter<EUDPClt>
{
public:
    EUDPClt(std::string host, uint16_t port) : host_(std::move(host)), port_(port) {}
    ~EUDPClt() {onDeinit();}
    UTIL_INLINE int onInit();
    UTIL_INLINE void onDeinit();
    UTIL_INLINE void onExport(LogInfo const &);

protected:
    std::string host_;
    uint16_t port_;
    int fd_;
    sockaddr_in svr_addr_;
};

class EUDPSvr : public Exporter<EUDPSvr>
{
public:
    EUDPSvr(uint16_t port) : port_(port), is_running_(false) {}
    ~EUDPSvr() { onDeinit(); }

    UTIL_INLINE int onInit();
    UTIL_INLINE void onDeinit();
    UTIL_INLINE void onExport(LogInfo const &);

protected:
    uint16_t port_;
    int fd_;
    bool is_running_;
    std::vector<sockaddr_in> clt_addrs_;
    std::thread thrd_;
};

} // llt


#ifndef STATIC_LIB
#include "exporterudp.cc"
#endif
