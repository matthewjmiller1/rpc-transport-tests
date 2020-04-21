/* First, so it stays self-compiling */
#include "grpc_transport.hpp"

namespace rt {

GrpcServer::GrpcServer(std::string address, uint16_t port,
               std::function<void(const Msg&, Msg&)> rcvFn) :
    Server(address, port, rcvFn) {}

void
GrpcServer::wait()
{
}

}
