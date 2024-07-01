cmake_minimum_required(VERSION 3.25 )
cmake_policy(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper_tests )

file(GLOB ib_helper "../src/ib_helper/*.cpp" )
file(GLOB util_src "../src/util/*.cpp" )
file(GLOB test_src "./*.cpp" )
file(GLOB date_src "../date/src/*.cpp" )

find_package(CURL REQUIRED)
find_package(Boost 1.69 COMPONENTS log log_setup REQUIRED)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_executable( ib_helper_tests 
    ${test_src} 
    ${util_src} 
    ${ib_helper} 
    ${date_src} 
)

if(MSVC)
    SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    target_compile_options(ib_helper_tests PRIVATE /EHsc)
    target_compile_definitions(ib_helper_tests PRIVATE -DHAS_REMOTE_API=1)
else(MSVC)
    target_compile_options(ib_helper_tests PRIVATE -g -O0)
    target_compile_definitions(ib_helper_tests PRIVATE -DHAS_REMOTE_API=1)
endif(MSVC)

target_include_directories(ib_helper_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../date/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ib_helper
    ${IB_CLIENT_DIR}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(ib_helper_tests 
    ib_helper
    dfp
    ib_api
    GTest::gtest
    GTest::gtest_main
    Boost::log
    Boost::log_setup
    ${CURL_LIBRARIES}
)

if(MSVC)
    target_compile_definitions(ib_helper_tests PRIVATE NOMINMAX)
endif(MSVC)

