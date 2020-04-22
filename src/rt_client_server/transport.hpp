#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>

namespace rt {

    struct DataBuf final {
        const uint8_t *_addr;
        size_t _len;

        static const uint8_t *cStrToAddr(const char *cStr);
    };

    struct Msg final {
        std::vector<DataBuf> _bufs;
    };

    typedef std::function<void(const Msg&, Msg&)> RcvFn;

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

        virtual void sendReq(const Msg &request, Msg &reply) = 0;

    protected:

        const std::string _serverAddress;
        const uint16_t _serverPort;
    };
}
