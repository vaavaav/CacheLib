#pragma once
#include <grpcpp/server.h>

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "cachelib/holpaca/control-plane/ControllerConfig.h"
#include "cachelib/holpaca/control-plane/ProxyManager.h"
#include "cachelib/holpaca/control-plane/algorithms/ControlAlgorithm.h"

namespace facebook {
namespace cachelib {
namespace holpaca {
class Controller {
  std::vector<std::thread> m_controlAlgorithms;
  std::shared_ptr<grpc::Server> m_server;
  std::thread m_serverThread;
  std::shared_ptr<ProxyManager> m_proxyManager;

 public:
  Controller(ControllerConfig config);
  ~Controller();
};
} // namespace holpaca
} // namespace cachelib
} // namespace facebook
