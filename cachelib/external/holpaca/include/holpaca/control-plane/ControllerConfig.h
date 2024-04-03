#pragma once
#include <grpcpp/server.h>
#include <holpaca/control-plane/ControlAlgorithm.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace holpaca {
class ControllerConfig {
  std::shared_ptr<std::string> m_logFile;
  std::shared_ptr<spdlog::logger> m_logger;
  std::string m_registrationAddress;
  std::unordered_map<std::string, std::shared_ptr<ControlAlgorithm>>
      m_controlAlgorithms;
  uint64_t m_maxCacheSize{0};

 public:
  ControllerConfig& setLogger(std::string logFile);
  ControllerConfig& setRegistrationAddress(std::string address);
  ControllerConfig& addControlAlgorithm(std::string configFile);
  ControllerConfig& setMaxCacheSize(uint64_t size);
  ControllerConfig& fromFile(std::string& configFile);
  bool isValid() const;

  friend class Controller;
};
} // namespace holpaca