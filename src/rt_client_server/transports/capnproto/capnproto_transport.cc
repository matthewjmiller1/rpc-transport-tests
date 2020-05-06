/* First, so it stays self-compiling */
#include "capnproto_transport.hpp"

namespace rt {

struct CapnprotoServer::impl final {
    explicit impl(std::string address, uint16_t port);

private:

};

CapnprotoServer::impl::impl(std::string address, uint16_t port)
{
    // TODO: start listener
}

CapnprotoServer::CapnprotoServer(std::string address, uint16_t port) :
    Server(address, port), _pImpl(std::make_unique<impl>(address, port))
{}

CapnprotoServer::~CapnprotoServer() = default;

void
CapnprotoServer::wait()
{
    // TODO: block for requests
}

struct CapnprotoClient::impl final {
    explicit impl(std::string serverAddress, uint16_t serverPort);

private:

};

CapnprotoClient::impl::impl(std::string serverAddress, uint16_t serverPort)
{
    // TODO: connect to server
}

CapnprotoClient::CapnprotoClient(std::string serverAddress,
                                 uint16_t serverPort) :
    Client(serverAddress, serverPort),
    _pImpl(std::make_unique<impl>(serverAddress, serverPort))
{}

CapnprotoClient::~CapnprotoClient() = default;

void
CapnprotoClient::sendReq(const Msg &request, Msg &reply,
                         MsgDataContainer &replyData)
{
    // TODO: send data
}

}
