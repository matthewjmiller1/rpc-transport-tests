#pragma once

#include <rt.pb.h>

struct PayloadCreator {
    virtual ~PayloadCreator() = default;
    virtual void fill(std::vector<std::string> &msgData,
                      rt::Msg &msg, int32_t blockSize, int32_t blockCount) = 0;

    static void
    addToMsg(const rpc_transports::Payload &payload,
             std::vector<std::string> &msgData, rt::Msg &msg)
    {
        std::string str;
        payload.SerializeToString(&str);

        rt::DataBuf buf;
        buf._addr = rt::DataBuf::cStrToAddr(str.data());
        buf._len = str.size();
        msg._bufs.push_back(std::move(buf));

        msgData.push_back(std::move(str));
    }
};
