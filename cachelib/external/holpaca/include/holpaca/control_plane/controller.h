#pragma once
#include <atomic>
#include <holpaca/config.h>
#include <holpaca/data_plane/stage.h>
#include <memory>
#include <holpaca/config.h>
#include <unordered_map>
#include <holpaca/control_plane/cache_proxy.h>
#include <holpaca/control_algorithm/control_algorithm.h>
#include <holpaca/control_algorithm/marginal_hits.h>
#include <holpaca/control_algorithm/proportional_share.h>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace holpaca::data_plane;
using namespace holpaca::control_algorithm;

namespace holpaca::control_plane {
    class Controller {
      Cache* proxy; 
      std::vector<std::shared_ptr<ControlAlgorithm>> m_control_algorithms;
      std::shared_ptr<spdlog::logger> logger;

      public:
        Controller(std::chrono::milliseconds periodicity, char const* logfile = config::controller_log_file) {
          try {
            logger = spdlog::basic_logger_st("Controller", logfile, true);
          } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Log init failed: " << ex.what() << std::endl;
            abort();
          }
          logger->flush_on(spdlog::level::trace);
          logger->set_level(spdlog::level::debug);
          logger->info("Initialization");
          proxy = new CacheProxy(data_plane::config::stage_address);
          m_control_algorithms.push_back(
            std::make_shared<ProportionalShare>(proxy, periodicity)
          );
        }

        ~Controller() {
          logger->info("Destruction");
          delete proxy;
        }
    };
}