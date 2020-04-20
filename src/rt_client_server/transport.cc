/* First, so it stays self-compiling */
#include "transport.hpp"

namespace rt {

Server::Server(std::string address, uint16_t port,
               std::function<void(const Msg&, Msg&)> rcvFn) :
    _address(address), _port(port), _rcvFn(rcvFn) {}

}
