cmake_minimum_required( VERSION 3.25 )
set(CMAKE_CXX_STANDARD 20)

project ( ib_helper VERSION 1.0.1 DESCRIPTION "IB API Helper" )

file(GLOB ib_helper "ib_helper/*.cpp" )
file(GLOB util_src "util/*.cpp" )
file(GLOB ib_src ${IB_CLIENT_SOURCE_DIR}/CppClient/client/*.cpp)

if(HH_DATELIB)
    file(GLOB date_src "../date/src/*.cpp" )
    find_package(CURL REQUIRED)
endif(HH_DATELIB)

find_package(Boost 1.69 COMPONENTS log log_setup REQUIRED)
find_package(Protobuf REQUIRED)

file(GLOB PROTO_FILES ${IB_CLIENT_SOURCE_DIR}/proto/*.proto)

add_library( ib_api 
    ${ib_src} 
)

protobuf_generate(
    LANGUAGE CPP 
    TARGET ib_api 
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${IB_CLIENT_SOURCE_DIR}/proto
)

target_include_directories(ib_api INTERFACE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ib_helper
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)

if(MSVC)
    target_compile_options(ib_api PRIVATE /EHsc)
else(MSVC)
    target_compile_options(ib_api PRIVATE -g -O0)
endif(MSVC)

if(HH_DATELIB)
    target_compile_definitions(ib_api PRIVATE -DHAS_REMOTE_API=1 -DAUTO_DOWNLOAD=1)
    target_include_directories( ib_api PRIVATE
        ${CURL_INCLUDE_DIRS}
    )
endif(HH_DATELIB)

target_compile_definitions(ib_api PRIVATE BOOST_ALL_DYN_LINK)

target_include_directories( ib_api PRIVATE
    ${IB_CLIENT_SOURCE_DIR}/CppClient/client
    ${Protobuf_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(ib_api PUBLIC
    dfp
    protobuf::libprotobuf
)

add_library( ib_helper ${util_src} ${ib_helper} ${date_src} )

if(MSVC)
    target_compile_options(ib_helper PRIVATE /EHsc)
else(MSVC)
    target_compile_options(ib_helper PRIVATE -g -O0)
endif(MSVC)

if(HH_DATELIB)
    target_compile_definitions(ib_helper PRIVATE -DHH_DATELIB=1 -DHAS_REMOTE_API=1 -DAUTO_DOWNLOAD=1)
    target_include_directories( ib_helper PRIVATE 
        ${CMAKE_CURRENT_LIST_DIR}/../date/include 
        ${CURL_INCLUDE_DIRS}
    )
    target_link_libraries(ib_helper PRIVATE
        CURL::libcurl
    )
endif(HH_DATELIB)

target_compile_definitions(ib_helper PRIVATE BOOST_ALL_DYN_LINK)

target_include_directories( ib_helper PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/util
    ${IB_CLIENT_SOURCE_DIR}/CppClient/client
    ${Boost_INCLUDE_DIR}
)

target_link_libraries(ib_helper PRIVATE
    ib_api
    dfp
    Boost::log
    Boost::log_setup
    protobuf::libprotobuf
)

set_target_properties(ib_helper PROPERTIES VERSION ${PROJECT_VERSION})
