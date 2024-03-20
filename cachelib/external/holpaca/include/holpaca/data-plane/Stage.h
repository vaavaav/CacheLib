#pragma once
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <holpaca/data-plane/PerformanceMonitor.h>
#include <holpaca/data-plane/StageServer.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace holpaca {
class Stage : public DataAPI {
  std::shared_ptr<spdlog::logger> m_logger;
  std::shared_ptr<PerformanceMonitor> m_performanceMonitor;
  std::unique_ptr<grpc::Server> m_server;
  std::shared_ptr<StageServer> m_stageServer;
  Cache* m_cache;
  std::string m_serverAddress;

 public:
  Stage() = default;
  Stage& setLogger(std::string& logFile);
  Stage& setCache(Cache* cache);
  Stage& setServerAddress(std::string& address);
  void operator()();
  ~Stage();
};
} // namespace holpaca
