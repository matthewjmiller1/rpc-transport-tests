/* First, so it stays self-compiling */
#include "null_transport.hpp"

namespace rt {

NullServer::NullServer(std::string address, uint16_t port,
               std::function<void(const Msg&, Msg&)> rcvFn) :
    Server(address, port, rcvFn) {}

void
NullServer::wait()
{
}

}
