#MIT License
#Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

cmake_minimum_required(VERSION 3.16.2 FATAL_ERROR)
project(tll_utilities VERSION 1.0.1 LANGUAGES CXX)

# print to console
message( STATUS "Building with system: " ${CMAKE_SYSTEM_NAME})
# MESSAGE( STATUS "CC: " ${CMAKE_C_COMPILER})
# MESSAGE( STATUS "CXX: " ${CMAKE_CXX_COMPILER})
# message(STATUS "CC : ")
# execute_process(COMMAND ${CMAKE_C_COMPILER} -v)

# message(STATUS "CXX : ")
# execute_process(COMMAND ${CMAKE_CXX_COMPILER} -v)

find_package(cxxopts REQUIRED)

find_package(GTest QUIET)
if(GTEST_FOUND)
    set(HAVE_GTEST 1)
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GTest_CXX_FLAGS}")
    # set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
    set(HAVE_GTEST 0)
    message(STATUS "Disabled GTEST")
endif()

find_package(Boost QUIET)
if(BOOST_FOUND)
    set(HAVE_BOOST 1)
else()
    set(HAVE_BOOST 0)
    message(STATUS "Disabled BOOST")
endif()

find_package(TBB QUIET)
if(TBB_FOUND)
    set(HAVE_TBB 1)
else()
    set(HAVE_TBB 0)
    message(STATUS "Disabled TBB")
endif()

CHECK_INCLUDE_FILE_CXX("concurrentqueue/concurrentqueue.h" HAVE_MC)
if(NOT HAVE_MC)
    set(HAVE_MC 0)
    message(STATUS "Disabled MoodyCamel")
endif()


find_package(OpenMP)
if (OPENMP_FOUND)
    set (HAVE_OPENMP 1)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
    set (HAVE_OPENMP 0)
    message(STATUS "Disabled OpenMP")
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")

# if(CMAKE_BUILD_TYPE STREQUAL "")
#   set(CMAKE_BUILD_TYPE Debug)
# endif()

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#   # Debug
#   set(CMAKE_C_FLAGS_DEBUG "-g -O0")
#   add_definitions(
#     -D__LLT_DEBUG__
#   )
# elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
#   # Release
#   set(CMAKE_C_FLAGS_DEBUG "-O3")
#   add_definitions(
#   )
# endif()



# add_definitions(-D __EXT="1")

set(CMAKE_SKIP_BUILD_RPATH TRUE)

# cxx flags
# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
# set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-reorder -fopenmp")

# set environments
set(PRJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${PRJ_ROOT}
    ${PRJ_ROOT}/libs
    ${PRJ_ROOT}/tests
)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )

# set(PRJ_LINK_LIB
#   $<$<STREQUAL:${CMAKE_SYSTEM_NAME},QNX>:socket>
#   $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Linux>:pthread>
# )


add_definitions(
    -DHAVE_GTEST=${HAVE_GTEST}
    -DHAVE_BOOST=${HAVE_BOOST}
    -DHAVE_TBB=${HAVE_TBB}
    -DHAVE_MC=${HAVE_MC}
    -DHAVE_OPENMP=${HAVE_OPENMP}
)

# add_subdirectory(libs)
add_subdirectory(tests)

