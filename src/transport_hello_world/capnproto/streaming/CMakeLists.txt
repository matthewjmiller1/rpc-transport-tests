project("Cap'n Proto Streaming Samples" CXX)
cmake_minimum_required(VERSION 3.1)

find_package(CapnProto CONFIG REQUIRED)
add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter
  -Wno-unused-variable)

if(TARGET CapnProto::capnp-rpc)
  capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS stream.capnp)
  add_executable(client client.cpp ${CAPNP_SRCS})
  add_executable(server server.cpp ${CAPNP_SRCS})
  target_link_libraries(client PRIVATE CapnProto::capnp-rpc)
  target_link_libraries(server PRIVATE CapnProto::capnp-rpc)
  target_include_directories(client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_include_directories(server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()
