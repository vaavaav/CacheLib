#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <holpaca/control-plane/Controller.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace holpaca {

Controller& Controller::setLogger(std::string& logFile) {
  try {
    m_logger = spdlog::basic_logger_st("Controller", logFile, true);
    m_logger->set_level(spdlog::level::trace);
    m_logger->flush_on(spdlog::level::debug);
  } catch (const spdlog::spdlog_ex& ex) {
    std::cerr << "Log initialization failed: " << ex.what() << std::endl;
  }
  return *this;
}

Controller& Controller::setRegistrationAddress(std::string& address) {
  m_registrationAddress = address;
  return *this;
}

Controller& Controller::addControlAlgorithm(
    std::shared_ptr<ControlAlgorithm> controlAlgorithm,
    std::string& configFile) {
  controlAlgorithm->fromFile(configFile);
  m_controlAlgorithms.push_back(controlAlgorithm);
  return *this;
}

bool Controller::isValid() const {
  // check logger
  if (m_logger == nullptr) {
    return false;
  }
  // check all control algorithms
  for (auto& ca : m_controlAlgorithms) {
    if (!ca->isValid()) {
      m_logger->error("Invalid control algorithm '{}'", ca->id());
      return false;
    }
  }
  return true;
}

void Controller::operator()() {
  if (!this->isValid()) {
    std::cerr << "Invalid controller" << std::endl;
    return;
  }

  m_proxyManager = std::make_shared<ProxyManager>();

  m_registrationServer =
      grpc::ServerBuilder()
          .AddListeningPort(m_registrationAddress,
                            grpc::InsecureServerCredentials())
          .RegisterService(m_proxyManager.get())
          .BuildAndStart();
  if (m_registrationServer == nullptr) {
    m_logger->error("Registration server could not be created");
    return;
  }
  m_logger->info("Registration service listening on {}", m_registrationAddress);
  // start server
  std::thread{[this]() { m_registrationServer->Wait(); }};
}

Controller::~Controller() {
  if (m_registrationServer != nullptr) {
    m_registrationServer->Shutdown();
  }
}

} // namespace holpaca