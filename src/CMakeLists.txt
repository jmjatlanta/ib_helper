cmake_minimum_required( VERSION 3.14 )
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper VERSION 1.0.1 DESCRIPTION "IB API Helper" )

add_definitions(-DBOOST_ALL_DYN_LINK)

if(MSVC)
    set(CMAKE_CXX_FLAGS "/EHsc -DHAS_REMOTE_API=0")
else(MSVC)
    set(CMAKE_CXX_FLAGS "-g -O0 -DHAS_REMOTE_API=0")
endif(MSVC)

file(GLOB ib_src "ib_api/client/*.cpp")
file(GLOB ib_helper "ib_helper/*.cpp" )
file(GLOB util_src "util/*.cpp" )
file(GLOB date_src "../date/src/*.cpp" )

find_package(Boost 1.69 COMPONENTS log log_setup REQUIRED)

add_library( ib_helper ${ib_src} ${util_src} ${ib_helper} ${date_src} )
target_include_directories( ib_helper PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/../date/include 
    ${Boost_INCLUDE_DIR}
)
set_target_properties(ib_helper PROPERTIES VERSION ${PROJECT_VERSION})
