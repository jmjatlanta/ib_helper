cmake_minimum_required(VERSION 3.25 )
cmake_policy(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)

project ( tick )

if(MSVC)
    SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(CMAKE_CXX_FLAGS "/EHsc -DHAS_REMOTE_API=0")
else(MSVC)
    set(CMAKE_CXX_FLAGS "-g -O0 -DHAS_REMOTE_API=0")
endif(MSVC)

find_package(Boost 1.69 COMPONENTS log log_setup REQUIRED)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_library( tick 
    TickCapture.cpp
)

target_include_directories(tick PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../date/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ib_helper
    ${IB_CLIENT_DIR}
)

target_link_libraries(tick 
    ib_helper
    dfp
    ib_api
    Boost::log
    Boost::log_setup
)

if(MSVC)
    target_compile_definitions(tick PRIVATE NOMINMAX)
endif(MSVC)

