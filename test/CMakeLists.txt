cmake_minimum_required(VERSION 3.10 )
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper_tests )

set(CMAKE_CXX_FLAGS "-g -O0 -DHAS_REMOTE_API=0")

file(GLOB ib_src "../src/ib_api/client/*.cpp")
file(GLOB ib_helper "../src/ib_helper/*.cpp" )
file(GLOB util_src "../src/util/*.cpp" )
file(GLOB test_src "./*.cpp" )
file(GLOB date_src "../date/src/*.cpp" )

add_executable( ib_helper_tests ${test_src} ${ib_src} ${util_src} ${ib_helper} ${date_src} )
target_include_directories(ib_helper_tests PRIVATE ../googletest/googletest/include
    ../date/include
)

target_link_libraries(ib_helper_tests 
    ${CMAKE_SOURCE_DIR}/../IntelRDFPMathLib20U2/LIBRARY/gcc000libbid.a 
    ${CMAKE_SOURCE_DIR}/build/lib/libgtest.a
    ${CMAKE_SOURCE_DIR}/build/lib/libgtest_main.a
    )

