cmake_minimum_required(VERSION 3.17)

project(rsocket_hello_world)

find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(folly REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem regex context
  program_options)

find_library(DOUBLE-CONVERSION double-conversion)

add_executable(rsocket_test_server RequestResponseHelloWorld_Server.cpp)

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

add_executable(rsocket_test_client RequestResponseHelloWorld_Client.cpp)

target_link_libraries(
  rsocket_test_client
  ${RSOCKET_LIBRARIES}
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY})
