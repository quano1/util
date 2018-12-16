#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <string>
#include <chrono>
#include <thread>
#include <unordered_map>

#include "log.hpp"
bool canLog(int log_type);

struct Counter
{
public:
    // Counter();

    Counter(std::string const &a_str, LogType aLogType);

    virtual ~Counter();

private:

    std::string _context;
    LogType _logType;
    std::chrono::high_resolution_clock::time_point _tbeg;

    static std::unordered_map<std::thread::id, int> _indents;

};

#endif // COUNTER_HPP
