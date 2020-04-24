/* First, so it stays self-compiling */
#include "transport.hpp"

namespace rt {

const uint8_t *
DataBuf::cStrToAddr(const char *cStr)
{
    //return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(cStr));
    return reinterpret_cast<const uint8_t *>(cStr);
}

Server::Server(std::string address, uint16_t port, RcvFn rcvFn) :
    _address(address), _port(port), _rcvFn(rcvFn) {}

Client::Client(std::string serverAddress, uint16_t serverPort) :
    _serverAddress(serverAddress), _serverPort(serverPort) {}

}
