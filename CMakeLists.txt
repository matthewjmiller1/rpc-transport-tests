cmake_minimum_required(VERSION 2.8)

project(rpc_transport)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(rt_client src/rt_client.cc)
add_executable(rt_server src/rt_server.cc)
