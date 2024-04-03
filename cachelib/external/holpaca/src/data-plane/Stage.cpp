#include <holpaca/data-plane/Stage.h>

#include <thread>

namespace holpaca {

Stage::Stage(StageConfig& config) {
  m_logger = spdlog::basic_logger_st("Stage", config.m_logFile);
  if (m_logger == nullptr) {
    std::cerr << "Logger is invalid" << std::endl;
    return;
  }
  m_logger->flush_on(spdlog::level::debug);
  m_logger->set_level(spdlog::level::trace);

  m_cache = config.m_cache;
  m_stageServer = new StageServer(m_cache);
  m_server = grpc::ServerBuilder()
                 .AddListeningPort(config.m_serviceAddress,
                                   grpc::InsecureServerCredentials())
                 .RegisterService(m_stageServer)
                 .BuildAndStart();

  m_logger->info("Stage Server listening on {}", config.m_serviceAddress);
  m_serverThread = std::thread([this] { m_server->Wait(); });
}

Stage::~Stage() {
  if (m_server != nullptr) {
    m_server->Shutdown();
    m_serverThread.join();
  }
  if (m_stageServer != NULL) {
    delete m_stageServer;
  }
  m_logger->info("Stage server shutdown");
}

} // namespace holpaca