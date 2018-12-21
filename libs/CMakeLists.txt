cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

SET(TARGET lltlog)

SET(TARGET_SRC
	${PRJ_ROOT}/libs/log.hpp
	${PRJ_ROOT}/libs/log.cpp
	${PRJ_ROOT}/libs/Counter.hpp
	${PRJ_ROOT}/libs/Counter.cpp
	${PRJ_ROOT}/libs/LMngr.hpp
	${PRJ_ROOT}/libs/LMngr.cpp
)

#TARGET-------------------------------------------------------------------------
# set target
ADD_LIBRARY(${TARGET} SHARED ${TARGET_SRC} )

TARGET_LINK_LIBRARIES(${TARGET}
    pthread
)

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
