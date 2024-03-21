#pragma once
#include <holpaca/Cache.h>
#include <holpaca/control-plane/StageProxy.h>
#include <holpaca/protos/holpaca.grpc.pb.h>
#include <holpaca/protos/holpaca.pb.h>

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace holpaca {
class ProxyManager : public holpaca::registration::Service {
  std::shared_timed_mutex m_mutex;
  std::unordered_map<std::string, std::shared_ptr<StageProxy>> m_proxies;

 public:
  ProxyManager() = default;
  bool add(std::string& address);
  void remove(std::string& address);
  std::unordered_map<std::string, std::shared_ptr<Cache>> getCaches();

  // Registration Service
  grpc::Status connect(grpc::ServerContext* context,
                       const ConnectRequest* request,
                       ConnectResponse* response) override final;
  grpc::Status disconnect(grpc::ServerContext* context,
                          const DisconnectRequest* request,
                          DisconnectResponse* response) override final;
};
} // namespace holpaca
