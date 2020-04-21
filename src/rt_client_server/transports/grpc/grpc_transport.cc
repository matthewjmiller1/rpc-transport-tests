/* First, so it stays self-compiling */
#include "grpc_transport.hpp"

#include <string>

namespace rt {

RcvFn GrpcServer::_globalRcvFn;

grpc::Status
ReqReplyServiceImpl::ReqReply(grpc::ServerContext *context,
    grpc::ServerReaderWriter<grpc_transport::Msg, grpc_transport::Msg> *stream)
{
    auto retVal = grpc::Status::OK;
    grpc_transport::Msg req;
    rt::Msg rcvMsg, sndMsg;
    auto rcvFn = GrpcServer::getRcvFn();

    while (stream->Read(&req)) {
        DataBuf buf;
        auto data = req.mutable_data();
        buf._addr =
            const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(data->data()));
        buf._len = data->size();
        rcvMsg._bufs.push_back(buf);
    }

    rcvFn(rcvMsg, sndMsg);

    for (const auto &buf : sndMsg._bufs) {
        grpc_transport::Msg rsp;
        rsp.set_data(buf._addr, buf._len);
        stream->Write(rsp);
    }

    return retVal;
}

RcvFn
GrpcServer::getRcvFn()
{
    return _globalRcvFn;
}

GrpcServer::GrpcServer(std::string address, uint16_t port,
                       std::function<void(const Msg&, Msg&)> rcvFn) :
    Server(address, port, rcvFn)
{
    const auto portStr = std::to_string(port);
    const auto serverAddress = address + ":" + portStr;
    grpc::ServerBuilder builder;

    // XXX: make MP safe, if needed.
    _globalRcvFn = rcvFn;

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
