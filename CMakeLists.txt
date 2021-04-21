cmake_minimum_required(VERSION 3.19)
project(client_cpp)

set(CMAKE_CXX_STANDARD 20)

add_executable(client_cpp main.cpp)
target_link_libraries(client_cpp Winhttp)