cmake_minimum_required(VERSION 3.17)

project(rpc_transport)

find_package(Protobuf REQUIRED)
find_package(gflags REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS rt.proto)

set(TRANSPORT_HDRS
  transport.hpp
  transports/null/null_transport.hpp)

set(TRANSPORT_LIBS
  null_transport)

add_library(transport SHARED transport.cc)
target_include_directories(transport PUBLIC .)

add_library(null_transport SHARED transports/null/null_transport.cc)
target_include_directories(null_transport PUBLIC .)
target_link_libraries(null_transport transport)

# Build source files
foreach(_target rt_client rt_server)
  add_executable(${_target} "${_target}.cc"
    ${PROTO_SRCS} ${PROTO_HDRS} ${TRANSPORT_HDRS})
  target_link_libraries(${_target} ${Protobuf_LIBRARIES} ${TRANSPORT_LIBS}
    gflags)
  target_include_directories(${_target} PRIVATE .)
endforeach()
