/* First, so it stays self-compiling */
#include "flatbuffers_transport.hpp"

namespace rt {

FlatbuffersServer::FlatbuffersServer(std::string address, uint16_t port) :
    Server(address, port) {}

void
FlatbuffersServer::wait()
{
}

FlatbuffersClient::FlatbuffersClient(std::string serverAddress,
                                     uint16_t serverPort) :
    Client(serverAddress, serverPort)
{
}

void
FlatbuffersClient::sendReq(const Msg &request, Msg &reply,
                           MsgDataContainer &replyData)
{
}

}

