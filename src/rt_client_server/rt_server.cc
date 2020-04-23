#include <transports/null/null_transport.hpp>
#include <transports/grpc/grpc_transport.hpp>
#include "payload_creator.hpp"

#include <memory>
#include <iostream>

#include <gflags/gflags.h>

DEFINE_int32(port, 54321, "port to listen on");
DEFINE_string(transport, "null", "transport to use");

struct WriteRspPayloadCreator final : public PayloadCreator {
    void fill(std::vector<std::string> &msgData, rt::Msg &msg,
              int32_t blockSize, int32_t blockCount) override;
};

struct ReadRspPayloadCreator final : public PayloadCreator {
    void fill(std::vector<std::string> &msgData, rt::Msg &msg,
              int32_t blockSize, int32_t blockCount) override;
};

void
WriteRspPayloadCreator::fill(std::vector<std::string> &msgData,
                          rt::Msg &msg, int32_t blockSize, int32_t blockCount)
{
}

void
ReadRspPayloadCreator::fill(std::vector<std::string> &msgData,
                          rt::Msg &msg, int32_t blockSize, int32_t blockCount)
{
}

static void
ServerRcvFn(const rt::Msg &req, rt::Msg &rsp)
{
    std::unique_ptr<PayloadCreator> payloadCreator;

    if (req._bufs.empty()) {
        throw std::runtime_error("empty request");
    }

    std::string str(reinterpret_cast<char const*>(req._bufs[0]._addr),
                    req._bufs[0]._len);
    rpc_transports::Payload payload;
    payload.ParseFromString(str);

    if (!payload.has_hdr()) {
        throw std::runtime_error("no header");
    }

    const auto hdr = payload.hdr();

    if (hdr.has_w_req_hdr()) {
        payloadCreator = std::make_unique<WriteRspPayloadCreator>();
    } else if (hdr.has_r_req_hdr()) {
        payloadCreator = std::make_unique<ReadRspPayloadCreator>();
    } else {
        throw std::runtime_error("unexpected header type");
    }
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
