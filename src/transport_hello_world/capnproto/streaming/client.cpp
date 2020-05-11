#include "stream.capnp.h"
#include <capnp/ez-rpc.h>
#include <kj/debug.h>
#include <iostream>
#include <vector>

struct ReplySvcImpl final : public StreamService::ReplyCallback::Server {

    kj::Promise<void>
    sendChunk(SendChunkContext context) override
    {
        auto chunk = context.getParams().getChunk();
        std::cout << "Client received \"" << chunk.cStr() << "\"" << std::endl;
        _vec.push_back(std::string(chunk.cStr()));
        return kj::READY_NOW;
    }

    kj::Promise<void>
    done(DoneContext context) override
    {
        std::cout << "Client done called:" << std::endl;
        for (const auto str : _vec) {
            std::cout << "  " << str << std::endl;
        }
        return kj::READY_NOW;
    }

private:

    std::vector<std::string> _vec;
};

static kj::Promise<void>
sendStrings(StreamService::RequestCallback::Client stream, uint32_t strNum,
            uint32_t stopNum)
{
    if (strNum == stopNum) {
        auto doneReq = stream.doneRequest();
        doneReq.setReplySvc(kj::heap<ReplySvcImpl>());
        return doneReq.send().ignoreResult();
    }

    std::string str("Test " + std::to_string(strNum));
    auto chunkReq = stream.sendChunkRequest();
    chunkReq.setChunk(str.c_str());
    return chunkReq.send().then([stream=kj::mv(stream),
                                strNum, stopNum]() mutable {
            return sendStrings(kj::mv(stream), strNum + 1, stopNum);
        });
}

int
main(int argc, const char* argv[])
{
    capnp::EzRpcClient client("localhost:54321");
    StreamService::Client svc = client.getMain<StreamService>();

    auto& waitScope = client.getWaitScope();

    auto req = svc.reqReplyRequest();
    auto reqSvc = req.send().getReqSvc();
    auto promise = sendStrings(reqSvc, 1, 11);
    promise.wait(waitScope);

    return 0;
}
