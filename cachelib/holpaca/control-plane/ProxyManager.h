#pragma once
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "cachelib/holpaca/Cache.h"
#include "cachelib/holpaca/control-plane/CacheProxy.h"
#include "cachelib/holpaca/protos/Holpaca.grpc.pb.h"
#include "cachelib/holpaca/protos/Holpaca.pb.h"

namespace facebook {
namespace cachelib {
namespace holpaca {
class ProxyManager : public ::holpaca::Controller::Service {
  std::shared_timed_mutex m_mutex;
  std::unordered_map<std::string, std::shared_ptr<CacheProxy>> m_proxies;

 public:
  void keepAlive(std::string& address);
  std::unordered_map<std::string, std::shared_ptr<Cache>> getCaches();

  grpc::Status KeepAlive(grpc::ServerContext* context,
                         const ::holpaca::KeepAliveRequest* request,
                         ::holpaca::KeepAliveResponse* response) override;
};
} // namespace holpaca
} // namespace cachelib
} // namespace facebook