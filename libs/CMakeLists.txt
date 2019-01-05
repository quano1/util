cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

#TARGET-------------------------------------------------------------------------
# set target
set(TARGET lltlog)

set(TARGET_SRC
	${PRJ_ROOT}/libs/SimpleSignal.hpp
	${PRJ_ROOT}/libs/ThreadPool.hpp
	${PRJ_ROOT}/libs/LogMngr.hpp
	${PRJ_ROOT}/libs/LogMngr.cpp
	${PRJ_ROOT}/libs/Exporter.hpp
	${PRJ_ROOT}/libs/Exporter.cpp
)

add_library(${TARGET}_obj OBJECT ${TARGET_SRC} )

set_property(TARGET ${TARGET}_obj PROPERTY CXX_STANDARD 11)
set_property(TARGET ${TARGET}_obj PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${TARGET}_obj PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(${TARGET}_static STATIC $<TARGET_OBJECTS:${TARGET}_obj>)
add_library(${TARGET}.${PROJECT_VERSION} SHARED $<TARGET_OBJECTS:${TARGET}_obj>)
target_link_libraries(${TARGET}.${PROJECT_VERSION}
    pthread
)

#TARGET-------------------------------------------------------------------------
# set target
set(TARGET lltlogall)

set(TARGET_SRC
	${PRJ_ROOT}/libs/ExporterUDP.hpp
	${PRJ_ROOT}/libs/ExporterUDP.cpp
)

add_library(${TARGET}_obj OBJECT ${TARGET_SRC} )

set_property(TARGET ${TARGET}_obj PROPERTY CXX_STANDARD 11)
set_property(TARGET ${TARGET}_obj PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${TARGET}_obj PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(${TARGET}_static STATIC $<TARGET_OBJECTS:${TARGET}_obj>)
add_library(${TARGET} SHARED $<TARGET_OBJECTS:${TARGET}_obj>)
target_link_libraries(${TARGET}
    pthread
    lltlog.${PROJECT_VERSION}
)