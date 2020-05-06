project("Cap'n Proto Samples" CXX)
cmake_minimum_required(VERSION 3.1)

find_package(CapnProto CONFIG REQUIRED)

# Don't build the rpc sample if find_package() found an installation of Cap'n Proto lite.
if(TARGET CapnProto::capnp-rpc)
  capnp_generate_cpp(calculatorSources calculatorHeaders calculator.capnp)
  add_executable(calculator-client calculator-client.c++ ${calculatorSources})
  add_executable(calculator-server calculator-server.c++ ${calculatorSources})
  target_link_libraries(calculator-client PRIVATE CapnProto::capnp-rpc)
  target_link_libraries(calculator-server PRIVATE CapnProto::capnp-rpc)
  target_include_directories(calculator-client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_include_directories(calculator-server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()
