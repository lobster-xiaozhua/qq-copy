cmake_minimum_required(VERSION 3.22.1)

project(qqchat_core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -flto -fno-exceptions -fno-rtti -Wall -Wextra)
    add_link_options(-flto)
else()
    add_compile_options(-O2 -Wall -Wextra)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

add_library(qqchat_core SHARED
    native-lib.cpp
    chat_client.cpp
    protocol_codec.cpp
)

target_include_directories(qqchat_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(log-lib log)
find_library(android-lib android)
find_library(utils-lib utils)

target_link_libraries(qqchat_core
    PRIVATE
        ${log-lib}
        ${android-lib}
        ${utils-lib}
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET qqchat_core POST_BUILD
        COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment --remove-section=.note
            $<TARGET_FILE:qqchat_core>
        COMMENT "Stripping symbols from ${PROJECT_NAME}"
    )
endif()

include(ProcessorCount)
ProcessorCount(NPROC)
if(NPROC EQUAL 0)
    set(NPROC 1)
endif()
message(STATUS "qqchat_core: build type=${CMAKE_BUILD_TYPE}, parallel=${NPROC}")
