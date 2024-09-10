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
if(HH_DATELIB)
    file(GLOB date_src "../date/src/*.cpp" )
    find_package(CURL REQUIRED)
endif(HH_DATELIB)
file(GLOB ib_src ${IB_CLIENT_DIR}/*.cpp)

find_package(Boost 1.69 COMPONENTS log log_setup REQUIRED)

add_library( ib_api ${ib_src} )

if(MSVC)
    target_compile_options(ib_api PRIVATE /EHsc)
else(MSVC)
    target_compile_options(ib_api PRIVATE -g -O0)
endif(MSVC)

target_compile_definitions(ib_api PRIVATE BOOST_ALL_DYN_LINK)

target_include_directories( ib_api PRIVATE
    ${IB_CLIENT_DIR}
)

add_library( ib_helper ${util_src} ${ib_helper} ${date_src} )

if(MSVC)
    target_compile_options(ib_helper PRIVATE /EHsc)
else(MSVC)
    target_compile_options(ib_helper PRIVATE -g -O0)
endif(MSVC)

target_compile_definitions(ib_helper PRIVATE BOOST_ALL_DYN_LINK)
if(HH_DATELIB)
    target_compile_definitions(ib_helper PRIVATE -DHH_DATELIB=1 -DHAS_REMOTE_API=1 -DAUTO_DOWNLOAD=1)
    target_include_directories( ib_helper PRIVATE 
        ${CMAKE_CURRENT_LIST_DIR}/../date/include 
        ${CURL_INCLUDE_DIRS}
    )
endif(HH_DATELIB)

target_include_directories( ib_helper PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/util
    ${IB_CLIENT_DIR}
    ${Boost_INCLUDE_DIR}
)

set_target_properties(ib_helper PROPERTIES VERSION ${PROJECT_VERSION})
