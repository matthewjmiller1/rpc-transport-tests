/* First, so it stays self-compiling */
#include "null_transport.hpp"

namespace rt {

NullServer::NullServer(std::string address, uint16_t port)
    : Server(address, port)
{
}

void
NullServer::wait()
{
}

NullClient::NullClient(std::string serverAddress, uint16_t serverPort)
    : Client(serverAddress, serverPort)
{
}

void
NullClient::sendReq(const Msg &request, Msg &reply, MsgDataContainer &replyData)
{
}

}  // namespace rt
