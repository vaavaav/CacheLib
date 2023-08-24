#pragma once
#include <holpaca/common/cache.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <optional>
#include <unordered_map>

using namespace holpaca::common;

namespace holpaca::control_algorithm {
class ProportionalShare {
  ProportionalShare() = delete;
  Cache* cache;
  std::shared_ptr<spdlog::logger> m_logger;
  std::vector<id_t> pool_ids;
  int left{0};
  int max{0};
  std::unordered_map<id_t, int> priorities = {{0, 1}, {1, 1}, {2, 4}, {3, 4}};
  std::unordered_map<id_t, int> rate;
  std::vector<id_t> inactive;

  void collect() {
    m_logger->info("Collecting cache status");
    if (cache == NULL) {
      m_logger->warn("error while accessing cache object");
      return;
    }
    pool_ids.clear();
    inactive.clear();
    rate.clear();
    max = cache->size();
    left = max;
    for (auto const& [pid, status] : cache->getStatus()) {
      if (status.isActive) {
        pool_ids.push_back(pid);
        m_logger->info("pool={} is active, curr size={}", pid, status.usedMem);
      } else {
        inactive.push_back(pid);
      }
    }
  }

  void compute() {
    // for (auto const& pid : pool_ids){
    //     rate[pid] = left / (static_cast<int>(pid) + 2);
    //     m_logger->info("pool={} rate={}", pid, rate[pid]);
    //     left -= rate[pid];
    //     m_logger->info("left={}", left);
    // }
    // for (auto const& pid : pool_ids){
    //     rate[pid] += left / pool_ids.size();
    // }
    int partitions = 0;
    for (auto const& pid : pool_ids) {
      partitions += priorities[pid];
    }
    for (auto const& pid : pool_ids) {
      rate[pid] = static_cast<int>(priorities[pid] *
                                   (static_cast<double>(max) / partitions));
      left -= rate[pid];
    }
    for (auto const& pid : pool_ids) {
      rate[pid] += left / pool_ids.size();
    }
  }
  void enforce() {
    for (auto const& [pool_id, new_size] : rate) {
      cache->resize(pool_id, new_size);
      m_logger->info("pool={} newsize={}", pool_id, new_size);
    }
    for (auto const& pid : inactive) {
      cache->resize(pid, 0);
    }
  }

 public:
 void run() {
    collect();
    compute();
    enforce();
  }
  ProportionalShare(Cache* cache) : cache(cache) {
    try {
      m_logger = spdlog::basic_logger_st("ps", "/tmp/ps.log", true);
    } catch (const spdlog::spdlog_ex& ex) {
      std::cerr << "Log init failed: " << ex.what() << std::endl;
      abort();
    }
    m_logger->flush_on(spdlog::level::trace);
    m_logger->set_level(spdlog::level::debug);
    m_logger->info("Initialization");
  }
  ~ProportionalShare() { m_logger->info("Destructing ProportionalShare"); }
};
} // namespace holpaca::control_algorithm