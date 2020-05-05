cmake_minimum_required(VERSION 3.17)

project(flatbuffers_hello_world)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_program(FBS_CMD NAMES flatc)
message(STATUS "Using flatc command ${FBS_CMD}")

# .fbs file
set(FBS_BASE "greeter")
get_filename_component(FBS_FILE "${FBS_BASE}.fbs" ABSOLUTE)

set(FBS_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${FBS_BASE}.grpc.fb.cc")
set(FBS_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${FBS_BASE}.grpc.fb.h")

message(STATUS "Using directory ${CMAKE_CURRENT_BINARY_DIR}")
add_custom_command(
  OUTPUT "${FBS_SRCS}" "${FBS_HDRS}"
  COMMAND "${FBS_CMD}"
  ARGS --grpc --cpp -o "${CMAKE_CURRENT_BINARY_DIR}" "${FBS_FILE}"
  DEPENDS "${FBS_FILE}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}"
  "/usr/local/grpc_v1.15.1/include")
link_directories("/usr/local/grpc_v1.15.1/lib")

# Build source files
foreach(_target server client)
  add_executable(${_target} "${_target}.cpp" ${FBS_SRCS} ${FBS_HDRS})
  target_link_libraries(${_target} gpr grpc_unsecure grpc++_unsecure)
endforeach()
