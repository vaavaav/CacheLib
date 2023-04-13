#pragma once
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <holpaca/config.h>
#include <holpaca/data_plane/cache_server.h>
#include <holpaca/data_plane/stage.h>
#include <holpaca/protos/holpaca.grpc.pb.h>
#include <holpaca/protos/holpaca.pb.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <thread>

using grpc::Server;
using grpc::ServerBuilder;
using holpaca::common::Cache;

namespace holpaca::data_plane {
class StageServer : public Stage {
 public:
  StageServer(CacheServer* cache,
              std::chrono::milliseconds periodicity,
              char const* logfile = config::stage_log_file)
      : Stage(cache) {
    try {
      logger = spdlog::basic_logger_mt("StageServer", logfile, true);
    } catch (const spdlog::spdlog_ex& ex) {
      std::cerr << "Log init failed: " << ex.what() << std::endl;
    }
    logger->flush_on(spdlog::level::debug);
    logger->set_level(spdlog::level::trace);
    logger->info("Building Stage Server");

    ServerBuilder builder;

    builder.AddListeningPort(config::stage_address,
                             grpc::InsecureServerCredentials());
    builder.RegisterService(cache);

    server = std::unique_ptr<Server>(builder.BuildAndStart());
    server_thread = std::thread{[this]() { server->Wait(); }};
    logger->info("Stage Server initialized");
  }

  ~StageServer() {
    server->Shutdown();
    server_thread.join();
    logger->info("Stage Server destroyed");
  }

 private:
  std::unique_ptr<Server> server;
  std::thread server_thread;
  std::shared_ptr<spdlog::logger> logger;
};
} // namespace holpaca::data_plane
