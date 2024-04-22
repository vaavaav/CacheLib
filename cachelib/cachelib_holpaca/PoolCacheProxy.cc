#include "PoolCacheProxy.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

namespace facebook::cachelib {
PoolCacheProxy& PoolCacheProxy::connect(const std::string& address) {
  m_channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  m_stage = holpaca::stage::NewStub(m_channel);
  return *this;
}

bool PoolCacheProxy::isConnected() const {
  return m_channel->GetState(true) == GRPC_CHANNEL_READY;
}

std::string PoolCacheProxy::get(std::string& key) {
  holpaca::GetRequest req;
  holpaca::GetResponse rep;
  grpc::ClientContext ctx;

  req.set_key(key);
  m_stage->Get(&ctx, req, &rep);

  return rep.value();
}

bool PoolCacheProxy::put(std::string& key, std::string& value) {
  holpaca::PutRequest req;
  holpaca::PutResponse rep;
  grpc::ClientContext ctx;

  req.set_key(key);
  req.set_value(value);
  m_stage->Put(&ctx, req, &rep);

  return rep.success();
}

bool PoolCacheProxy::remove(std::string& key) {
  holpaca::RemoveRequest req;
  holpaca::RemoveResponse rep;
  grpc::ClientContext ctx;

  req.set_key(key);
  m_stage->Remove(&ctx, req, &rep);

  return rep.success();
}

} // namespace facebook::cachelib