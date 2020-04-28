/* First, so it stays self-compiling */
#include "grpc_transport.hpp"

#include <string>

#include <glog/logging.h>
#include <log_levels.hpp>

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
    rt::MsgDataContainer reqData, rspData;

    while (stream->Read(&req)) {
        if (VLOG_IS_ON(rt::ll::STRING_MEM) && (req.data().size() < 100)) {
            VLOG(rt::ll::STRING_MEM) << "Deserialized msg: \"" <<
                req.DebugString();
        }
        auto str = req.release_data();
        reqData.push_back(std::unique_ptr<std::string>(str));
        DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(reqData.back()->data());
        buf._len = reqData.back()->size();

        if (VLOG_IS_ON(rt::ll::STRING_MEM) && (buf._len < 100)) {
            VLOG(rt::ll::STRING_MEM) << "mem[0] " <<
                rt::DataBuf::bytesToHex(buf._addr, 1);
            VLOG(rt::ll::STRING_MEM) << "storing " <<
                static_cast<const void*>(buf._addr) <<
                " " << buf._len;
            VLOG(rt::ll::STRING_MEM) << 
                rt::DataBuf::bytesToHex((uint8_t *) reqData.back()->c_str(),
                                        reqData.back()->size());
        }
        rcvMsg._bufs.push_back(std::move(buf));
    }

    try {
        rcvFn(rcvMsg, sndMsg, rspData);
    } catch (const std::exception& e) {
        std::cerr << "rcvFn exception: " << e.what() << std::endl;
        std::abort();
    }

    VLOG(rt::ll::STRING_MEM) << "rsp size " << sndMsg._bufs.size();

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

GrpcServer::GrpcServer(std::string address, uint16_t port, RcvFn rcvFn) :
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
        throw std::runtime_error("server was not created");
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
        throw std::runtime_error("channel was not created");
    }

    _stub = grpc_transport::ReqReplyService::NewStub(_channel);
    if (nullptr == _stub) {
        throw std::runtime_error("stub was not created");
    }
}

void
GrpcClient::sendReq(const Msg &request, Msg &reply, MsgDataContainer &replyData)
{
    grpc::ClientContext context;
    std::unique_ptr<grpc::ClientReaderWriter<grpc_transport::Msg,
        grpc_transport::Msg>> stream(_stub->ReqReply(&context));

    const auto deadline = std::chrono::system_clock::now() +
        std::chrono::milliseconds(10000);
    context.set_deadline(deadline);

    for (const auto &buf : request._bufs) {
        grpc_transport::Msg msg;

        VLOG(rt::ll::STRING_MEM) << "mem[0] " <<
            rt::DataBuf::bytesToHex(buf._addr, 1);

        // XXX: this makes a deep copy of the data.
        msg.set_data(buf._addr, buf._len);

        if (VLOG_IS_ON(rt::ll::STRING_MEM) && (buf._len < 100)) {
            VLOG(rt::ll::STRING_MEM) << "Serialized msg: \"" <<
                msg.DebugString() << "\"";
            VLOG(rt::ll::STRING_MEM) << 
                rt::DataBuf::bytesToHex((uint8_t *) msg.data().c_str(),
                                        msg.data().size());
        }
        stream->Write(msg);
    }

    stream->WritesDone();

    grpc_transport::Msg msg;
    while (stream->Read(&msg)) {
        auto data = msg.release_data();
        replyData.push_back(std::unique_ptr<std::string>(data));
        DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(data->data());
        buf._len = data->size();
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
