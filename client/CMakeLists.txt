cmake_minimum_required(VERSION 3.14)
project(client)

set(CMAKE_CXX_STANDARD 20)

add_executable(client client.cpp)

target_link_libraries(client pthread)
