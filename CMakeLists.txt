cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -u _printf_float -u _scanf_float")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME FKH750XBH6_LUA)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    # Core/Src/uart_dyn_rx.c
    Core/Src/terminal.c
)

set(Lua_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/hardware_bindings.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/embedded_lua.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lapi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lauxlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lbaselib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lcode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lcorolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lctype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/ldblib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/ldebug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/ldo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/ldump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lgc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/linit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/liolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/llex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lmathlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/loadlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lobject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lopcodes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/loslib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lparser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lstate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lstring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lstrlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/ltable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/ltablib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/ltm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lundump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lutf8lib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Src/lzio.c
)
set(LUA_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/LUA_Lib/Inc
)

set(MX_LINK_LIBS
    LUA_Library
)

add_library(LUA_Library OBJECT)
target_sources(LUA_Library PRIVATE ${Lua_Src})
target_include_directories(stm32cubemx INTERFACE ${LUA_Include_Dirs})
target_link_libraries(LUA_Library PUBLIC stm32cubemx)

target_link_libraries(${CMAKE_PROJECT_NAME}
    LUA_Library
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
