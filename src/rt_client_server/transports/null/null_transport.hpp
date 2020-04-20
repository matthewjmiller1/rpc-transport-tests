#pragma once

#include <transport.hpp>

namespace rt {

    struct NullServer final : public Server {
        explicit NullServer(std::string address, uint16_t port,
                            std::function<void(const Msg&, Msg&)> rcvFn);

        void wait() override;
    };
}
