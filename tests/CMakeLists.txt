cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

#TARGET-------------------------------------------------------------------------
# set target
SET(TARGET test_log)

SET(TARGET_SRC
	${PRJ_ROOT}/tests/test_log.cpp
)

ADD_EXECUTABLE(${TARGET} ${TARGET_SRC} )

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

TARGET_LINK_LIBRARIES(${TARGET}
    lltlog
)


#TARGET-------------------------------------------------------------------------
# set target
SET(TARGET test_signal)

SET(TARGET_SRC
	${PRJ_ROOT}/tests/test_signal.cpp
)

ADD_EXECUTABLE(${TARGET} ${TARGET_SRC} )

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

#TARGET-------------------------------------------------------------------------
# set target
SET(TARGET test_lmngr)

SET(TARGET_SRC
	${PRJ_ROOT}/tests/test_lmngr.cpp
)

ADD_EXECUTABLE(${TARGET} ${TARGET_SRC} )

TARGET_LINK_LIBRARIES(${TARGET}
    pthread
)

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

#TARGET-------------------------------------------------------------------------
# set target
SET(TARGET test_ss)

SET(TARGET_SRC
	${PRJ_ROOT}/tests/test_ss.cpp
)

ADD_EXECUTABLE(${TARGET} ${TARGET_SRC} )

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

#TARGET-------------------------------------------------------------------------
# set target
SET(TARGET server_udp)

SET(TARGET_SRC
	${PRJ_ROOT}/tests/server_udp.cpp
)

ADD_EXECUTABLE(${TARGET} ${TARGET_SRC} )

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
