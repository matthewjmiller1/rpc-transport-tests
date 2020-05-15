/* First, so it stays self-compiling */
#include "transport.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>

namespace rt {

RcvFn Server::_rcvFn = nullptr;

const uint8_t *
DataBuf::cStrToAddr(const char *cStr)
{
    return reinterpret_cast<const uint8_t *>(cStr);
}

std::string
DataBuf::bytesToHex(const uint8_t *buf, size_t len)
{
    std::stringstream ss;

    ss << "0x" << std::hex << std::setfill('0');

    for (auto i = 0U; i < len; ++i) {
        ss << std::hex << std::setw(2) << static_cast<int>(buf[i]);
    }

    return ss.str();
}

Server::Server(std::string address, uint16_t port)
    : _address(address), _port(port)
{
}

void
Server::setRcvFn(RcvFn fn)
{
    // XXX: make MP safe, if needed.
    _rcvFn = fn;
}

RcvFn
Server::getRcvFn()
{
    return _rcvFn;
}

Client::Client(std::string serverAddress, uint16_t serverPort)
    : _serverAddress(serverAddress), _serverPort(serverPort)
{
}

}  // namespace rt
