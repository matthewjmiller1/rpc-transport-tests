#pragma once

#include <transport.hpp>

namespace rt {

struct RsocketServer final : public Server {
    explicit RsocketServer(std::string address, uint16_t port, RcvFn rcvFn);

    void wait() override;

private:

};

struct RsocketClient final : public Client {
    explicit RsocketClient(std::string serverAddress, uint16_t serverPort);

    void sendReq(const Msg &request, Msg &reply,
                 MsgDataContainer &replyData) override;

private:

};

}
