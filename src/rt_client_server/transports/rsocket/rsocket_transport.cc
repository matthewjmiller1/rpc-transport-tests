/* First, so it stays self-compiling */
#include "rsocket_transport.hpp"

#include <rsocket/transports/tcp/TcpConnectionAcceptor.h>
#include <rsocket/RSocket.h>
#include <yarpl/Flowable.h>

#include <iostream>

namespace rt {

typedef yarpl::flowable::Flowable<rsocket::Payload> FlowablePayload;

RcvFn RsocketServer::_globalRcvFn;

struct RsocketServer::Handler : public rsocket::RSocketResponder {
      std::shared_ptr<FlowablePayload>
      handleRequestChannel(rsocket::Payload initialPayload,
                           std::shared_ptr<FlowablePayload> request,
                           rsocket::StreamId) override;
};

std::shared_ptr<FlowablePayload>
RsocketServer::Handler::handleRequestChannel(rsocket::Payload initialPayload,
    std::shared_ptr<FlowablePayload> request, rsocket::StreamId)
{
    rt::Msg rcvMsg, sndMsg;
    auto rcvFn = RsocketServer::getRcvFn();
    rt::MsgDataContainer reqData, rspData;

    // Create that request message by mapping over the stream.
    request->map([&rcvMsg, &reqData](rsocket::Payload p) {
        auto str = p.moveDataToString();
        auto strP = std::make_unique<std::string>(std::move(str));
        reqData.push_back(std::move(strP));

        DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(reqData.back()->data());
        buf._len = reqData.back()->size();

        rcvMsg._bufs.push_back(std::move(buf));
        return 0;
    });

    try {
        rcvFn(rcvMsg, sndMsg, rspData);
    } catch (const std::exception& e) {
        std::cerr << "rcvFn exception: " << e.what() << std::endl;
        std::abort();
    }

    return yarpl::flowable::Flowable<>::range(0, sndMsg._bufs.size())->map(
        [&sndMsg](int64_t idx) {
        const auto &buf = sndMsg._bufs[idx];
        // wrapBuffer() is zero copy (vs. copyBuffer())
        return rsocket::Payload(folly::IOBuf::wrapBuffer(buf._addr, buf._len));
    });
}

RcvFn
RsocketServer::getRcvFn()
{
    return _globalRcvFn;
}

RsocketServer::RsocketServer(std::string address, uint16_t port, RcvFn rcvFn) :
    Server(address, port, rcvFn)
{
    rsocket::TcpConnectionAcceptor::Options opts;
    opts.address = folly::SocketAddress(address.c_str(), port);

    // XXX: make MP safe, if needed.
    _globalRcvFn = rcvFn;

    _server = rsocket::RSocket::createServer(
        std::make_unique<rsocket::TcpConnectionAcceptor>(std::move(opts)));
    if (nullptr == _server) {
        throw std::runtime_error("server was not created");
    }
}

void
RsocketServer::wait()
{
    _server->startAndPark([](const rsocket::SetupParameters&) {
                            return std::make_shared<Handler>();
                          });
}

RsocketClient::RsocketClient(std::string serverAddress, uint16_t serverPort) :
    Client(serverAddress, serverPort)
{
}

void
RsocketClient::sendReq(const Msg &request, Msg &reply,
                       MsgDataContainer &replyData)
{
}

}
