#include <holpaca/data-plane/Stage.h>
#include <holpaca/data-plane/StageServer.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <thread>

namespace holpaca {
Stage& Stage::setLogger(std::string& logFile) {
  m_logger = spdlog::basic_logger_st("Stage", logFile, true);
  return *this;
}

Stage& Stage::setCache(Cache* cache) {
  m_cache = cache;
  return *this;
}

Stage& Stage::setServerAddress(std::string& address) {
  m_serverAddress = address;
  return *this;
}

void Stage::operator()() {
  if (m_logger == nullptr) {
    std::cerr << "Logger could not be created" << std::endl;
    return;
  }

  m_stageServer = std::make_shared<StageServer>(m_cache);

  m_server =
      grpc::ServerBuilder()
          .AddListeningPort(m_serverAddress, grpc::InsecureServerCredentials())
          .RegisterService(m_stageServer.get())
          .BuildAndStart();
  if (m_server == nullptr) {
    m_logger->error("Server could not be created");
    return;
  }

  m_logger->info("Stage server listening on {}", m_serverAddress);
  // start server
  std::thread{[this]() { m_server->Wait(); }};
}

} // namespace holpaca
