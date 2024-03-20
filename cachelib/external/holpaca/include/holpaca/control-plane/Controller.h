#pragma once
#include <grpcpp/server.h>
#include <holpaca/control-plane/ControlAlgorithm.h>
#include <holpaca/control-plane/ProxyManager.h>
#include <holpaca/data-plane/ControlAPI.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace holpaca {

class Controller {
  std::shared_ptr<spdlog::logger> m_logger;
  std::string m_registrationAddress;
  std::vector<std::shared_ptr<ControlAlgorithm>> m_controlAlgorithms;
  std::vector<std::thread> m_runningControlAlgorithms;
  std::shared_ptr<grpc::Server> m_registrationServer;
  std::shared_ptr<ProxyManager> m_proxyManager;

 public:
  Controller() = default;
  ~Controller();
  Controller& setLogger(std::string& logFile);
  Controller& setRegistrationAddress(std::string& address);
  Controller& addControlAlgorithm(
      std::shared_ptr<ControlAlgorithm> controlAlgorithm,
      std::string& configFile);
  Controller& fromFile(std::string& configFile);
  bool isValid() const;
  void operator()();
};

} // namespace holpaca