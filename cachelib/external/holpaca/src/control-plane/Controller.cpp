#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <holpaca/control-plane/Controller.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace holpaca {

Controller::Controller(ControllerConfig& config) : m_logger(config.m_logger) {
  m_proxyManager = std::make_shared<ProxyManager>();
  m_logger->info("Proxy manager created");
  m_registrationServer =
      grpc::ServerBuilder()
          .AddListeningPort(config.m_registrationAddress,
                            grpc::InsecureServerCredentials())
          .RegisterService(m_proxyManager.get())
          .BuildAndStart();
  if (m_registrationServer == nullptr) {
    m_logger->error("Registration server could not be created");
    return;
  }
  m_logger->info("Registration service listening on {}",
                 config.m_registrationAddress);
  // start server
  m_registrationServerThread =
      std::thread{[this]() { m_registrationServer->Wait(); }};
}

Controller::~Controller() {
  if (m_registrationServer != nullptr) {
    m_registrationServer->Shutdown();
  }
  m_registrationServerThread.join();
}

} // namespace holpaca