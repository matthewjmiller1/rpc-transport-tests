#pragma once

#include <transport.hpp>

namespace rt {

struct CapnprotoServer final : public Server {
    explicit CapnprotoServer(std::string address, uint16_t port);

    ~CapnprotoServer();

    void wait() override;

private:
    struct impl;
    std::unique_ptr<impl> _pImpl;
};

struct CapnprotoClient final : public Client {
    explicit CapnprotoClient(std::string serverAddress, uint16_t serverPort);
    ~CapnprotoClient();

    void sendReq(const Msg &request, Msg &reply,
                 MsgDataContainer &replyData) override;

private:
    struct impl;
    std::unique_ptr<impl> _pImpl;
};

}  // namespace rt
