#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <holpaca/Cache.h>
#include <holpaca/data-plane/Stage.h>
#include <holpaca/data-plane/StageConfig.h>
#include <holpaca/data-plane/StageServer.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <thread>

namespace holpaca {

class Stage::Impl : public Cache {
  std::shared_ptr<grpc::Server> m_server;
  std::shared_ptr<spdlog::logger> m_logger;
  StageServer* m_stageServer;
  Cache* m_cache;
  std::thread m_serverThread;

 public:
  Impl(StageConfig& config) {
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
  };
  ~Impl() {
    if (m_server != nullptr) {
      m_server->Shutdown();
      m_serverThread.join();
    }
    if (m_stageServer != NULL) {
      delete m_stageServer;
    }
    m_logger->info("Stage server shutdown");
  };
};

} // namespace holpaca
