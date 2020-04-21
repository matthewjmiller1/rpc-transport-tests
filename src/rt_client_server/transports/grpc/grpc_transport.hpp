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

private:

    std::unique_ptr<grpc::Server> _server;
    ReqReplyServiceImpl _service;
};

}
