#pragma once

namespace tll {
struct StatCCI
{
    size_t push_size=0, pop_size=0;
    size_t push_count=0, pop_count=0;
    size_t push_hit_count=0, pop_hit_count=0;
    size_t push_miss_count=0, pop_miss_count=0;
};
}

#include "lf/ccbuffer.h"
#include "mt/ccbuffer.h"
