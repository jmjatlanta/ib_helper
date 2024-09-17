cmake_minimum_required(VERSION 3.25 )
cmake_policy(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper_tests )

file(GLOB ib_helper "../src/ib_helper/*.cpp" )
file(GLOB util_src "../src/util/*.cpp" )
file(GLOB test_src "./*.cpp" )

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

if(HH_DATELIB)
    file(GLOB date_src "../date/src/*.cpp" )
    find_package(CURL REQUIRED)
endif(HH_DATELIB)

find_package(Boost 1.74 COMPONENTS log log_setup REQUIRED)

add_executable( ib_helper_tests 
    ${test_src} 
    ${util_src} 
    ${ib_helper} 
    ${date_src} 
)

target_compile_definitions(ib_helper_tests PRIVATE BOOST_ALL_DYN_LINK)

if(MSVC)
    SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    target_compile_options(ib_helper_tests PRIVATE /EHsc)
    target_compile_definitions(ib_helper_tests PRIVATE NOMINMAX)
else(MSVC)
    target_compile_options(ib_helper_tests PRIVATE -g -O0)
endif(MSVC)

if(HH_DATELIB)
    target_compile_definitions(ib_helper_tests PRIVATE -DHH_DATELIB=1 -DHAS_REMOTE_API=1)
    target_include_directories(ib_helper_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../date/include
        ${CURL_INCLUDE_DIRS}
    )
    target_link_libraries(ib_helper_tests 
        ${CURL_LIBRARIES}
    )
endif(HH_DATELIB)

target_include_directories(ib_helper_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ib_helper
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/util
    ${IB_CLIENT_DIR}
)

target_link_libraries(ib_helper_tests 
    ib_helper
    dfp
    ib_api
    GTest::gtest
    GTest::gmock_main
    Boost::log
    Boost::log_setup
)

