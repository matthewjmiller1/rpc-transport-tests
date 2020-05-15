/* First, so it stays self-compiling */
#include "capnproto_transport.hpp"

#include "capnproto_transport.capnp.h"

#include <kj/debug.h>
#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <iostream>

namespace rt {

struct CapnprotoServer::impl final {
    explicit impl(std::string address, uint16_t port);

    std::unique_ptr<capnp::EzRpcServer> _server;

private:
    struct ReqSvcImpl final : public StreamService::RequestCallback::Server {
        kj::Promise<void> sendChunk(SendChunkContext context) override;
        kj::Promise<void> done(DoneContext context) override;

    private:
        static kj::Promise<void> sendReplyStream(
            StreamService::ReplyCallback::Client stream, const rt::Msg &sndMsg,
            size_t bufIdx);

        rt::Msg _rcvMsg;
        rt::Msg _sndMsg;
        rt::MsgDataContainer _reqData;
        rt::MsgDataContainer _rspData;
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
    auto strP = std::make_unique<std::string>(
        reinterpret_cast<const char *>(chunk.begin()), chunk.size());
    _reqData.push_back(std::move(strP));

    DataBuf buf;
    buf._addr = rt::DataBuf::cStrToAddr(_reqData.back()->data());
    buf._len = _reqData.back()->size();
    _rcvMsg._bufs.push_back(std::move(buf));

    return kj::READY_NOW;
}

kj::Promise<void>
CapnprotoServer::impl::ReqSvcImpl::done(DoneContext context)
{
    auto replySvc = context.getParams().getReplySvc();
    auto rcvFn = getRcvFn();

    try {
        rcvFn(_rcvMsg, _sndMsg, _rspData);
    } catch (const std::exception &e) {
        std::cerr << "rcvFn exception: " << e.what() << std::endl;
        std::abort();
    }

    return sendReplyStream(replySvc, _sndMsg, 0);
}

kj::Promise<void>
CapnprotoServer::impl::ReqSvcImpl::sendReplyStream(
    StreamService::ReplyCallback::Client stream, const rt::Msg &sndMsg,
    size_t bufIdx)
{
    if (bufIdx == sndMsg._bufs.size()) {
        return stream.doneRequest().send().ignoreResult();
    }

    const auto &buf = sndMsg._bufs[bufIdx];
    auto chunkReq = stream.sendChunkRequest();
    chunkReq.setChunk(capnp::Data::Reader(buf._addr, buf._len));
    return chunkReq.send().then(
        [stream = kj::mv(stream), bufIdx, &sndMsg]() mutable {
            return sendReplyStream(kj::mv(stream), sndMsg, (bufIdx + 1));
        });
}

CapnprotoServer::impl::impl(std::string address, uint16_t port)
{
    if (address == "::") {
        // capnproto format for wildcard listener
        address = "*";
    }
    const auto serverAddress = address + ":" + std::to_string(port);
    _server = std::make_unique<capnp::EzRpcServer>(
        kj::heap<StreamServiceImpl>(), serverAddress);
    if (nullptr == _server) {
        throw std::runtime_error("server was not created");
    }
}

CapnprotoServer::CapnprotoServer(std::string address, uint16_t port)
    : Server(address, port), _pImpl(std::make_unique<impl>(address, port))
{
}

CapnprotoServer::~CapnprotoServer() = default;

void
CapnprotoServer::wait()
{
    auto &waitScope = _pImpl->_server->getWaitScope();
    kj::NEVER_DONE.wait(waitScope);
}

struct CapnprotoClient::impl final {
    explicit impl(std::string serverAddress, uint16_t serverPort);

    std::unique_ptr<capnp::EzRpcClient> _client;
    std::unique_ptr<StreamService::Client> _svc;

    static kj::Promise<void> sendRequestStream(
        StreamService::RequestCallback::Client stream, const rt::Msg &sndMsg,
        rt::Msg &reply, MsgDataContainer &replyData, size_t bufIdx);

private:
    struct ReplySvcImpl final : public StreamService::ReplyCallback::Server {
        explicit ReplySvcImpl(rt::Msg &reply, MsgDataContainer &replyData)
            : _reply(reply), _replyData(replyData)
        {
        }

        kj::Promise<void> sendChunk(SendChunkContext context) override;
        kj::Promise<void> done(DoneContext context) override;

    private:
        rt::Msg &_reply;
        rt::MsgDataContainer &_replyData;
    };
};

CapnprotoClient::impl::impl(std::string serverAddress, uint16_t serverPort)
{
    const auto serverUri = serverAddress + ":" + std::to_string(serverPort);

    _client = std::make_unique<capnp::EzRpcClient>(serverUri);
    if (nullptr == _client) {
        throw std::runtime_error("client was not created");
    }

    auto svc = _client->getMain<StreamService>();
    _svc = std::make_unique<StreamService::Client>(std::move(svc));
    if (nullptr == _svc) {
        throw std::runtime_error("service was not created");
    }
}

kj::Promise<void>
CapnprotoClient::impl::ReplySvcImpl::sendChunk(SendChunkContext context)
{
    auto chunk = context.getParams().getChunk();
    // This makes a deep copy of the string's data
    auto strP = std::make_unique<std::string>(
        reinterpret_cast<const char *>(chunk.begin()), chunk.size());
    _replyData.push_back(std::move(strP));

    DataBuf buf;
    buf._addr = rt::DataBuf::cStrToAddr(_replyData.back()->data());
    buf._len = _replyData.back()->size();
    _reply._bufs.push_back(std::move(buf));

    return kj::READY_NOW;
}

kj::Promise<void>
CapnprotoClient::impl::ReplySvcImpl::done(DoneContext context)
{
    return kj::READY_NOW;
}

kj::Promise<void>
CapnprotoClient::impl::sendRequestStream(
    StreamService::RequestCallback::Client stream, const rt::Msg &sndMsg,
    rt::Msg &reply, MsgDataContainer &replyData, size_t bufIdx)
{
    if (bufIdx == sndMsg._bufs.size()) {
        auto doneReq = stream.doneRequest();
        doneReq.setReplySvc(kj::heap<ReplySvcImpl>(reply, replyData));
        return doneReq.send().ignoreResult();
    }

    const auto &buf = sndMsg._bufs[bufIdx];
    auto chunkReq = stream.sendChunkRequest();
    chunkReq.setChunk(capnp::Data::Reader(buf._addr, buf._len));
    return chunkReq.send().then([stream = kj::mv(stream), bufIdx, &sndMsg,
                                 &reply, &replyData]() mutable {
        return sendRequestStream(kj::mv(stream), sndMsg, reply, replyData,
                                 (bufIdx + 1));
    });
}

CapnprotoClient::CapnprotoClient(std::string serverAddress, uint16_t serverPort)
    : Client(serverAddress, serverPort),
      _pImpl(std::make_unique<impl>(serverAddress, serverPort))
{
}

CapnprotoClient::~CapnprotoClient() = default;

void
CapnprotoClient::sendReq(const Msg &request, Msg &reply,
                         MsgDataContainer &replyData)
{
    auto &waitScope = _pImpl->_client->getWaitScope();
    auto req = _pImpl->_svc->reqReplyRequest();
    auto reqSvc = req.send().getReqSvc();
    auto promise =
        impl::sendRequestStream(reqSvc, request, reply, replyData, 0);
    promise.wait(waitScope);
}

}  // namespace rt
