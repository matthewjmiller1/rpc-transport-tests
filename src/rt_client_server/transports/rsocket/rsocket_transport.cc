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
    auto cnt = 0;
    rt::MsgDataContainer reqData, rspData;

    VLOG(rt::ll::STRING_MEM) << __func__ << " called";

    #if 0
    auto subscriber = yarpl::flowable::Subscriber<rsocket::Payload>::create(
        [&cnt, &rcvMsg, &reqData](rsocket::Payload p) {
            // TODO: remove
            std::cout << "receiving " << cnt << std::endl;
            std::cout << "payload length  " << p.data->length() << std::endl;
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
    auto subscriber = yarpl::flowable::Subscriber<rsocket::Payload>::create(
        [](rsocket::Payload p) {
            // TODO: remove
            std::cout << "rcv payload length  " << p.data->length() <<
                std::endl;
            return 0;
        });

    request->subscribe(subscriber);
    sleep(2);
    #endif
    // Create the request message by mapping over the stream.
    #if 0
    auto foo = request->map([&cnt, &rcvMsg, &reqData](rsocket::Payload p) {
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

    std::cout << "foo: " << foo << std::endl;
    #endif

    VLOG(rt::ll::STRING_MEM) << "rcvMsg size: " << rcvMsg._bufs.size();
    // TODO: remove
    std::cout << "rcvMsg size: " << rcvMsg._bufs.size() << " cnt: " <<
        cnt << std::endl;

    #if 0
    try {
        rcvFn(rcvMsg, sndMsg, rspData);
    } catch (const std::exception& e) {
        std::cerr << "rcvFn exception: " << e.what() << std::endl;
        std::abort();
    }
    #endif

    std::cout << "returning " << std::endl;
    return request->map([](rsocket::Payload p) {
        std::cout << "payload length  " << p.data->length() << std::endl;
        std::cout << "sending " << std::endl;
        return rsocket::Payload("foo");
    });
    #if 0
    return yarpl::flowable::Flowable<>::range(0, sndMsg._bufs.size())->map(
        [&sndMsg](int64_t idx) {
        std::cout << "sending " << idx << std::endl;
        const auto &buf = sndMsg._bufs[idx];
        // wrapBuffer() is zero copy (vs. copyBuffer())
        return rsocket::Payload(folly::IOBuf::wrapBuffer(buf._addr, buf._len));
    });
    #endif
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
    folly::SocketAddress address;

    address.setFromHostPort(serverAddress, serverPort);

    _client =
        rsocket::RSocket::createConnectedClient(
            std::make_unique<rsocket::TcpConnectionFactory>(
            *_workerThread.getEventBase(), std::move(address))).get();
    if (nullptr == _client) {
        throw std::runtime_error("client was not created");
    }
}

void
RsocketClient::sendReq(const Msg &request, Msg &reply,
                       MsgDataContainer &replyData)
{
    auto reqFlow =
        yarpl::flowable::Flowable<>::range(0, 10)->map(
            [](int64_t idx) {
            rsocket::Payload p("foo" + std::to_string(idx));
            // TODO: remove
            std::cout << "sending payload length " << p.data->length() <<
                std::endl;
            return p;
        });
    reqFlow->doOnNext([](const rsocket::Payload &p) {
        std::cout << "complete" << std::endl;
    });
    #if 0
    auto reqFlow =
        yarpl::flowable::Flowable<>::range(0, request._bufs.size())->map(
            [&request](int64_t idx) {
            const auto &buf = request._bufs[idx];
            // wrapBuffer() is zero copy (vs. copyBuffer())
            std::cout << "buf " << buf._len << " " << std::hex << buf._addr <<
                std::endl;
            rsocket::Payload p(folly::IOBuf::wrapBuffer(buf._addr, buf._len));
            // TODO: remove
            std::cout << "sending payload length " << p.data->length() <<
                idx << std::endl;
            return p;
        });
    #endif

    // TODO: remove
    std::cout << "req size " << request._bufs.size() << std::endl;
    std::cout << "requester" << std::endl;
    auto requester = _client->getRequester();
    std::cout << "reqChannel" << std::endl;
    auto reqChannel = requester->requestChannel(reqFlow);
    std::cout << "subscribe" << std::endl;
    #if 0
    reqChannel->subscribe([&reply, &replyData](rsocket::Payload p) {
        std::cout << "received payload" << std::endl;
        auto str = p.moveDataToString();
        auto strP = std::make_unique<std::string>(std::move(str));
        replyData.push_back(std::move(strP));
        DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(strP->data());
        buf._len = strP->size();
        reply._bufs.push_back(std::move(buf));
    });
    #endif
    reqChannel->subscribe([](rsocket::Payload p) {
        std::cout << "received payload len " <<  p.data->length() << std::endl;
    });
    std::cout << "subscribe done" << std::endl;
}

}
