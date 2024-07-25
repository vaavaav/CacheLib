#include "Stage.h"

#include <grpcpp/create_channel.h>

#include <thread>

#include "../protos/Holpaca.grpc.pb.h"
#include "../protos/Holpaca.pb.h"
#include "Stage.h"

namespace facebook {
namespace cachelib {
namespace holpaca {

Stage::Stage(Cache* const cache,
             std::string address,
             std::string controllerAddress) {
  if (cache == NULL) {
    std::cerr << "Cache cannot be null" << std::endl;
    abort();
  }
  m_cache = cache;
  m_server = grpc::ServerBuilder()
                 .AddListeningPort(address, grpc::InsecureServerCredentials())
                 .RegisterService(this)
                 .BuildAndStart();
  m_address = address;
  m_serverThread = std::thread([this] { m_server->Wait(); });
  m_keepAliveThread = std::thread([this, controllerAddress, address] {
    auto controllerStub = ::holpaca::Controller::NewStub(grpc::CreateChannel(
        controllerAddress, grpc::InsecureChannelCredentials()));
    while (!m_stop) {
      grpc::ClientContext ctx;
      ::holpaca::KeepAliveRequest req;
      ::holpaca::KeepAliveResponse rep;
      req.set_address(m_address);
      controllerStub->KeepAlive(&ctx, req, &rep);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });
}

Stage::~Stage() {
  m_stop.exchange(true);
  m_keepAliveThread.join();
  m_server->Shutdown();
  m_serverThread.join();
}

grpc::Status Stage::GetStatus(grpc::ServerContext* context,
                              const ::holpaca::GetStatusRequest* request,
                              ::holpaca::GetStatusResponse* response) {
  auto pools = response->mutable_pools();
  for (const auto& [pid, pstatus] : m_cache->getStatus()) {
    ::holpaca::GetStatusResponse::PoolStatus s;
    s.set_maxsize(pstatus.maxSize);
    s.set_usedsize(pstatus.usedSize);
    s.mutable_tailaccesses()->insert(pstatus.tailAccesses.begin(),
                                     pstatus.tailAccesses.end());
    s.mutable_mrc()->insert(pstatus.mrc.begin(), pstatus.mrc.end());
    pools->insert({pid, s});
  }

  return grpc::Status::OK;
}
grpc::Status Stage::Resize(grpc::ServerContext* context,
                           const ::holpaca::ResizeRequest* request,
                           ::holpaca::ResizeResponse* response) {
  m_cache->resize(std::unordered_map<int32_t, uint64_t>(
      request->newsizes().begin(), request->newsizes().end()));
  return grpc::Status::OK;
}
} // namespace holpaca
} // namespace cachelib
} // namespace facebook