cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

link_directories(${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

#TARGET-------------------------------------------------------------------------
# set target
set(TARGET test_lmngr)

set(TARGET_SRC
	${PRJ_ROOT}/tests/test_lmngr.cpp
	${PRJ_ROOT}/tests/Base.cpp
)

add_executable(${TARGET} ${TARGET_SRC} )

set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

target_link_libraries(${TARGET}
    lltlogall_static
    lltlog_static
    ${PRJ_LINK_LIB}
)

set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

#TARGET-------------------------------------------------------------------------
# set target
set(TARGET server_udp)

set(TARGET_SRC
	${PRJ_ROOT}/tests/server_udp.cpp
)

add_executable(${TARGET} ${TARGET_SRC} )
target_link_libraries(${TARGET}
  	${PRJ_LINK_LIB}
)

set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

#TARGET-------------------------------------------------------------------------
# set target
set(TARGET test_lmngrc)

set(TARGET_SRC
	${PRJ_ROOT}/tests/test_lmngr.c
)

add_executable(${TARGET} ${TARGET_SRC} )

set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

target_link_libraries(${TARGET}
    lltlog.${PROJECT_VERSION}
  	${PRJ_LINK_LIB}
)

set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${TARGET} PROPERTY LINKER_LANGUAGE CXX)

# SET_SOURCE_FILES_PROPERTIES( test_lmngr.c PROPERTIES LANGUAGE C )