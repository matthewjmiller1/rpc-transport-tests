cmake_minimum_required(VERSION 3.17)

project(rpc_transport)

find_package(Protobuf REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/rt_client_server/rt.proto)

add_executable(rt_client src/rt_client_server/rt_client.cc ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(rt_server src/rt_client_server/rt_server.cc ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(rt_client ${Protobuf_LIBRARIES})
target_link_libraries(rt_server ${Protobuf_LIBRARIES})
