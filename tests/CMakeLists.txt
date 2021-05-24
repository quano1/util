#MIT License
#Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

cmake_minimum_required(VERSION 3.16.2 FATAL_ERROR)
project(tll_tests VERSION 1.0.1 LANGUAGES CXX)

add_executable(tll_tests)
target_compile_features(tll_tests PRIVATE cxx_std_17)

include(ProcessorCount)
ProcessorCount(NUM_CPU)

target_sources(tll_tests
PRIVATE
    tll_tests.cc
    benchmark.cc
    $<$<EQUAL:${HAVE_GTEST},1>:unittests.cc>
    $<$<EQUAL:${HAVE_GTEST},1>:lffifo_unittests.cc>
    $<$<EQUAL:${HAVE_GTEST},1>:lffifo_stresstests.cc>
    $<$<EQUAL:${HAVE_GTEST},1>:log_unittests.cc>
    $<$<EQUAL:${HAVE_GTEST},1>:util_unittests.cc>
)

# print to console
# message( STATUS "Building with system: " ${CMAKE_SYSTEM_NAME})
# MESSAGE( STATUS "CC: " ${CMAKE_C_COMPILER})
# MESSAGE( STATUS "CXX: " ${CMAKE_CXX_COMPILER})
# message(STATUS "CC : ")
# execute_process(COMMAND ${CMAKE_C_COMPILER} -v)

# message(STATUS "CXX : ")
# execute_process(COMMAND ${CMAKE_CXX_COMPILER} -v)

# if(CMAKE_BUILD_TYPE STREQUAL "")
#   set(CMAKE_BUILD_TYPE Debug)
# endif()

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#   # Debug
#   set(CMAKE_C_FLAGS_DEBUG "-g -O0")
add_definitions(
    -DNUM_CPU=${NUM_CPU}
)
# elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
#   # Release
#   set(CMAKE_C_FLAGS_DEBUG "-O3")
#   add_definitions(
#   )
# endif()

# add_definitions(-D __EXT="1")

# set(CMAKE_SKIP_BUILD_RPATH TRUE)

# cxx flags
# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
# set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wno-deprecated-declarations -Wno-reorder -fopenmp")

# set environments
# set(PRJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
if(HAVE_GTEST)
	include_directories(
        ${GTest_SOURCE_DIR}/include 
        ${GTest_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/lockfree/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/config/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/parameter/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/mp11/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/assert/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/static_assert/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/type_traits/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/mpl/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/preprocessor/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/array/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/throw_exception/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/align/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/utility/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boostorg/predef/include
    )
	target_link_libraries(tll_tests gtest)
endif()

# add_test(NAME test COMMAND tll_tests)
add_custom_command(TARGET tll_tests PRE_BUILD
                  COMMAND cmake -E copy ${CMAKE_CURRENT_LIST_DIR}/../tools/plot*sh ${CMAKE_BINARY_DIR}/)

# include_directories(
#   ${PRJ_ROOT}
#   ${PRJ_ROOT}/libs
#   ${PRJ_ROOT}/tests
# )

# set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
# set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )

# set(PRJ_LINK_LIB
#   $<$<STREQUAL:${CMAKE_SYSTEM_NAME},QNX>:socket> 
#   $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Linux>:pthread>
# )

# add_subdirectory(libs)
# add_subdirectory(tests)

