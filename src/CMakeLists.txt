cmake_minimum_required( VERSION 3.14 )
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper VERSION 1.0.1 DESCRIPTION "IB API Helper" )

set(CMAKE_CXX_FLAGS "-g -O0 -DHAS_REMOTE_API=0")

file(GLOB ib_src "ib_api/client/*.cpp")
file(GLOB ib_helper "ib_helper/*.cpp" )
file(GLOB util_src "util/*.cpp" )
file(GLOB date_src "../date/src/*.cpp" )

add_library( ib_helper ${ib_src} ${util_src} ${ib_helper} ${date_src} )
target_include_directories( ib_helper PRIVATE ../date/include )
set_target_properties(ib_helper PROPERTIES VERSION ${PROJECT_VERSION})
