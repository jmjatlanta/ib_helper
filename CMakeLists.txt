cmake_minimum_required(VERSION 3.25 )
cmake_policy(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project ( ib_helper )

set(IB_CLIENT_SOURCE_DIR "" CACHE FILEPATH "Path of IB/source directory")
set(HH_DATELIB OFF CACHE BOOL "Use Howard Hinnant Date Library")

if(NOT EXISTS ${IB_CLIENT_SOURCE_DIR}/CppClient/client/EWrapper.h)
    MESSAGE(SEND_ERROR "Cannot find EWrapper.h in ${IB_CLIENT_SOURCE_DIR}")
endif()

# tests need a FetchContent download of googletest (and some need a running TWS), so only build
# them by default when ib_helper is the top-level project
option(IB_HELPER_BUILD_TESTS "Build the ib_helper unit tests" ${PROJECT_IS_TOP_LEVEL})

# date declares option(BUILD_SHARED_LIBS OFF), which would put BUILD_SHARED_LIBS in a parent
# project's cache; as a subproject, shadow it here instead (OFF is the default anyway)
if(NOT PROJECT_IS_TOP_LEVEL AND NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()

if(HH_DATELIB)
    add_subdirectory ( date )
endif(HH_DATELIB)

add_subdirectory ( IntelDFP )
add_subdirectory ( src )
if(IB_HELPER_BUILD_TESTS)
    add_subdirectory ( test )
endif()
