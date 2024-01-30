#pragma once
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <holpaca/config.h>
#include <holpaca/control_algorithm/control_algorithm.h>
#include <holpaca/control_algorithm/hit_ratio_maximization.h>
#include <holpaca/control_algorithm/marginal_hits.h>
#include <holpaca/control_algorithm/miss_rate_min.h>
#include <holpaca/control_algorithm/miss_rate_min_qos.h>
#include <holpaca/control_algorithm/proportional_share.h>
#include <holpaca/control_plane/cache_proxy.h>
#include <holpaca/data_plane/stage.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <memory>
#include <unordered_map>

using namespace holpaca::data_plane;
using namespace holpaca::control_algorithm;

namespace holpaca::control_plane {
class Controller {
  Cache* proxy;
  std::vector<std::thread> m_control_algorithms;
  std::atomic_bool m_continue{true};
  std::shared_ptr<spdlog::logger> logger;

 public:
  Controller(std::chrono::milliseconds const periodicity,
             char const* logfile = config::controller_log_file,
             char** args = NULL) {
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
    m_continue = true;
    m_control_algorithms.push_back(
        // std::make_shared<ProportionalShare>(proxy, periodicity)
        std::thread{[this, args, periodicity]() {
          std::shared_ptr<ControlAlgorithm> ca;
          if (std::string(args[0]) == "hit_ratio_maximization") {
            ca = std::make_shared<HitRatioMaximization>(this->proxy);
          } else if (std::string(args[0]) ==
                     "hit_ratio_maximization_with_qos") {
            ca = std::make_shared<MissRateMinQOS>(
                this->proxy,
                std::unordered_map<pid_t, double>{
                    {strtol(args[1], NULL, 10), strtol(args[2], NULL, 10)}});
          } else {
            std::cout << args[0] << std::endl;
            logger->error("Invalid control algorithm");
          }
          while (this->m_continue) {
            ca->run();
            std::this_thread::sleep_for(periodicity);
          }
        }});
  }

  ~Controller() {
    m_continue.store(false);
    for (auto& ca : m_control_algorithms) {
      ca.join();
    }
    delete proxy;
    logger->info("Destruction");
  }
};
} // namespace holpaca::control_plane