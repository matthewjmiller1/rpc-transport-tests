#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>

namespace rt {

    struct DataBuf final {
        uint8_t *_addr;
        size_t _len;
    };

    struct Msg final {
        std::vector<DataBuf> _bufs;
    };

    struct Server {
        explicit Server(std::string address, uint16_t port,
                        std::function<void(const Msg&, Msg&)> rcvFn);

        virtual ~Server() = default;

        virtual void wait() = 0;

    protected:

        const std::string _address;
        const uint16_t _port;
        const std::function<void(const Msg&, Msg&)> _rcvFn;
    };
}
