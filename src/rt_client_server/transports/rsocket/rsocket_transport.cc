/* First, so it stays self-compiling */
#include "rsocket_transport.hpp"

namespace rt {

RsocketServer::RsocketServer(std::string address, uint16_t port, RcvFn rcvFn) :
    Server(address, port, rcvFn)
{
}

void
RsocketServer::wait()
{
}

RsocketClient::RsocketClient(std::string serverAddress, uint16_t serverPort) :
    Client(serverAddress, serverPort)
{
}

void
RsocketClient::sendReq(const Msg &request, Msg &reply,
                       MsgDataContainer &replyData)
{
}

}
