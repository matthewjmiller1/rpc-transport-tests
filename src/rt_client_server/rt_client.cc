#include <transports/null/null_transport.hpp>
#include <transports/grpc/grpc_transport.hpp>
#include "payload_creator.hpp"

#include <memory>
#include <chrono>
#include <cmath>

#include <gflags/gflags.h>
#include <sodium.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/variance.hpp>

namespace ba = boost::accumulators;

typedef ba::accumulator_set<double,
        ba::stats<ba::tag::count, ba::tag::mean, ba::tag::variance> > StatSet;

DEFINE_string(address, "localhost", "address to connnect to");
DEFINE_int32(port, 54321, "port to connect to");
DEFINE_string(transport, "null", "transport to use");
DEFINE_string(workload, "write", "workload type");
DEFINE_int32(block_size, 4096, "block size for workload");
DEFINE_int32(block_count, 2, "block count for workload");
DEFINE_int32(op_count, 1, "op count to execute");

struct WriteReqPayloadCreator final : public PayloadCreator {
    void fill(rt::MsgDataContainer &msgData, rt::Msg &msg,
              int32_t blockSize, int32_t blockCount) override;
};

struct ReadReqPayloadCreator final : public PayloadCreator {
    void fill(rt::MsgDataContainer &msgData, rt::Msg &msg,
              int32_t blockSize, int32_t blockCount) override;
};

void
WriteReqPayloadCreator::fill(rt::MsgDataContainer &msgData,
                             rt::Msg &msg, int32_t blockSize,
                             int32_t blockCount)
{
    // The first buffer is the header
    for (auto i = 0; i < (blockCount + 1); ++i) {
        rpc_transports::Payload payload;

        if (0 == i) {
            auto hdr = payload.mutable_hdr()->mutable_w_req_hdr();
            hdr->set_buf_count(blockCount);
            hdr->set_buf_size(blockSize);
        } else {
            const auto len = blockSize;
            auto buf = std::make_unique<uint8_t[]>(len);
            randombytes_buf(buf.get(), len);
            payload.mutable_data()->set_data(buf.get(), len);
        }

        addToMsg(payload, msgData, msg);
    }
}

void
ReadReqPayloadCreator::fill(rt::MsgDataContainer &msgData,
                            rt::Msg &msg, int32_t blockSize, int32_t blockCount)
{
    rpc_transports::Payload payload;
    auto hdr = payload.mutable_hdr()->mutable_r_req_hdr();

    hdr->set_buf_count(blockCount);
    hdr->set_buf_size(blockSize);

    addToMsg(payload, msgData, msg);
}

static uint64_t
getRemoteProcessTimeUs(const rt::Msg &reply)
{
    uint64_t retVal = 0;

    if (reply._bufs.empty()) {
        throw std::runtime_error("empty reply");
    }

    std::string str(reinterpret_cast<char const*>(reply._bufs[0]._addr),
                    reply._bufs[0]._len);
    rpc_transports::Payload payload;
    payload.ParseFromString(str);

    if (!payload.has_hdr()) {
        throw std::runtime_error("no header");
    }

    const auto hdr = payload.hdr();

    if (hdr.has_w_rsp_hdr()) {
        retVal = hdr.w_rsp_hdr().msg_process_time_us(); 
    } else if (hdr.has_r_rsp_hdr()) {
        retVal = hdr.r_rsp_hdr().msg_process_time_us(); 
    } else {
        throw std::runtime_error("unexpected header type");
    }

    return retVal;
}

void
printStats(const std::string &label, const StatSet &stats)
{
    std::cout << label << ": avg=" << ba::mean(stats) <<
        " dev=" << std::sqrt(ba::variance(stats)) <<
        " count=" << ba::count(stats) <<  std::endl;
}

int
main(int argc, char **argv)
{
    gflags::SetUsageMessage("RPC transport test client");
    google::InitGoogleLogging(argv[0]);
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
        payloadCreator = std::make_unique<WriteReqPayloadCreator>();
    } else if (FLAGS_workload.find("read") != std::string::npos) {
        payloadCreator = std::make_unique<ReadReqPayloadCreator>();
    } else {
        std::cerr << "Unknown workload: " << FLAGS_workload << std::endl;
        return 1;
    }

    std::cout << "Sending " <<  FLAGS_op_count << " " << FLAGS_workload <<
        " op(s), each for " << FLAGS_block_count << " blocks of size " <<
        FLAGS_block_size << " bytes" << std::endl;

    StatSet latAcc;
    StatSet tputAcc;
    for (auto i = 0; i < FLAGS_op_count; ++i) {
        rt::Msg req, reply;
        rt::MsgDataContainer reqData, replyData;
        payloadCreator->fill(reqData, req, FLAGS_block_size, FLAGS_block_count);
        const auto start = std::chrono::steady_clock::now();
        VLOG(rt::ll::STRING_MEM) << "mem[0] " <<
            static_cast<unsigned
                int>(static_cast<unsigned char>(req._bufs[0]._addr[0]));
        transport->sendReq(req, reply, replyData);
        const auto end = std::chrono::steady_clock::now();
        const auto payloadBytes = (FLAGS_block_size * FLAGS_block_count);
        std::chrono::duration<double> delta = (end - start);
        std::chrono::duration<double, std::milli> deltaMs = (end - start);

        VLOG(rt::ll::LATENCY) << "Original deltaMs=" << deltaMs.count() <<
            " delta=" << delta.count();
        const auto remoteTimeUs = getRemoteProcessTimeUs(reply);
        std::chrono::microseconds remoteTime(remoteTimeUs);
        VLOG(rt::ll::LATENCY) << "remoteTimeUsec: " << remoteTime.count();
        deltaMs -= remoteTime;
        delta -= remoteTime;
        VLOG(rt::ll::LATENCY) << "Calculated deltaMs=" << deltaMs.count() <<
            " delta=" << delta.count();

        latAcc(deltaMs.count());
        tputAcc((((8 * payloadBytes)) / delta.count()) / (1024 * 1024));
    }

    printStats("Throughput (Mbps)", tputAcc);
    printStats("Latency (ms)", latAcc);

    return 0;
}
