#include "Counter.hpp"

Counter::Counter() : _context("")
{
    using namespace std::chrono;
    _tbeg = std::chrono::high_resolution_clock::now();
}

Counter::Counter(std::string const &a_str) : _context(a_str)
{
    using namespace std::chrono;
    _tbeg = std::chrono::high_resolution_clock::now();
}

Counter::~Counter()
{
    using namespace std::chrono;
    high_resolution_clock::time_point lNow = high_resolution_clock::now();
    double diff = duration <double, std::milli> (lNow - _tbeg).count();
    Log::ins()->log(static_cast<int>(LogType::Prof), false, "%s PROF %s %f ms\n", Log::ins()->prepare_pre().data(), _context.data(), diff);
}