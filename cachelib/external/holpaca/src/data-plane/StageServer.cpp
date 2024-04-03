#include <holpaca/Cache.h>
#include <holpaca/data-plane/StageServer.h>

namespace holpaca {
StageServer::StageServer(Cache* cache) : m_cache(cache) {}

grpc::Status StageServer::GetStatus(grpc::ServerContext* context,
                                    const StatusRequest* request,
                                    StatusResponse* response) {
  auto status = m_cache->getStatus();
  response->set_maxsize(status.maxSize);
  response->set_usedsize(status.usedSize);
  response->set_hits(status.hits);
  response->set_lookups(status.lookups);
  response->mutable_tailaccesses()->insert(status.tailAccesses.begin(),
                                           status.tailAccesses.end());
  response->mutable_mrc()->insert(status.mrc.begin(), status.mrc.end());

  return grpc::Status::OK;
}
grpc::Status StageServer::Resize(grpc::ServerContext* context,
                                 const ResizeRequest* request,
                                 ResizeResponse* response) {
  m_cache->resize(request->newsize());
  return grpc::Status::OK;
}
} // namespace holpaca