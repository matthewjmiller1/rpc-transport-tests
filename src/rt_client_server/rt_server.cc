#include <transports/null/null_transport.hpp>
#include <transports/grpc/grpc_transport.hpp>

#include <memory>
#include <iostream>

#include <gflags/gflags.h>

DEFINE_int32(port, 54321, "port to listen on");
DEFINE_string(transport, "null", "transport to use");

static void
ServerRcvFn(const rt::Msg &req, rt::Msg &rsp)
{
}

int
main(int argc, char **argv)
{
    const auto addr = "::";

    gflags::SetUsageMessage("RPC transport test server");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::unique_ptr<rt::Server> transport;

    if (FLAGS_transport.find("null") != std::string::npos) {
        transport = std::make_unique<rt::NullServer>(addr, FLAGS_port,
                                                     ServerRcvFn);
    } else if (FLAGS_transport.find("grpc") != std::string::npos) {
        transport = std::make_unique<rt::GrpcServer>(addr, FLAGS_port,
                                                     ServerRcvFn);
    } else {
        std::cerr << "Unknown transport: " << FLAGS_transport << std::endl;
        return 1;
    }

    transport->wait();

    return 0;
}
