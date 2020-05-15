#pragma once

#include <transport.hpp>

namespace rt {

struct NullServer final : public Server {
    explicit NullServer(std::string address, uint16_t port);

    void wait() override;
};

struct NullClient final : public Client {
    explicit NullClient(std::string serverAddress, uint16_t serverPort);

    void sendReq(const Msg &request, Msg &reply,
                 MsgDataContainer &replyData) override;
};

}  // namespace rt
