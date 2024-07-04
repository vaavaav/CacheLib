#pragma once
#include <holpaca/Cache.h>
#include <holpaca/data-plane/Stage.grpc.pb.h>
#include <holpaca/data-plane/Stage.pb.h>

using grpc::Channel;
using grpc::ClientContext;

namespace facebook::cachelib {

class PoolCacheProxy : public holpaca::DataInterface {
  std::shared_ptr<grpc::Channel> m_channel;
  std::unique_ptr<holpaca::stage::Stub> m_stage;

 public:
  PoolCacheProxy() = default;
  PoolCacheProxy& connect(const std::string& address);
  bool isConnected() const;
  std::string get(std::string& key) override final;
  bool put(std::string& key, std::string& value) override final;
  bool remove(std::string& key) override final;
};

} // namespace facebook::cachelib