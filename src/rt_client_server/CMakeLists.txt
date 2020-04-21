cmake_minimum_required(VERSION 3.17)

project(rpc_transport)

find_package(Protobuf REQUIRED)
find_package(gflags REQUIRED)
find_package(gRPC REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter)

find_program(GRPC_CPP_PLUGIN_EXECUTABLE NAMES grpc_cpp_plugin)

set(GRPC_LIBS gRPC::grpc++ gRPC::grpc++_reflection)

protobuf_generate_cpp(RT_PROTO_SRCS RT_PROTO_HDRS rt.proto)

list(APPEND TRANSPORT_HDRS transport.hpp)

add_library(transport STATIC transport.cc)
target_include_directories(transport PUBLIC .)

set(TRANSPORTS
  null
  grpc)

# Build libraries
foreach(_target ${TRANSPORTS})
  set(TRANSPORT_BASE ${_target}_transport)
  if ("${_target}" STREQUAL "grpc")
    get_filename_component(PROTO_FILE
      "transports/${_target}/${TRANSPORT_BASE}.proto" ABSOLUTE)
    get_filename_component(PROTO_PATH "${PROTO_FILE}" PATH)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})
    list(APPEND PROTO_LIBS ${Protobuf_LIBRARIES} ${GRPC_LIBS})
    set(GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${TRANSPORT_BASE}.grpc.pb.cc")
    set(GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${TRANSPORT_BASE}.grpc.pb.h")
    add_custom_command(
      OUTPUT "${GRPC_SRCS}" "${GRPC_HDRS}"
      COMMAND protobuf::protoc
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" -I "${PROTO_PATH}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}" "${PROTO_FILE}"
      DEPENDS "${PROTO_FILE}")
  endif()
  add_library(${_target}_transport STATIC
    transports/${_target}/${TRANSPORT_BASE}.cc ${PROTO_SRCS} ${PROTO_HDRS}
    ${GRPC_SRCS} ${GRPC_HDRS})
  target_include_directories(${TRANSPORT_BASE} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(${TRANSPORT_BASE} transport ${PROTO_LIBS})
  list(APPEND TRANSPORT_LIBS ${TRANSPORT_BASE})
  list(APPEND TRANSPORT_HDRS transports/${_target}/${TRANSPORT_BASE}.hpp)
endforeach()

# Build source files
foreach(_target rt_client rt_server)
  add_executable(${_target} "${_target}.cc"
    ${RT_PROTO_SRCS} ${RT_PROTO_HDRS} ${TRANSPORT_HDRS})
  target_link_libraries(${_target} ${Protobuf_LIBRARIES} ${TRANSPORT_LIBS}
    gflags)
  target_include_directories(${_target} PRIVATE .)
endforeach()
