#pragma once

#include <rt.pb.h>
#include "log_levels.hpp"
#include "transport.hpp"
#include <glog/logging.h>

struct PayloadCreator {
    virtual ~PayloadCreator() = default;

    virtual void fill(rt::MsgDataContainer &msgData,
                      rt::Msg &msg, int32_t blockSize, int32_t blockCount) = 0;


    static void
    addToMsg(const rpc_transports::Payload &payload,
             rt::MsgDataContainer &msgData, rt::Msg &msg)
    {
        auto str = std::make_unique<std::string>();
        payload.SerializeToString(str.get());

        msgData.push_back(std::move(str));
        rt::DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(msgData.back()->data());
        buf._len = msgData.back()->size();
        msg._bufs.push_back(std::move(buf));

        VLOG(rt::ll::STRING_MEM) << "mem[0] " <<
            static_cast<unsigned
                int>(static_cast<unsigned char>(buf._addr[0]));
        if (VLOG_IS_ON(rt::ll::STRING_MEM) && (buf._len < 100)) {
            VLOG(rt::ll::STRING_MEM) << "Serialized string: \"" <<
                payload.DebugString() << "\"";
            for (char ch : *msgData.back()) {
                VLOG(rt::ll::STRING_MEM) << "0x" << static_cast<unsigned
                    int>(static_cast<unsigned char>(ch));
            }
        }
    }
};
