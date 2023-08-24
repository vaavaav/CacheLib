#pragma once
#include <holpaca/common/cache.h>
#include <holpaca/config.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>

using holpaca::common::Cache;

namespace holpaca::control_algorithm {
class NaiveControlAlgorithm {
  NaiveControlAlgorithm() = delete;
  Cache* cache;
  std::shared_ptr<spdlog::logger> m_logger;

  struct Stats {
    uint32_t hits;
    size_t size;
  };
  std::unordered_map<id_t, Stats> stats;
  struct Solution {
    Id giver;
    size_t delta;
    Id taker;
  };
  std::optional<Solution> solution;
  void collect();
  void compute();
  void enforce();

 public:
  float const hit_rate_threashold{0.3f};
  NaiveControlAlgorithm(Cache* cache) : cache(cache) {
    try {
      m_logger =
          spdlog::basic_logger_st("naive", "/tmp/naive.log", true);
    } catch (const spdlog::spdlog_ex& ex) {
      std::cerr << "Log init failed: " << ex.what() << std::endl;
      abort();
    }
    m_logger->flush_on(spdlog::level::trace);
    m_logger->set_level(spdlog::level::debug);
    m_logger->info("Initialization");
  }
  ~NaiveControlAlgorithm() {
    m_logger->info("Destructing NaiveControlAlgorithm");
  }
};
} // namespace holpaca::control_algorithm
