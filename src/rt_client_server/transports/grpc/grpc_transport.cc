/* First, so it stays self-compiling */
#include "grpc_transport.hpp"

#include <string>

namespace rt {

grpc::Status
ReqReplyServiceImpl::ReqReply(grpc::ServerContext *context,
    grpc::ServerReaderWriter<grpc_transport::Msg, grpc_transport::Msg> *stream)
{
    return grpc::Status::OK;
}

GrpcServer::GrpcServer(std::string address, uint16_t port,
               std::function<void(const Msg&, Msg&)> rcvFn) :
    Server(address, port, rcvFn)
{
    const auto portStr = std::to_string(port);
    const auto serverAddress = address + ":" + portStr;
    grpc::ServerBuilder builder;

    builder.AddListeningPort(serverAddress,
                             grpc::InsecureServerCredentials());

    builder.RegisterService(&_service);
}

void
GrpcServer::wait()
{
    _server->Wait();
}

}
