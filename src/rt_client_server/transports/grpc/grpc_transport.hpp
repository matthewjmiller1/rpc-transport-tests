#pragma once

#include <transport.hpp>

namespace rt {

/*
 * XXX: use PIMPL to avoid including gRPC headers here which are of a
 * different version than what the flatbuffers_transport uses.
 */

struct GrpcServer final : public Server {
    explicit GrpcServer(std::string address, uint16_t port);
    ~GrpcServer();

    void wait() override;

private:
    struct impl;
    std::unique_ptr<impl> _pImpl;
};

struct GrpcClient final : public Client {
    explicit GrpcClient(std::string serverAddress, uint16_t serverPort);
    ~GrpcClient();

    void sendReq(const Msg &request, Msg &reply,
                 MsgDataContainer &replyData) override;

private:
    struct impl;
    std::unique_ptr<impl> _pImpl;
};

}  // namespace rt
