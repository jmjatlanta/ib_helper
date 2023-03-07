cmake_minimum_required(VERSION 3.14 )
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project ( ib_helper )

add_subdirectory ( src )
add_subdirectory ( googletest )
add_subdirectory ( date )
add_subdirectory ( test )
