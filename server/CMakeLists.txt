cmake_minimum_required(VERSION 3.16)
project(qqchat_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Boost REQUIRED COMPONENTS system thread)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)
pkg_check_modules(HIREDIS REQUIRED hiredis)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS}
)

link_directories(
    ${MYSQL_LIBRARY_DIRS}
    ${HIREDIS_LIBRARY_DIRS}
)

add_executable(qqchat_server
    src/chat_server.cpp
    src/connection_manager.cpp
    src/message_handler.cpp
    src/database_pool.cpp
    src/cache_manager.cpp
    src/protocol_codec.cpp
    src/main.cpp
)

target_link_libraries(qqchat_server
    ${Boost_LIBRARIES}
    ${MYSQL_LIBRARIES}
    ${HIREDIS_LIBRARIES}
    pthread
)

target_compile_options(qqchat_server PRIVATE
    ${MYSQL_CFLAGS_OTHER}
    ${HIREDIS_CFLAGS_OTHER}
)

install(TARGETS qqchat_server DESTINATION bin)
install(FILES config/server.conf DESTINATION etc/qqchat)
