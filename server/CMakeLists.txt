cmake_minimum_required(VERSION 3.20)
project(qqchat_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

option(ENABLE_LTO "Enable Link-Time Optimization" ON)
option(ENABLE_CCACHE "Use ccache if available" ON)

if(ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "ccache found: ${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
if(ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_OK OUTPUT LTO_ERROR)
    if(LTO_OK)
        message(STATUS "LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "LTO not supported: ${LTO_ERROR}")
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Wall -Wextra -Wpedantic -g -O0 -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native -fno-exceptions -fno-rtti)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)
pkg_check_modules(HIREDIS REQUIRED hiredis)

add_executable(qqchat_server
    src/chat_server.cpp
    src/connection_manager.cpp
    src/message_handler.cpp
    src/database_pool.cpp
    src/cache_manager.cpp
    src/protocol_codec.cpp
    src/main.cpp
)

target_include_directories(qqchat_server
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MYSQL_INCLUDE_DIRS}
        ${HIREDIS_INCLUDE_DIRS}
)

target_link_directories(qqchat_server
    PRIVATE
        ${MYSQL_LIBRARY_DIRS}
        ${HIREDIS_LIBRARY_DIRS}
)

target_link_libraries(qchat_server
    PRIVATE
        Boost::system
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
        ${MYSQL_LIBRARIES}
        ${HIREDIS_LIBRARIES}
        pthread
        dl
)

target_compile_options(qqchat_server PRIVATE
    ${MYSQL_CFLAGS_OTHER}
    ${HIREDIS_CFLAGS_OTHER}
)

install(TARGETS qqchat_server DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/server.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/qqchat)

message(STATUS "========================================")
message(STATUS "  QQChat Server Build Configuration")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  LTO: ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "========================================")
