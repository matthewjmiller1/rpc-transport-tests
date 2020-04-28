cmake_minimum_required(VERSION 3.17)

project(rpc_transport)

find_package(Protobuf REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(gRPC REQUIRED)
find_package(folly REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem regex context
  program_options)
find_package(yarpl REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter)

find_program(GRPC_CPP_PLUGIN_EXECUTABLE NAMES grpc_cpp_plugin)
find_library(DOUBLE-CONVERSION double-conversion)

set(GRPC_LIBS gRPC::grpc++ gRPC::grpc++_reflection)

set(RSOCKET_LIBS
  ReactiveSocket
  folly
  ${Boost_LIBRARIES}
  ${DOUBLE-CONVERSION}
  event
  iberty
  atomic
  ${CMAKE_DL_LIBS}
  yarpl
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  glog::glog
  gflags)

protobuf_generate_cpp(RT_PROTO_SRCS RT_PROTO_HDRS rt.proto)

list(APPEND LOG_HDRS log_levels.hpp)
list(APPEND TRANSPORT_HDRS transport.hpp payload_creator.hpp)

add_library(transport STATIC transport.cc)
target_include_directories(transport PUBLIC .)

set(TRANSPORTS
  null
  grpc
  rsocket)

# Build libraries
foreach(_target ${TRANSPORTS})
  set(TRANSPORT_BASE ${_target}_transport)
  set(TARGET_LIBS)

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

  if ("${_target}" STREQUAL "rsocket")
    list(APPEND TARGET_LIBS ${RSOCKET_LIBS})
  endif()

  add_library(${_target}_transport STATIC
    transports/${_target}/${TRANSPORT_BASE}.cc ${PROTO_SRCS} ${PROTO_HDRS}
    ${GRPC_SRCS} ${GRPC_HDRS} ${LOG_HDRS})
  target_include_directories(${TRANSPORT_BASE} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(${TRANSPORT_BASE} transport ${PROTO_LIBS}
    ${TARGET_LIBS})
  list(APPEND TRANSPORT_LIBS ${TRANSPORT_BASE})
  list(APPEND TRANSPORT_HDRS transports/${_target}/${TRANSPORT_BASE}.hpp)
endforeach()

# Build source files
foreach(_target rt_client rt_server)
  add_executable(${_target} "${_target}.cc"
    ${RT_PROTO_SRCS} ${RT_PROTO_HDRS} ${TRANSPORT_HDRS} ${LOG_HDRS})
  target_link_libraries(${_target} ${Protobuf_LIBRARIES} ${TRANSPORT_LIBS}
    gflags glog::glog -lsodium)
  target_include_directories(${_target} PRIVATE .)
endforeach()
