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

logos_module(
    NAME battleship
    SOURCES
        src/battleship_interface.h
        src/battleship_plugin.h
        src/battleship_plugin.cpp
    EXTERNAL_LIBS
        battleship
)
