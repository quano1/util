cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

SET(TARGET lltlog)

SET(TARGET_SRC
	${PRJ_ROOT}/libs/SimpleSignal.hpp
	${PRJ_ROOT}/libs/ThreadPool.hpp
	${PRJ_ROOT}/libs/LogMngr.hpp
	${PRJ_ROOT}/libs/LogMngr.cpp
	${PRJ_ROOT}/libs/Exporter.hpp
	${PRJ_ROOT}/libs/Exporter.cpp
)

#TARGET-------------------------------------------------------------------------
# set target
ADD_LIBRARY(${TARGET} STATIC ${TARGET_SRC} )

TARGET_LINK_LIBRARIES(${TARGET}
    pthread
)



SET(TARGET lltlogall)

SET(TARGET_SRC
	${PRJ_ROOT}/libs/ExporterUDP.hpp
	${PRJ_ROOT}/libs/ExporterUDP.cpp
)

#TARGET-------------------------------------------------------------------------
# set target
ADD_LIBRARY(${TARGET} STATIC ${TARGET_SRC} )

TARGET_LINK_LIBRARIES(${TARGET}
    pthread
    lltlog
)

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
