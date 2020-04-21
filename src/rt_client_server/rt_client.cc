#include <transports/null/null_transport.hpp>
#include <transports/grpc/grpc_transport.hpp>

#include <gflags/gflags.h>

DEFINE_string(address, "localhost", "address to connnect to");
DEFINE_int32(port, 54321, "port to connect to");
DEFINE_string(transport, "null", "transport to use");

int
main(int argc, char **argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::unique_ptr<rt::Client> transport;

    if (FLAGS_transport.find("null") != std::string::npos) {
        transport = std::make_unique<rt::NullClient>(FLAGS_address, FLAGS_port);
    } else if (FLAGS_transport.find("grpc") != std::string::npos) {
        transport = std::make_unique<rt::GrpcClient>(FLAGS_address, FLAGS_port);
    } else {
        std::cerr << "Unknown transport: " << FLAGS_transport << std::endl;
        return 1;
    }

    return 0;
}
