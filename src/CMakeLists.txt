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
find_package(Protobuf CONFIG REQUIRED)

file(GLOB PROTO_FILES ${IB_CLIENT_SOURCE_DIR}/proto/*.proto)

add_library( ib_api
    ${ib_src}
)
add_library( ib_helper::ib_api ALIAS ib_api )

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
endif(MSVC)

if(HH_DATELIB)
    target_compile_definitions(ib_api PRIVATE -DHAS_REMOTE_API=1 -DAUTO_DOWNLOAD=1)
    target_include_directories( ib_api PRIVATE
        ${CURL_INCLUDE_DIRS}
    )
endif(HH_DATELIB)

target_compile_definitions(ib_api PRIVATE BOOST_ALL_DYN_LINK)

# PUBLIC: the TWS API headers (and the generated .pb.h files they include) are part of
# ib_helper's public headers, e.g. IBConnector.hpp includes EWrapper.h
target_include_directories( ib_api PUBLIC
    ${IB_CLIENT_SOURCE_DIR}/CppClient/client
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_include_directories( ib_api PRIVATE
    ${Protobuf_INCLUDE_DIR}
)

target_link_libraries(ib_api PUBLIC
    dfp
    protobuf::libprotobuf
)

add_library( ib_helper ${util_src} ${ib_helper} ${date_src} )
add_library( ib_helper::ib_helper ALIAS ib_helper )

if(MSVC)
    target_compile_options(ib_helper PRIVATE /EHsc)
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

# PUBLIC: ib_helper's headers need the TWS API headers and types (Decimal, protobuf messages)
target_link_libraries(ib_helper PUBLIC
    ib_api
)
target_link_libraries(ib_helper PRIVATE
    dfp
    Boost::log
    Boost::log_setup
    protobuf::libprotobuf
)

set_target_properties(ib_helper PROPERTIES VERSION ${PROJECT_VERSION})
