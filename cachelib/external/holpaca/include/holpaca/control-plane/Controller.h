#pragma once
#include <grpcpp/server.h>
#include <holpaca/control-plane/ControlAlgorithm.h>
#include <holpaca/control-plane/ControllerConfig.h>
#include <holpaca/control-plane/ProxyManager.h>
#include <spdlog/spdlog.h>
#include <toml++/toml.h>

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace holpaca {

class Controller {
  std::shared_ptr<spdlog::logger> const m_logger;
  std::vector<std::thread> m_controlAlgorithms;
  std::shared_ptr<grpc::Server> m_registrationServer;
  std::thread m_registrationServerThread;
  std::shared_ptr<ProxyManager> m_proxyManager;
  uint64_t m_maxCacheSize{0};

 public:
  Controller(ControllerConfig& config);
  ~Controller();
};

} // namespace holpaca