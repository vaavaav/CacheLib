#include <grpcpp/server_builder.h>
#include <holpaca/control-plane/ControllerConfig.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace holpaca {

ControllerConfig& ControllerConfig::setLogger(std::string logFile) {
  m_logger = spdlog::basic_logger_st("Controller", logFile, true);
  if (m_logger != nullptr) {
    m_logger->set_level(spdlog::level::trace);
    m_logger->flush_on(spdlog::level::debug);
  }
  return *this;
}

ControllerConfig& ControllerConfig::setRegistrationAddress(
    std::string address) {
  m_registrationAddress = address;
  return *this;
}

ControllerConfig& ControllerConfig::addControlAlgorithm(
    std::string configFile) {
  // todo
  return *this;
}

bool ControllerConfig::isValid() const {
  // check logger
  bool result = true;
  if (m_logger == nullptr) {
    std::cerr << "Logger could not be created" << std::endl;
    result = false;
  }
  if (m_registrationAddress == "") {
    std::cerr << "Registration address is not set" << std::endl;
    result = false;
  }
  // check all control algorithms
  for (auto& [filename, ca] : m_controlAlgorithms) {
    if (ca == nullptr) {
      std::cerr << "Invalid control algorithm configuration file: " << filename
                << std::endl;
      result = false;
    }
  }
  return result;
}
} // namespace holpaca