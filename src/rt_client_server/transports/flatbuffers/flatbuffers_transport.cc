/* First, so it stays self-compiling */
#include "flatbuffers_transport.hpp"

#include <flatbuffers_transport.grpc.fb.h>
#include <grpc++/grpc++.h>

namespace rt {

struct FlatbuffersServer::impl final {
    explicit impl(std::string address, uint16_t port);

    std::unique_ptr<grpc::Server> _server;

private:

    struct ReqReplyServiceImpl final :
        public fbs_transport::ReqReplyService::Service {

        grpc::Status ReqReply(grpc::ServerContext *context,
            grpc::ServerReaderWriter<
                flatbuffers::grpc::Message<fbs_transport::Msg>,
                flatbuffers::grpc::Message<fbs_transport::Msg>> *stream)
            override;
    };

    ReqReplyServiceImpl _service;
};

grpc::Status
FlatbuffersServer::impl::ReqReplyServiceImpl::ReqReply(
    grpc::ServerContext *context,
    grpc::ServerReaderWriter<
        flatbuffers::grpc::Message<fbs_transport::Msg>,
        flatbuffers::grpc::Message<fbs_transport::Msg>> *stream)
{
    auto retVal = grpc::Status::OK;
    rt::Msg rcvMsg, sndMsg;
    auto rcvFn = Server::getRcvFn();
    rt::MsgDataContainer reqData, rspData;
    flatbuffers::grpc::MessageBuilder mb;
    flatbuffers::grpc::Message<fbs_transport::Msg> reqMsg;

    while (stream->Read(&reqMsg)) {
        const auto req = reqMsg.GetRoot();
        // This makes a deep copy of the string's data
        auto str = req->data()->str();
        auto strP = std::make_unique<std::string>(std::move(str));
        reqData.push_back(std::move(strP));
        DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(reqData.back()->data());
        buf._len = reqData.back()->size();
        rcvMsg._bufs.push_back(std::move(buf));
    }

    try {
        rcvFn(rcvMsg, sndMsg, rspData);
    } catch (const std::exception& e) {
        std::cerr << "rcvFn exception: " << e.what() << std::endl;
        std::abort();
    }

    for (const auto &buf : sndMsg._bufs) {
        auto data = mb.CreateString(reinterpret_cast<const char *>(buf._addr),
                                    buf._len);
        auto rsp = fbs_transport::CreateMsg(mb, data);
        mb.Finish(rsp);
        stream->Write(mb.ReleaseMessage<fbs_transport::Msg>());
    }

    return retVal;
}

FlatbuffersServer::impl::impl(std::string address, uint16_t port)
{
    // XXX: could refactor to share a lot of common code with GrpcServer
    const auto portStr = std::to_string(port);
    if (address == "::") {
        // grpc format for wildcard listener
        address = "[::]";
    }
    const auto serverAddress = address + ":" + portStr;
    grpc::ServerBuilder builder;

    builder.AddListeningPort(serverAddress,
                             grpc::InsecureServerCredentials());

    builder.RegisterService(&_service);

    _server = builder.BuildAndStart();
    if (nullptr == _server) {
        throw std::runtime_error("server was not created");
    }
}

FlatbuffersServer::FlatbuffersServer(std::string address, uint16_t port) :
    Server(address, port), _pImpl(std::make_unique<impl>(address, port))
{}

FlatbuffersServer::~FlatbuffersServer() = default;

void
FlatbuffersServer::wait()
{
    _pImpl->_server->Wait();
}

struct FlatbuffersClient::impl final {
    explicit impl(std::string serverAddress, uint16_t serverPort);

    std::shared_ptr<fbs_transport::ReqReplyService::Stub> _stub;

private:

    std::shared_ptr<grpc::Channel> _channel;
};

FlatbuffersClient::impl::impl(std::string serverAddress, uint16_t serverPort)
{
    const auto portStr = std::to_string(serverPort);
    const auto serverUri = serverAddress + ":" + portStr;

    _channel = grpc::CreateChannel(serverUri,
                                   grpc::InsecureChannelCredentials());
    if (nullptr == _channel) {
        throw std::runtime_error("channel was not created");
    }

    _stub = fbs_transport::ReqReplyService::NewStub(_channel);
    if (nullptr == _stub) {
        throw std::runtime_error("stub was not created");
    }
}

FlatbuffersClient::FlatbuffersClient(std::string serverAddress,
                                     uint16_t serverPort) :
    Client(serverAddress, serverPort),
    _pImpl(std::make_unique<impl>(serverAddress, serverPort))
{}

FlatbuffersClient::~FlatbuffersClient() = default;

void
FlatbuffersClient::sendReq(const Msg &request, Msg &reply,
                           MsgDataContainer &replyData)
{
    grpc::ClientContext context;
    flatbuffers::grpc::MessageBuilder mb;
    std::unique_ptr<grpc::ClientReaderWriter<
        flatbuffers::grpc::Message<fbs_transport::Msg>,
        flatbuffers::grpc::Message<fbs_transport::Msg>>>
            stream(_pImpl->_stub->ReqReply(&context));

    const auto deadline = std::chrono::system_clock::now() +
        std::chrono::milliseconds(10000);
    context.set_deadline(deadline);

    for (const auto &buf : request._bufs) {
        auto data = mb.CreateString(reinterpret_cast<const char *>(buf._addr),
                                    buf._len);
        auto msg = fbs_transport::CreateMsg(mb, data);
        mb.Finish(msg);
        stream->Write(mb.ReleaseMessage<fbs_transport::Msg>());
    }

    stream->WritesDone();

    flatbuffers::grpc::Message<fbs_transport::Msg> msg;
    while (stream->Read(&msg)) {
        const auto rsp = msg.GetRoot();
        // This makes a deep copy of the string's data
        auto str = rsp->data()->str();
        auto strP = std::make_unique<std::string>(std::move(str));
        replyData.push_back(std::move(strP));

        DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(replyData.back()->data());
        buf._len = replyData.back()->size();
        reply._bufs.push_back(std::move(buf));
    }

    const auto status = stream->Finish();
    if (!status.ok()) {
        throw std::runtime_error("send failed: (" +
                                 std::to_string(status.error_code()) +
                                 ") " + status.error_message());
    }
}

}
