cmake_minimum_required(VERSION 3.14)
project(BattleshipModulePlugin LANGUAGES CXX)

# Include the Logos Module CMake helper
if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Compile protobuf messages for multiplayer serialization
find_package(Protobuf REQUIRED)
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_SRC ${PROTO_GEN_DIR}/battleship.pb.cc)
set(PROTO_HDR ${PROTO_GEN_DIR}/battleship.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR}
    COMMAND protobuf::protoc
        --cpp_out=${PROTO_GEN_DIR}
        -I${PROTO_DIR}
        ${PROTO_DIR}/battleship.proto
    DEPENDS ${PROTO_DIR}/battleship.proto
    COMMENT "Generating protobuf sources for battleship"
)

logos_module(
    NAME battleship
    SOURCES
        src/battleship_interface.h
        src/battleship_plugin.h
        src/battleship_plugin.cpp
        ${PROTO_SRC}
        ${PROTO_HDR}
    EXTERNAL_LIBS
        battleship
)

target_include_directories(battleship_module_plugin PRIVATE ${PROTO_GEN_DIR})
target_link_libraries(battleship_module_plugin PRIVATE protobuf::libprotobuf)
