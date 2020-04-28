#pragma once

#include <rt.pb.h>
#include "log_levels.hpp"
#include "transport.hpp"
#include <glog/logging.h>

struct PayloadCreator {
    virtual ~PayloadCreator() = default;

    virtual void fill(rt::MsgDataContainer &msgData,
                      rt::Msg &msg, int32_t blockSize,
                      int32_t blockCount) const = 0;

    virtual void
    fill(const rt::Msg &origMsg, rt::MsgDataContainer &msgData,
         rt::Msg &msg, int32_t blockSize, int32_t blockCount) const
    {
        fill(msgData, msg, blockSize, blockCount);
    }

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

        if (VLOG_IS_ON(rt::ll::STRING_MEM)) {
            const auto byteLen = std::min(100UL, msg._bufs.back()._len);
            VLOG(rt::ll::STRING_MEM) << "mem[0] " <<
                rt::DataBuf::bytesToHex(msg._bufs.back()._addr, byteLen);
            if (msg._bufs.back()._len < 100) {
                VLOG(rt::ll::STRING_MEM) << "Serialized string: \"" <<
                    payload.DebugString() << "\"";
                VLOG(rt::ll::STRING_MEM) << 
                    rt::DataBuf::bytesToHex((uint8_t *) msgData.back()->c_str(),
                                            msgData.back()->size());
            }
        }
    }
};
