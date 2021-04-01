#include <gtest/gtest.h>

// #include <omp.h>

// #include "../libs/util.h"
// #include "../libs/counter.h"
// #include "../libs/contiguouscircular.h"

int unittests(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}