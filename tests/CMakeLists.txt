cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

SET(TARGET test_log)

SET(TARGET_SRC
	${PRJ_ROOT}/tests/test_log.cpp
)

#TARGET-------------------------------------------------------------------------
# set target
# ADD_LIBRARY(${TARGET} SHARED ${TARGET_SRC} )
ADD_EXECUTABLE(${TARGET} ${TARGET_SRC} )

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

TARGET_LINK_LIBRARIES(${TARGET}
    lltlog
)

INCLUDE_DIRECTORIES(
    ${PRJ_ROOT}/libs
)
