cmake_minimum_required( VERSION 3.14 )

project ( ib_ts )

set(CMAKE_CXX_FLAGS "-g -O0")

file(GLOB ib_src "ib_api/client/*.cpp")
file(GLOB ib_helper "ib_helper/*.cpp" )
file(GLOB util_src "util/*.cpp" )

add_executable( ib_ts ib_ts.cpp ${ib_src} ${util_src} ${ib_helper} )
target_link_libraries(ib_ts ${CMAKE_SOURCE_DIR}/../IntelRDFPMathLib20U2/LIBRARY/gcc000libbid.a )

add_executable( TickLogger TickLogger.cpp ${ib_src} ${util_src} ${ib_helper} )
target_link_libraries(TickLogger ${CMAKE_SOURCE_DIR}/../IntelRDFPMathLib20U2/LIBRARY/gcc000libbid.a )
