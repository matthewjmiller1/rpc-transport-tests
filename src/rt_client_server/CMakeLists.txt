cmake_minimum_required(VERSION 3.17)

project(rpc_transport)

option(ENABLE_FLATBUFFERS "Enable build for flatbuffers" OFF)

if (ENABLE_FLATBUFFERS)
  add_definitions(-DENABLE_FLATBUFFERS)
endif()

find_package(Protobuf REQUIRED)
if (NOT ENABLE_FLATBUFFERS)
  find_package(gRPC REQUIRED)
endif()
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(folly REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem regex context
  program_options)
find_package(yarpl REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter)


if (ENABLE_FLATBUFFERS)
  find_program(FBS_CMD NAMES flatc)
else()
  find_program(GRPC_CPP_PLUGIN_EXECUTABLE NAMES grpc_cpp_plugin)
endif()
find_library(DOUBLE-CONVERSION double-conversion)

if (NOT ENABLE_FLATBUFFERS)
  set(GRPC_LIBS gRPC::grpc++ gRPC::grpc++_reflection)
endif()

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

set(FBS_LIBS gpr grpc_unsecure grpc++_unsecure)

protobuf_generate_cpp(RT_PROTO_SRCS RT_PROTO_HDRS rt.proto)

list(APPEND LOG_HDRS log_levels.hpp)
list(APPEND TRANSPORT_HDRS transport.hpp payload_creator.hpp)

add_library(transport STATIC transport.cc)
target_include_directories(transport PUBLIC .)

set(TRANSPORTS null)
if (NOT ENABLE_FLATBUFFERS)
  list(APPEND TRANSPORTS grpc rsocket)
else()
  list(APPEND TRANSPORTS flatbuffers)
endif()

# Build libraries
foreach(_target ${TRANSPORTS})
  set(TRANSPORT_BASE ${_target}_transport)
  set(TARGET_LIBS)
  set(TARGET_INCLUDE_DIRS)
  set(TARGET_LINK_DIRS)
  set(GEN_SRCS)
  set(GEN_HDRS)
  set(COMPILE_OPTS)

  if ("${_target}" STREQUAL "grpc")
    get_filename_component(PROTO_FILE
      "transports/${_target}/${TRANSPORT_BASE}.proto" ABSOLUTE)
    get_filename_component(PROTO_PATH "${PROTO_FILE}" PATH)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})
    list(APPEND PROTO_LIBS ${Protobuf_LIBRARIES} ${GRPC_LIBS})
    set(GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${TRANSPORT_BASE}.grpc.pb.cc")
    set(GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${TRANSPORT_BASE}.grpc.pb.h")
    list(APPEND GEN_SRCS ${PROTO_SRCS} ${GRPC_SRCS})
    list(APPEND GEN_HDRS ${PROTO_HDRS} ${GRPC_HDRS})
    add_custom_command(
      OUTPUT "${GRPC_SRCS}" "${GRPC_HDRS}"
      COMMAND protobuf::protoc
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" -I "${PROTO_PATH}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}" "${PROTO_FILE}"
      DEPENDS "${PROTO_FILE}")
  endif()

  if ("${_target}" STREQUAL "flatbuffers")
    list(APPEND TARGET_INCLUDE_DIRS "/usr/local/grpc_v1.15.1/include")
    list(APPEND TARGET_LINK_DIRS "/usr/local/grpc_v1.15.1/lib")
    get_filename_component(FBS_FILE
      "transports/${_target}/${TRANSPORT_BASE}.fbs" ABSOLUTE)
    get_filename_component(FBS_PATH "${FBS_FILE}" PATH)
    list(APPEND TARGET_LIBS ${FBS_LIBS})
    set(FBS_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${TRANSPORT_BASE}.grpc.fb.cc")
    set(FBS_HDRS
      "${CMAKE_CURRENT_BINARY_DIR}/${TRANSPORT_BASE}.grpc.fb.h")
    set(FBS_HDRS2"${CMAKE_CURRENT_BINARY_DIR}/${TRANSPORT_BASE}_generated.h")
    list(APPEND COMPILE_OPTS "-Wno-deprecated-declarations")
    list(APPEND GEN_SRCS ${FBS_SRCS})
    list(APPEND GEN_HDRS ${FBS_HDRS} ${FBS_HDRS2})
    add_custom_command(
      OUTPUT "${FBS_SRCS}" "${FBS_HDRS}" "${FBS_HDRS2}"
      COMMAND "${FBS_CMD}"
      ARGS --grpc --cpp -o "${CMAKE_CURRENT_BINARY_DIR}" "${FBS_FILE}"
      DEPENDS "${FBS_FILE}")
  endif()

  if ("${_target}" STREQUAL "rsocket")
    list(APPEND TARGET_LIBS ${RSOCKET_LIBS})
  endif()

  add_library(${_target}_transport STATIC
    transports/${_target}/${TRANSPORT_BASE}.cc ${GEN_SRCS} ${GEN_HDRS}
    ${LOG_HDRS})
  target_compile_options(${TRANSPORT_BASE} PRIVATE ${COMPILE_OPTS})
  target_include_directories(${TRANSPORT_BASE} PRIVATE ${TARGET_INCLUDE_DIRS})
  target_include_directories(${TRANSPORT_BASE} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR})
  target_link_directories(${TRANSPORT_BASE} PUBLIC ${TARGET_LINK_DIRS})
  target_link_libraries(${TRANSPORT_BASE} PRIVATE transport ${PROTO_LIBS}
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
