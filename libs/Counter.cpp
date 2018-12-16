#include <string>
#include <chrono>
#include <thread>
#include <unordered_map>

#include "Counter.hpp"
#include "log.hpp"

std::unordered_map<std::thread::id, int> Counter::_indents;

Counter::Counter() : _context("")
{
    using namespace std::chrono;
    if(!canLog(static_cast<int>(LogType::Prof))) return;
    _tbeg = std::chrono::high_resolution_clock::now();
    _indents[std::this_thread::get_id()]++;
}

Counter::Counter(std::string const &a_str) : _context(a_str)
{
    using namespace std::chrono;
    if(!canLog(static_cast<int>(LogType::Prof))) return;
    _tbeg = std::chrono::high_resolution_clock::now();
    _indents[std::this_thread::get_id()]++;
}

Counter::~Counter()
{
    using namespace std::chrono;
    if(!canLog(static_cast<int>(LogType::Prof))) return;
    _indents[std::this_thread::get_id()]--;
    _LOGD("%d", _indents[std::this_thread::get_id()]);

    high_resolution_clock::time_point lNow = high_resolution_clock::now();
    double diff = duration <double, std::milli> (lNow - _tbeg).count();
    Log::ins()->log(static_cast<int>(LogType::Prof), false, "%s PROF %*s%s %f ms", Log::ins()->preInit().data(), _indents[std::this_thread::get_id()] * 4, "", _context.data(), diff);
    // Log::ins()->log(static_cast<int>(LogType::Prof), false, "%s PROF %s %f ms", Log::ins()->preInit().data(), _context.data(), diff);
}
