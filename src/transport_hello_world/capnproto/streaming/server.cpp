#include "stream.capnp.h"
#include <kj/debug.h>
#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <iostream>
#include <vector>

struct ReqSvcImpl final : public StreamService::RequestCallback::Server {

    kj::Promise<void>
    sendChunk(SendChunkContext context) override
    {
        auto chunk = context.getParams().getChunk();
        std::cout << "Server received \"" << chunk.cStr() << "\"" << std::endl;
        _vec.push_back(std::string(chunk.cStr()));
        return kj::READY_NOW;
    }

    kj::Promise<void>
    done(DoneContext context) override
    {
        std::cout << "Server done called:" << std::endl;
        for (const auto str : _vec) {
            std::cout << "  " << str << std::endl;
        }

        auto replySvc = context.getParams().getReplySvc();

        return sendStrings(replySvc, _vec, 0);
    }

private:

    static kj::Promise<void>
    sendStrings(StreamService::ReplyCallback::Client stream,
                const std::vector<std::string> &vec, uint32_t strNum)
    {
        if (strNum == vec.size()) {
            return stream.doneRequest().send().ignoreResult();
        }

        auto chunkReq = stream.sendChunkRequest();
        chunkReq.setChunk(vec[strNum].c_str());
        return chunkReq.send().then([stream=kj::mv(stream),
                                    strNum, &vec]() mutable {
                return sendStrings(kj::mv(stream), vec, strNum + 1);
            });
    }

    std::vector<std::string> _vec;
};

struct StreamServiceImpl final : public StreamService::Server {

    kj::Promise<void>
    reqReply(ReqReplyContext context) override
    {
        context.getResults().setReqSvc(kj::heap<ReqSvcImpl>());
        return kj::READY_NOW;
    }
};

int
main(int argc, const char* argv[])
{

    capnp::EzRpcServer server(kj::heap<StreamServiceImpl>(), "localhost:54321");

    auto& waitScope = server.getWaitScope();
    uint16_t port = server.getPort().wait(waitScope);
    std::cout << "Listening on port " << port << "..." << std::endl;

    kj::NEVER_DONE.wait(waitScope);
}
