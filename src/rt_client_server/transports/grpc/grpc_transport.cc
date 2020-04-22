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
    if (address == "::") {
        // grpc format for wildcard listener
        address = "[::]";
    }
    const auto serverAddress = address + ":" + portStr;
    grpc::ServerBuilder builder;

    // XXX: make MP safe, if needed.
    _globalRcvFn = rcvFn;

    builder.AddListeningPort(serverAddress,
                             grpc::InsecureServerCredentials());

    builder.RegisterService(&_service);

    _server = builder.BuildAndStart();
    if (nullptr == _server) {
        throw std::invalid_argument("server was not created");
    }
}

void
GrpcServer::wait()
{
    _server->Wait();
}

GrpcClient::GrpcClient(std::string serverAddress, uint16_t serverPort) :
    Client(serverAddress, serverPort)
{
    const auto portStr = std::to_string(serverPort);
    const auto serverUri = serverAddress + ":" + portStr;

    _channel = grpc::CreateChannel(serverUri,
                                   grpc::InsecureChannelCredentials());
    if (nullptr == _channel) {
        throw std::invalid_argument("channel was not created");
    }

    _stub = grpc_transport::ReqReplyService::NewStub(_channel);
    if (nullptr == _stub) {
        throw std::invalid_argument("stub was not created");
    }
}

void
GrpcClient::sendReq(const Msg &request, Msg &reply)
{
    grpc::ClientContext context;
    std::unique_ptr<grpc::ClientReaderWriter<grpc_transport::Msg,
        grpc_transport::Msg>> stream(_stub->ReqReply(&context));

    const auto deadline = std::chrono::system_clock::now() +
        std::chrono::milliseconds(10000);
    context.set_deadline(deadline);

    for (const auto &buf : request._bufs) {
        grpc_transport::Msg msg;
        // XXX: this makes a deep copy of the data.
        msg.set_data(buf._addr, buf._len);
        stream->Write(msg);
    }

    stream->WritesDone();

    grpc_transport::Msg msg;
    while (stream->Read(&msg)) {
        DataBuf buf;
        auto data = msg.mutable_data();
        buf._addr =
            const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(data->data()));
        buf._len = data->size();
        reply._bufs.push_back(buf);
    }

    const auto status = stream->Finish();
    if (!status.ok()) {
        throw std::runtime_error("send failed: (" +
                                 std::to_string(status.error_code()) +
                                 ") " + status.error_message());
    }
}

}
