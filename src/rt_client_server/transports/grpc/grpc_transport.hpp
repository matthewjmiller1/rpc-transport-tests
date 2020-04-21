#pragma once

#include <transport.hpp>
//#include "grpc_transport.grpc.pb.h"

namespace rt {

    struct GrpcServer final : public Server {
        explicit GrpcServer(std::string address, uint16_t port,
                            std::function<void(const Msg&, Msg&)> rcvFn);

        void wait() override;

    private:

        //grpc_transport::ServiceImpl _service;
    };
}
