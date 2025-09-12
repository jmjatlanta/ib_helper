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

if(HH_DATELIB)
    add_subdirectory ( date )
endif(HH_DATELIB)

add_subdirectory ( IntelDFP )
add_subdirectory ( src )
add_subdirectory ( test )
