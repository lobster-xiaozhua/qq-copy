cmake_minimum_required(VERSION 3.22.1)

project(qqchat_core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(qqchat_core SHARED
    native-lib.cpp
    chat_client.cpp
    protocol_codec.cpp
)

target_include_directories(qqchat_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(log-lib log)

target_link_libraries(qqchat_core
    ${log-lib}
)
