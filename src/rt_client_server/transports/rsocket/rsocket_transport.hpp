#pragma once

#include <transport.hpp>
#include <rsocket/RSocketServer.h>
#include <rsocket/RSocketClient.h>
#include <folly/io/async/ScopedEventBaseThread.h>

namespace rt {

struct RsocketServer final : public Server {
    explicit RsocketServer(std::string address, uint16_t port);

    void wait() override;

private:
    struct Handler;

    std::unique_ptr<rsocket::RSocketServer> _server;
};

struct RsocketClient final : public Client {
    explicit RsocketClient(std::string serverAddress, uint16_t serverPort);

    void sendReq(const Msg &request, Msg &reply,
                 MsgDataContainer &replyData) override;

private:
    std::unique_ptr<rsocket::RSocketClient> _client;
    folly::ScopedEventBaseThread _workerThread;
};

}  // namespace rt
