# Based on:
# https://chromium.googlesource.com/external/github.com/grpc/grpc/+/chromium-deps/2016-08-17/examples/cpp/helloworld/CMakeLists.txt
cmake_minimum_required(VERSION 3.17)

project(grpc_hello_world)

find_package(Protobuf REQUIRED)

find_package(gRPC REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# gRPC C++ plugin
#get_target_property(GRPC_CPP_PLUGIN_EXECUTABLE gRPC::grpc_cpp_plugin
#  IMPORTED_LOCATION_RELEASE)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE NAMES grpc_cpp_plugin)
message(STATUS "Using gRPC plugin ${GRPC_CPP_PLUGIN_EXECUTABLE}")

# Proto file
set(PROTO_BASE "helloworld")
get_filename_component(PROTO_FILE "${PROTO_BASE}.proto" ABSOLUTE)
get_filename_component(PROTO_PATH "${PROTO_FILE}" PATH)

# Generate sources
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS "${PROTO_FILE}")
set(GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_BASE}.grpc.pb.cc")
set(GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_BASE}.grpc.pb.h")
add_custom_command(
  OUTPUT "${GRPC_SRCS}" "${GRPC_HDRS}"
  COMMAND protobuf::protoc
  ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" -I "${PROTO_PATH}"
    --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}" "${PROTO_FILE}"
  DEPENDS "${PROTO_FILE}")

# Generated include directory
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Build source files
foreach(_target
  greeter_client greeter_server
  greeter_async_client greeter_async_client2 greeter_async_server)
  add_executable(${_target} "${_target}.cc" ${PROTO_SRCS}
    ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})
  target_link_libraries(${_target} ${Protobuf_LIBRARIES}
    gRPC::grpc++ gRPC::grpc++_reflection)
endforeach()
