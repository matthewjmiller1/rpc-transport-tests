#include <transports/null/null_transport.hpp>
#include <transports/grpc/grpc_transport.hpp>
#include <rt.pb.h>

#include <memory>

#include <gflags/gflags.h>
#include <sodium.h>

DEFINE_string(address, "localhost", "address to connnect to");
DEFINE_int32(port, 54321, "port to connect to");
DEFINE_string(transport, "null", "transport to use");
DEFINE_string(workload, "write", "workload type");
DEFINE_int32(block_size, 4096, "block size for workload");
DEFINE_int32(block_count, 1024, "block count for workload");
DEFINE_int32(op_count, 1, "op count to execute");

struct PayloadCreator {
    virtual ~PayloadCreator() = default;
    virtual void fill(std::vector<std::string> &msgData,
                      rt::Msg &msg, int32_t blockSize, int32_t blockCount) = 0;
};

struct WritePayloadCreator final : public PayloadCreator {
    void fill(std::vector<std::string> &msgData, rt::Msg &msg,
              int32_t blockSize, int32_t blockCount) override;
};

struct ReadPayloadCreator final : public PayloadCreator {
    void fill(std::vector<std::string> &msgData, rt::Msg &msg,
              int32_t blockSize, int32_t blockCount) override;
};

void
WritePayloadCreator::fill(std::vector<std::string> &msgData,
                          rt::Msg &msg, int32_t blockSize, int32_t blockCount)
{
    // The first buffer is the header
    for (auto i = 0; i < (blockCount + 1); ++i) {
        rpc_transports::Payload payload;

        if (0 == i) {
            payload.mutable_hdr()->mutable_w_req_hdr();
        } else {
            const auto len = blockSize;
            auto buf = std::make_unique<uint8_t[]>(len);
            randombytes_buf(buf.get(), len);
            payload.mutable_data()->set_data(buf.get(), len);
        }

        std::string str;
        payload.SerializeToString(&str);

        rt::DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(str.data());
        buf._len = str.size();
        msg._bufs.push_back(std::move(buf));

        msgData.push_back(std::move(str));
    }
}

void
ReadPayloadCreator::fill(std::vector<std::string> &msgData,
                         rt::Msg &msg, int32_t blockSize, int32_t blockCount)
{
}

int
main(int argc, char **argv)
{
    gflags::SetUsageMessage("RPC transport test client");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::unique_ptr<rt::Client> transport;
    std::unique_ptr<PayloadCreator> payloadCreator;

    if (sodium_init() < 0) {
        return 1;
    }

    if (FLAGS_transport.find("null") != std::string::npos) {
        transport = std::make_unique<rt::NullClient>(FLAGS_address, FLAGS_port);
    } else if (FLAGS_transport.find("grpc") != std::string::npos) {
        transport = std::make_unique<rt::GrpcClient>(FLAGS_address, FLAGS_port);
    } else {
        std::cerr << "Unknown transport: " << FLAGS_transport << std::endl;
        return 1;
    }

    if (FLAGS_workload.find("write") != std::string::npos) {
        payloadCreator = std::make_unique<WritePayloadCreator>();
    } else if (FLAGS_workload.find("read") != std::string::npos) {
        payloadCreator = std::make_unique<ReadPayloadCreator>();
    } else {
        std::cerr << "Unknown workload: " << FLAGS_workload << std::endl;
        return 1;
    }

    for (auto i = 0; i < FLAGS_op_count; ++i) {
        rt::Msg req, reply;
        std::vector<std::string> reqData;
        payloadCreator->fill(reqData, req, FLAGS_block_size, FLAGS_block_count);
        transport->sendReq(req, reply);
    }

    return 0;
}
