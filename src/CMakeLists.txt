cmake_minimum_required( VERSION 3.14 )
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper VERSION 1.0.1 DESCRIPTION "IB API Helper" )

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

add_compile_definitions(BOOST_ALL_NO_LIB)
add_compile_definitions(BOOST_ALL_DYN_LINK)

if(MSVC)
    set(CMAKE_CXX_FLAGS "/EHsc -DHAS_REMOTE_API=0")
else(MSVC)
    set(CMAKE_CXX_FLAGS "-g -O0 -DHAS_REMOTE_API=0")
endif(MSVC)

file(GLOB ib_helper "ib_helper/*.cpp" )
file(GLOB util_src "util/*.cpp" )
file(GLOB date_src "../date/src/*.cpp" )
file(GLOB ib_src ${IB_CLIENT_DIR}/*.cpp)

find_package(Boost 1.69 COMPONENTS log log_setup REQUIRED)

add_library( ib_helper ${ib_src} ${util_src} ${ib_helper} ${date_src} )
target_include_directories( ib_helper PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/../date/include 
    ${IB_CLIENT_DIR}
    ${Boost_INCLUDE_DIR}
)
set_target_properties(ib_helper PROPERTIES VERSION ${PROJECT_VERSION})
