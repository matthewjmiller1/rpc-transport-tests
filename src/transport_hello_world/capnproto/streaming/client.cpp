#include "stream.capnp.h"
#include <capnp/ez-rpc.h>
#include <kj/debug.h>
#include <iostream>

static kj::Promise<void>
sendStrings(StreamService::RequestCallback::Client stream, uint32_t strNum,
            uint32_t stopNum)
{
    if (strNum == stopNum) {
        return stream.doneRequest().send().ignoreResult();
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
    const auto doIterative = false;

    capnp::EzRpcClient client("localhost:54321");
    StreamService::Client svc = client.getMain<StreamService>();

    auto& waitScope = client.getWaitScope();

    auto req = svc.reqReplyRequest();
    auto reqSvc = req.send().getReqSvc();

    if (doIterative) {
        for (auto i = 1; i < 11; ++i) {
            auto chunkReq = reqSvc.sendChunkRequest();
            std::string str("Test " + std::to_string(i));
            chunkReq.setChunk(str.c_str());
            auto promise = chunkReq.send();
            promise.wait(waitScope);
        }

        auto doneReq = reqSvc.doneRequest();
        auto promise = doneReq.send();
        promise.wait(waitScope);
    } else {
        auto promise = sendStrings(reqSvc, 1, 11);
        promise.wait(waitScope);
    }

    return 0;
}
