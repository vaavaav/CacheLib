#include "cachelib/holpaca/control-plane/Controller.h"

namespace facebook {
namespace cachelib {
namespace holpaca {
Controller::Controller(ControllerConfig config) {
  m_proxyManager = std::make_shared<ProxyManager>();
  m_server = ::grpc::ServerBuilder()
                 .AddListeningPort(config.m_address,
                                   ::grpc::InsecureServerCredentials())
                 .RegisterService(m_proxyManager.get())
                 .BuildAndStart();
  if (m_server == nullptr) {
    throw std::runtime_error("Failed to start the server");
  }
  m_serverThread = std::thread([this] { m_server->Wait(); });
}

Controller::~Controller() {
  if (m_server != nullptr) {
    m_server->Shutdown();
    m_serverThread.join();
  }
}
} // namespace holpaca
} // namespace cachelib
} // namespace facebook