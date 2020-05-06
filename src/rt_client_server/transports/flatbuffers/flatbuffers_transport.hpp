#pragma once

#include <transport.hpp>
#include <memory>

namespace rt {

/*
 * XXX: use PIMPL to avoid including gRPC headers here which are of a
 * different version than what the grpc_transport uses.
 */

struct FlatbuffersServer final : public Server {
    explicit FlatbuffersServer(std::string address, uint16_t port);

    ~FlatbuffersServer();

    void wait() override;

private:

    struct impl;
    std::unique_ptr<impl> _pImpl;
};

struct FlatbuffersClient final : public Client {
    explicit FlatbuffersClient(std::string serverAddress, uint16_t serverPort);
    ~FlatbuffersClient();

    void sendReq(const Msg &request, Msg &reply,
                 MsgDataContainer &replyData) override;

private:

    struct impl;
    std::unique_ptr<impl> _pImpl;
};

}
