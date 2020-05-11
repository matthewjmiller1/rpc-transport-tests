/* First, so it stays self-compiling */
#include "capnproto_transport.hpp"

#include "capnproto_transport.capnp.h"

#include <kj/debug.h>
#include <capnp/ez-rpc.h>
#include <capnp/message.h>

namespace rt {

struct CapnprotoServer::impl final {
    explicit impl(std::string address, uint16_t port);

    std::unique_ptr<capnp::EzRpcServer> _server;

private:

    struct ReqSvcImpl final : public StreamService::RequestCallback::Server {

        kj::Promise<void> sendChunk(SendChunkContext context) override;
        kj::Promise<void> done(DoneContext context) override;

    private:

        static kj::Promise<void> sendData(
            StreamService::ReplyCallback::Client stream,
            const std::vector<std::string> &vec, uint32_t strNum);

        std::vector<std::string> _vec;
    };

    struct StreamServiceImpl final : public StreamService::Server {
        kj::Promise<void> reqReply(ReqReplyContext context) override;
    };
};

kj::Promise<void>
CapnprotoServer::impl::StreamServiceImpl::reqReply(ReqReplyContext context)
{
    context.getResults().setReqSvc(kj::heap<ReqSvcImpl>());
    return kj::READY_NOW;
}

kj::Promise<void>
CapnprotoServer::impl::ReqSvcImpl::sendChunk(SendChunkContext context)
{
    auto chunk = context.getParams().getChunk();
    // This makes a deep copy of the string's data
    _vec.push_back(std::string(reinterpret_cast<const char *>(chunk.begin()),
                               chunk.size()));
    return kj::READY_NOW;
}

kj::Promise<void>
CapnprotoServer::impl::ReqSvcImpl::done(DoneContext context)
{
    auto replySvc = context.getParams().getReplySvc();
    return sendData(replySvc, _vec, 0);
}

kj::Promise<void>
CapnprotoServer::impl::ReqSvcImpl::sendData(
    StreamService::ReplyCallback::Client stream,
    const std::vector<std::string> &vec, uint32_t strNum)
{
    if (strNum == vec.size()) {
        return stream.doneRequest().send().ignoreResult();
    }

    auto chunkReq = stream.sendChunkRequest();
    // TODO
    //chunkReq.setChunk(capnp::Data::Reader(vec[strNum].c_str()));
    return chunkReq.send().then([stream=kj::mv(stream),
                                strNum, &vec]() mutable {
            return sendData(kj::mv(stream), vec, strNum + 1);
        });
}

CapnprotoServer::impl::impl(std::string address, uint16_t port)
{
    const auto serverAddress = address + ":" + std::to_string(port);
    _server =
        std::make_unique<capnp::EzRpcServer>(kj::heap<StreamServiceImpl>(),
                                             serverAddress);
    if (nullptr == _server) {
        throw std::runtime_error("server was not created");
    }
}

CapnprotoServer::CapnprotoServer(std::string address, uint16_t port) :
    Server(address, port), _pImpl(std::make_unique<impl>(address, port))
{}

CapnprotoServer::~CapnprotoServer() = default;

void
CapnprotoServer::wait()
{
    auto &waitScope = _pImpl->_server->getWaitScope();
    kj::NEVER_DONE.wait(waitScope);
}

struct CapnprotoClient::impl final {
    explicit impl(std::string serverAddress, uint16_t serverPort);

private:

};

CapnprotoClient::impl::impl(std::string serverAddress, uint16_t serverPort)
{
    // TODO: connect to server
}

CapnprotoClient::CapnprotoClient(std::string serverAddress,
                                 uint16_t serverPort) :
    Client(serverAddress, serverPort),
    _pImpl(std::make_unique<impl>(serverAddress, serverPort))
{}

CapnprotoClient::~CapnprotoClient() = default;

void
CapnprotoClient::sendReq(const Msg &request, Msg &reply,
                         MsgDataContainer &replyData)
{
    // TODO: send data
}

}
