#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <string>
#include <chrono>

#include "log.hpp"

class Counter
{
public:
    Counter();

    Counter(std::string const &a_str);

    virtual ~Counter();

    std::string _context;
    std::chrono::high_resolution_clock::time_point _tbeg;

};

#endif // COUNTER_HPP
