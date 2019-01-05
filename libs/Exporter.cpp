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
