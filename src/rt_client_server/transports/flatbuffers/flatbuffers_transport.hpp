#pragma once

#include <transport.hpp>

namespace rt {

struct FlatbuffersServer final : public Server {
    explicit FlatbuffersServer(std::string address, uint16_t port);

    void wait() override;

private:

};

struct FlatbuffersClient final : public Client {
    explicit FlatbuffersClient(std::string serverAddress, uint16_t serverPort);

    void sendReq(const Msg &request, Msg &reply,
                 MsgDataContainer &replyData) override;

private:

};

}

