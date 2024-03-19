#pragma once
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <holpaca/data-plane/Cache.h>
#include <holpaca/data-plane/ControlAPI.h>
#include <holpaca/data-plane/PerformanceMonitor.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace holpaca {
class Stage {
  std::unique_ptr<spdlog::logger> m_logger;
  std::shared_ptr<PerformanceMonitor> m_performanceMonitor;
  std::unique_ptr<ControlAPI> m_controlAPI;
  std::unique_ptr<Cache> m_cache;

 public:
  Stage() = default;
  Stage& setLogger(const std::string& logFile);
  Stage& setControlInterface();
};
} // namespace holpaca
