#pragma once

#include <transport.hpp>
#include <grpcpp/grpcpp.h>
#include <grpc_transport.grpc.pb.h>

namespace rt {

struct ReqReplyServiceImpl final :
    public grpc_transport::ReqReplyService::Service {

    grpc::Status ReqReply(grpc::ServerContext *context,
        grpc::ServerReaderWriter<grpc_transport::Msg,
                                 grpc_transport::Msg> *stream) override;
};

struct GrpcServer final : public Server {
    explicit GrpcServer(std::string address, uint16_t port,
                        std::function<void(const Msg&, Msg&)> rcvFn);

    void wait() override;

    static RcvFn getRcvFn();

private:

    std::unique_ptr<grpc::Server> _server;
    ReqReplyServiceImpl _service;

    static RcvFn _globalRcvFn;
};

struct GrpcClient final : public Client {
    explicit GrpcClient(std::string serverAddress, uint16_t serverPort);

    void sendReq(const Msg &request, Msg &reply) override;

private:

    std::shared_ptr<grpc::Channel> _channel;
    std::shared_ptr<grpc_transport::ReqReplyService::Stub> _stub;
};

}