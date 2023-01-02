cmake_minimum_required(VERSION 3.10 )

project ( ib_helper_tests )

set(CMAKE_CXX_FLAGS "-g -O0")

file(GLOB ib_src "../src/ib_api/client/*.cpp")
file(GLOB ib_helper "../src/ib_helper/*.cpp" )
file(GLOB util_src "../src/util/*.cpp" )
file(GLOB test_src "./*.cpp" )

#include_directories( ../vendor ../include  )
add_executable( ib_helper_tests ${test_src} ${ib_src} ${util_src} ${ib_helper} )

target_link_libraries(ib_helper_tests 
    /home/jmjatlanta/Development/cpp/IntelRDFPMathLib20U2/LIBRARY/gcc000libbid.a 
    /home/jmjatlanta/Development/cpp/ib_helper/build/lib/libgtest.a
    /home/jmjatlanta/Development/cpp/ib_helper/build/lib/libgtest_main.a
    )

