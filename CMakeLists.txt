cmake_minimum_required(VERSION 3.25 )
cmake_policy(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project ( ib_helper )

set(IB_CLIENT_DIR "" CACHE FILEPATH "Path of IB/client directory")

if(NOT EXISTS ${IB_CLIENT_DIR}/EWrapper.h)
    MESSAGE(SEND_ERROR "Cannot find EWrapper.h in ${IB_CLIENT_DIR}")
endif()

add_subdirectory ( IntelDFP )
add_subdirectory ( src )
add_subdirectory ( test )
