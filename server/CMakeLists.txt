cmake_minimum_required(VERSION 3.16)
project(qqchat_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(MySQL REQUIRED)
find_package(hiredis REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIR}
    ${HIREDIS_INCLUDE_DIR}
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
    ${MYSQL_LIBRARY}
    ${HIREDIS_LIBRARY}
    pthread
)

install(TARGETS qqchat_server DESTINATION bin)
install(FILES config/server.conf DESTINATION etc/qqchat)
