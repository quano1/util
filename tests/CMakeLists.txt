cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

#TARGET-------------------------------------------------------------------------
# set target
SET(TARGET test_lmngr)

SET(TARGET_SRC
	${PRJ_ROOT}/tests/test_lmngr.cpp
	${PRJ_ROOT}/tests/log2.cpp
)

ADD_EXECUTABLE(${TARGET} ${TARGET_SRC} )

SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

TARGET_LINK_LIBRARIES(${TARGET}
    lltlog.so
    pthread
)

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

#TARGET-------------------------------------------------------------------------
# set target
SET(TARGET test_lmngrc)

SET(TARGET_SRC
	${PRJ_ROOT}/tests/test_lmngr.c
)

ADD_EXECUTABLE(${TARGET} ${TARGET_SRC} )

SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

TARGET_LINK_LIBRARIES(${TARGET}
    lltlog
    pthread
)

SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
SET_PROPERTY(TARGET ${TARGET} PROPERTY LINKER_LANGUAGE CXX)

# SET_SOURCE_FILES_PROPERTIES( test_lmngr.c PROPERTIES LANGUAGE C )