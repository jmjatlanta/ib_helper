cmake_minimum_required(VERSION 3.25 )
cmake_policy(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper_tests )

if(MSVC)
    SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(CMAKE_CXX_FLAGS "/EHsc -DHAS_REMOTE_API=0")
else(MSVC)
    set(CMAKE_CXX_FLAGS "-g -O0 -DHAS_REMOTE_API=0")
endif(MSVC)

file(GLOB ib_helper "../src/ib_helper/*.cpp" )
file(GLOB util_src "../src/util/*.cpp" )
file(GLOB test_src "./*.cpp" )
file(GLOB date_src "../date/src/*.cpp" )
file(GLOB ib_src "${IB_CLIENT_DIR}/*.cpp")

find_package(Boost 1.69 COMPONENTS log log_setup REQUIRED)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_executable( ib_helper_tests ${test_src} ${ib_src} ${util_src} ${ib_helper} ${date_src} )
target_include_directories(ib_helper_tests PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/../date/include
    ${CMAKE_SOURCE_DIR}/date/include
    ${CMAKE_SOURCE_DIR}/../src/ib_helper
    ${IB_CLIENT_DIR}
)

target_link_libraries(ib_helper_tests 
    dfp
    GTest::gtest
    GTest::gtest_main
    Boost::log
    Boost::log_setup
)

if(MSVC)
    target_compile_definitions(ib_helper_tests PRIVATE NOMINMAX)
endif(MSVC)