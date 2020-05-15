/* First, so it stays self-compiling */
#include "rsocket_transport.hpp"

#include <rsocket/transports/tcp/TcpConnectionAcceptor.h>
#include <rsocket/transports/tcp/TcpConnectionFactory.h>
#include <rsocket/RSocket.h>
#include <yarpl/Flowable.h>

#include <iostream>

#include <glog/logging.h>
#include <log_levels.hpp>

namespace rt {

typedef yarpl::flowable::Flowable<rsocket::Payload> FlowablePayload;

struct RsocketServer::Handler : public rsocket::RSocketResponder {
    std::shared_ptr<FlowablePayload> handleRequestChannel(
        rsocket::Payload initialPayload,
        std::shared_ptr<FlowablePayload> request, rsocket::StreamId) override;
};

std::shared_ptr<FlowablePayload>
RsocketServer::Handler::handleRequestChannel(
    rsocket::Payload initialPayload, std::shared_ptr<FlowablePayload> request,
    rsocket::StreamId)
{
    rt::Msg rcvMsg, sndMsg;
    auto rcvFn = getRcvFn();
    auto cnt = 0;
    rt::MsgDataContainer reqData, rspData;

    VLOG(rt::ll::STRING_MEM) << __func__ << " called";

    auto subscriber = yarpl::flowable::Subscriber<rsocket::Payload>::create(
        [&cnt, &rcvMsg, &reqData](rsocket::Payload p) {
            // TODO: remove
            std::cout << "receiving " << cnt << std::endl;
            ++cnt;
            auto str = p.moveDataToString();
            auto strP = std::make_unique<std::string>(std::move(str));
            reqData.push_back(std::move(strP));

            DataBuf buf;
            buf._addr = rt::DataBuf::cStrToAddr(reqData.back()->data());
            buf._len = reqData.back()->size();

            rcvMsg._bufs.push_back(std::move(buf));
        });

    request->subscribe(subscriber);
// Create the request message by mapping over the stream.
#if 0
    request->map([&cnt, &rcvMsg, &reqData](rsocket::Payload p) {
        std::cout << "receiving " << cnt << std::endl;
        ++cnt;
        auto str = p.moveDataToString();
        auto strP = std::make_unique<std::string>(std::move(str));
        reqData.push_back(std::move(strP));

        DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(reqData.back()->data());
        buf._len = reqData.back()->size();

        rcvMsg._bufs.push_back(std::move(buf));
        return 0;
    });
#endif

    VLOG(rt::ll::STRING_MEM) << "rcvMsg size: " << rcvMsg._bufs.size();
    // TODO: remove
    std::cout << "rcvMsg size: " << rcvMsg._bufs.size() << " cnt: " << cnt
              << std::endl;

    try {
        rcvFn(rcvMsg, sndMsg, rspData);
    } catch (const std::exception &e) {
        std::cerr << "rcvFn exception: " << e.what() << std::endl;
        std::abort();
    }

    return yarpl::flowable::Flowable<>::range(0, sndMsg._bufs.size())
        ->map([&sndMsg](int64_t idx) {
            const auto &buf = sndMsg._bufs[idx];
            // wrapBuffer() is zero copy (vs. copyBuffer())
            return rsocket::Payload(
                folly::IOBuf::wrapBuffer(buf._addr, buf._len));
        });
}

RsocketServer::RsocketServer(std::string address, uint16_t port)
    : Server(address, port)
{
    rsocket::TcpConnectionAcceptor::Options opts;
    opts.address = folly::SocketAddress(address.c_str(), port);

    _server = rsocket::RSocket::createServer(
        std::make_unique<rsocket::TcpConnectionAcceptor>(std::move(opts)));
    if (nullptr == _server) {
        throw std::runtime_error("server was not created");
    }
}

void
RsocketServer::wait()
{
    _server->startAndPark([](const rsocket::SetupParameters &) {
        return std::make_shared<Handler>();
    });
}

RsocketClient::RsocketClient(std::string serverAddress, uint16_t serverPort)
    : Client(serverAddress, serverPort)
{
    folly::SocketAddress address;

    address.setFromHostPort(serverAddress, serverPort);

    _client = rsocket::RSocket::createConnectedClient(
                  std::make_unique<rsocket::TcpConnectionFactory>(
                      *_workerThread.getEventBase(), std::move(address)))
                  .get();
    if (nullptr == _client) {
        throw std::runtime_error("client was not created");
    }
}

void
RsocketClient::sendReq(const Msg &request, Msg &reply,
                       MsgDataContainer &replyData)
{
    auto reqFlow = yarpl::flowable::Flowable<>::range(0, request._bufs.size())
                       ->map([&request](int64_t idx) {
                           const auto &buf = request._bufs[idx];
                           // TODO: remove
                           std::cout << "sending " << idx << std::endl;
                           // wrapBuffer() is zero copy (vs. copyBuffer())
                           return rsocket::Payload(
                               folly::IOBuf::wrapBuffer(buf._addr, buf._len));
                       });

    auto requester = _client->getRequester();
    auto reqChannel = requester->requestChannel(reqFlow);
    reqChannel->subscribe([&reply, &replyData](rsocket::Payload p) {
        auto str = p.moveDataToString();
        auto strP = std::make_unique<std::string>(std::move(str));
        replyData.push_back(std::move(strP));
        DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(strP->data());
        buf._len = strP->size();
        reply._bufs.push_back(std::move(buf));
    });
}

}  // namespace rt
