/* First, so it stays self-compiling */
#include "rsocket_transport.hpp"

#include <rsocket/transports/tcp/TcpConnectionAcceptor.h>
#include <rsocket/RSocket.h>

namespace rt {

struct RsocketServer::Handler : public rsocket::RSocketResponder {
};

RsocketServer::RsocketServer(std::string address, uint16_t port, RcvFn rcvFn) :
    Server(address, port, rcvFn)
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
