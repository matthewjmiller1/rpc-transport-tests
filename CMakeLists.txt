cmake_minimum_required(VERSION 3.17)

project(rpc_transport)

find_package(Protobuf REQUIRED)
find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(folly REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem regex context
  program_options)

find_library(DOUBLE-CONVERSION double-conversion)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/rt_client_server/rt.proto)

add_executable(rt_client src/rt_client_server/rt_client.cc
  ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(rt_server src/rt_client_server/rt_server.cc
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(rt_client ${Protobuf_LIBRARIES})
target_link_libraries(rt_server ${Protobuf_LIBRARIES})

# rsocket hello world tests

add_executable(
  rsocket_test_server
  src/transport_hello_world/rsocket/RequestResponseHelloWorld_Server.cpp
)

set(RSOCKET_LIBRARIES
  ReactiveSocket
  folly
  ${Boost_LIBRARIES}
  ${DOUBLE-CONVERSION}
  event
  iberty
  ${CMAKE_DL_LIBS}
  yarpl
  glog::glog
  gflags)

target_link_libraries(
  rsocket_test_server
  ${RSOCKET_LIBRARIES})

add_executable(
  rsocket_test_client
  src/transport_hello_world/rsocket/RequestResponseHelloWorld_Client.cpp
)

target_link_libraries(
  rsocket_test_client
  ${RSOCKET_LIBRARIES}
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY})
