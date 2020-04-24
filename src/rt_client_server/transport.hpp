#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>
#include <memory>

namespace rt {

    struct DataBuf final {
        const uint8_t *_addr;
        size_t _len;

        static const uint8_t *cStrToAddr(const char *cStr);
    };

    struct Msg final {
        std::vector<DataBuf> _bufs;
    };

    typedef std::vector<std::unique_ptr<std::string>> MsgDataContainer;
    typedef std::function<void(const Msg& req, Msg& rsp,
                               MsgDataContainer &rspData)> RcvFn;

    struct Server {
        explicit Server(std::string address, uint16_t port, RcvFn rcvFn);

        virtual ~Server() = default;

        virtual void wait() = 0;

    protected:

        const std::string _address;
        const uint16_t _port;
        const RcvFn _rcvFn;
    };

    struct Client {
        explicit Client(std::string serverAddress, uint16_t serverPort);

        virtual ~Client() = default;

        virtual void sendReq(const Msg &request, Msg &reply,
                             MsgDataContainer &replyData) = 0;

    protected:

        const std::string _serverAddress;
        const uint16_t _serverPort;
    };
}
