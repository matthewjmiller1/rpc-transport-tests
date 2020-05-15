#include <transports/null/null_transport.hpp>
#ifdef ENABLE_FLATBUFFERS
#include <transports/flatbuffers/flatbuffers_transport.hpp>
#else /* !ENABLE_FLATBUFFERS */
#include <transports/grpc/grpc_transport.hpp>
#include <transports/rsocket/rsocket_transport.hpp>
#include <transports/capnproto/capnproto_transport.hpp>
#endif /* ENABLE_FLATBUFFERS */
#include "payload_creator.hpp"

#include <memory>
#include <iostream>
#include <chrono>
#include <csignal>

#include <gflags/gflags.h>
#include <sodium.h>

DEFINE_int32(port, 54321, "port to listen on");
DEFINE_string(transport, "null", "transport to use");

struct WriteRspPayloadCreator final : public PayloadCreator {
    void fill(rt::MsgDataContainer &msgData, rt::Msg &msg, int32_t blockSize,
              int32_t blockCount) const override;
};

struct ReadRspPayloadCreator final : public PayloadCreator {
    void fill(rt::MsgDataContainer &msgData, rt::Msg &msg, int32_t blockSize,
              int32_t blockCount) const override;
};

struct EchoRspPayloadCreator final : public PayloadCreator {
    void
    fill(rt::MsgDataContainer &msgData, rt::Msg &msg, int32_t blockSize,
         int32_t blockCount) const override
    {
    }
    void fill(const rt::Msg &origMsg, rt::MsgDataContainer &msgData,
              rt::Msg &msg, int32_t blockSize,
              int32_t blockCount) const override;
};

void
WriteRspPayloadCreator::fill(rt::MsgDataContainer &msgData, rt::Msg &msg,
                             int32_t blockSize, int32_t blockCount) const
{
    rpc_transports::Payload payload;
    payload.mutable_hdr()->mutable_w_rsp_hdr();
    addToMsg(payload, msgData, msg);
}

void
ReadRspPayloadCreator::fill(rt::MsgDataContainer &msgData, rt::Msg &msg,
                            int32_t blockSize, int32_t blockCount) const
{
    std::vector<std::unique_ptr<uint8_t[]>> randData;
    const auto bufLen = blockSize;

    // Generate the random data and count that as the processing time.
    const auto start = std::chrono::steady_clock::now();
    for (auto i = 0; i < blockCount; ++i) {
        auto buf = std::make_unique<uint8_t[]>(bufLen);
        randombytes_buf(buf.get(), bufLen);
        randData.push_back(std::move(buf));
    }
    const auto end = std::chrono::steady_clock::now();
    const auto deltaUs =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // The first buffer is the header
    for (auto i = 0; i < (blockCount + 1); ++i) {
        rpc_transports::Payload payload;

        if (0 == i) {
            auto hdr = payload.mutable_hdr()->mutable_r_rsp_hdr();
            hdr->set_msg_process_time_us(deltaUs.count());
        } else {
            payload.mutable_data()->set_data(randData[i - 1].get(), bufLen);
        }

        addToMsg(payload, msgData, msg);
    }
}

void
EchoRspPayloadCreator::fill(const rt::Msg &origMsg,
                            rt::MsgDataContainer &msgData, rt::Msg &msg,
                            int32_t blockSize, int32_t blockCount) const
{
    std::vector<rpc_transports::Payload> echoData;

    // Deserialize the echo data and count that as the processing time.
    const auto start = std::chrono::steady_clock::now();
    for (const auto &buf : origMsg._bufs) {
        rpc_transports::Payload payload;
        std::string str(reinterpret_cast<char const *>(buf._addr), buf._len);
        payload.ParseFromString(str);
        echoData.push_back(std::move(payload));
    }
    const auto end = std::chrono::steady_clock::now();
    const auto deltaUs =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // The first buffer is the header
    for (auto i = 0UL; i < (echoData.size()); ++i) {
        rpc_transports::Payload payload;

        if (0 == i) {
            auto hdr = payload.mutable_hdr()->mutable_e_rsp_hdr();
            hdr->set_msg_process_time_us(deltaUs.count());
        } else {
            payload = std::move(echoData[i]);
        }

        addToMsg(payload, msgData, msg);

        if (VLOG_IS_ON(rt::ll::STRING_MEM) && (i == 1)) {
            const auto byteLen = std::min(100UL, origMsg._bufs[i]._len);
            VLOG(rt::ll::STRING_MEM)
                << "idx " << i << " req mem "
                << rt::DataBuf::bytesToHex(origMsg._bufs[i]._addr, byteLen)
                << " reply mem "
                << rt::DataBuf::bytesToHex(msg._bufs[i]._addr, byteLen);
        }
    }
}

static void
ServerRcvFn(const rt::Msg &req, rt::Msg &rsp, rt::MsgDataContainer &rspData)
{
    VLOG(rt::ll::STRING_MEM) << __func__ << " called";
    std::unique_ptr<PayloadCreator> payloadCreator;

    if (req._bufs.empty()) {
        throw std::runtime_error("empty request");
    }

    VLOG(rt::ll::STRING_MEM)
        << "parsing " << static_cast<const void *>(req._bufs[0]._addr) << " "
        << req._bufs[0]._len;
    VLOG(rt::ll::STRING_MEM)
        << "mem[0] " << rt::DataBuf::bytesToHex(req._bufs[0]._addr, 1);

    std::string str(reinterpret_cast<char const *>(req._bufs[0]._addr),
                    req._bufs[0]._len);
    rpc_transports::Payload payload;

    if (VLOG_IS_ON(rt::ll::STRING_MEM) && (str.size() < 100)) {
        VLOG(rt::ll::STRING_MEM)
            << rt::DataBuf::bytesToHex((uint8_t *)str.c_str(), str.size());
    }

    payload.ParseFromString(str);

    VLOG(rt::ll::STRING_MEM)
        << "Parsed string: \"" << payload.DebugString() << "\"";

    if (!payload.has_hdr()) {
        throw std::runtime_error("no header");
    }

    const auto hdr = payload.hdr();
    auto blockSize = 0;
    auto blockCount = 0;

    if (hdr.has_w_req_hdr()) {
        payloadCreator = std::make_unique<WriteRspPayloadCreator>();
        auto wHdr = hdr.w_req_hdr();
        blockSize = wHdr.buf_size();
        blockCount = wHdr.buf_count();
    } else if (hdr.has_r_req_hdr()) {
        payloadCreator = std::make_unique<ReadRspPayloadCreator>();
        auto rHdr = hdr.r_req_hdr();
        blockSize = rHdr.buf_size();
        blockCount = rHdr.buf_count();
    } else if (hdr.has_e_req_hdr()) {
        payloadCreator = std::make_unique<EchoRspPayloadCreator>();
        auto eHdr = hdr.e_req_hdr();
        blockSize = eHdr.buf_size();
        blockCount = eHdr.buf_count();
    } else {
        throw std::runtime_error("unexpected header type");
    }

    payloadCreator->fill(req, rspData, rsp, blockSize, blockCount);
}

static void
sigIntFn(int s)
{
    google::FlushLogFiles(google::INFO);
    exit(1);
}

int
main(int argc, char **argv)
{
    const auto addr = "::";

    gflags::SetUsageMessage("RPC transport test server");
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigIntFn;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);

    std::unique_ptr<rt::Server> transport;

    if (sodium_init() < 0) {
        return 1;
    }

    rt::Server::setRcvFn(ServerRcvFn);

    if (FLAGS_transport.find("null") != std::string::npos) {
        transport = std::make_unique<rt::NullServer>(addr, FLAGS_port);
#ifdef ENABLE_FLATBUFFERS
    } else if (FLAGS_transport.find("flatbuffers") != std::string::npos) {
        transport = std::make_unique<rt::FlatbuffersServer>(addr, FLAGS_port);
#else /* !ENABLE_FLATBUFFERS */
    } else if (FLAGS_transport.find("grpc") != std::string::npos) {
        transport = std::make_unique<rt::GrpcServer>(addr, FLAGS_port);
    } else if (FLAGS_transport.find("rsocket") != std::string::npos) {
        transport = std::make_unique<rt::RsocketServer>(addr, FLAGS_port);
    } else if (FLAGS_transport.find("capnproto") != std::string::npos) {
        transport = std::make_unique<rt::CapnprotoServer>(addr, FLAGS_port);
#endif /* ENABLE_FLATBUFFERS */
    } else {
        std::cerr << "Unknown transport: " << FLAGS_transport << std::endl;
        return 1;
    }

    transport->wait();

    return 0;
}
