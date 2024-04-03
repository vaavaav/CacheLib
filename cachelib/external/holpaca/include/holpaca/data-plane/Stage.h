#pragma once
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

#include <memory>
#include <thread>

namespace holpaca {
class Stage {
  std::shared_ptr<grpc::Server> m_server;
  std::shared_ptr<spdlog::logger> m_logger;
  StageServer* m_stageServer;
  Cache* m_cache;
  std::thread m_serverThread;

 public:
  Stage(StageConfig& config);
  ~Stage();
};
} // namespace holpaca
