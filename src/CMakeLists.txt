cmake_minimum_required( VERSION 3.14 )
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper VERSION 1.0.1 DESCRIPTION "IB API Helper" )

# NOTE: If you want the date library to get the TZ database, you'll need curl
# Set HAS_REMOTE_API=1 to do that. With HAS_REMOTE_API set to 0, you won't need curl

if(MSVC)
    add_compile_options(/EHsc -DHAS_REMOTE_API=1)
else(MSVC)
    add_compile_options(-g -O0 -DHAS_REMOTE_API=1)
endif(MSVC)

file(GLOB ib_helper "ib_helper/*.cpp" )
file(GLOB util_src "util/*.cpp" )
file(GLOB date_src "../date/src/*.cpp" )
file(GLOB ib_src ${IB_CLIENT_DIR}/*.cpp)

find_package(Boost 1.69 COMPONENTS log log_setup REQUIRED)

if(MSVC)
else(MSVC)
    find_package(CURL REQUIRED)
endif(MSVC)

add_library( ib_api ${ib_src} )

target_include_directories( ib_api PRIVATE
    ${IB_CLIENT_DIR}
)

add_library( ib_helper ${util_src} ${ib_helper} ${date_src} )

target_include_directories( ib_helper PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/../date/include 
    ${IB_CLIENT_DIR}
    ${Boost_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
)
set_target_properties(ib_helper PROPERTIES VERSION ${PROJECT_VERSION})
