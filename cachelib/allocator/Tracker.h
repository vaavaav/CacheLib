#pragma once

#include <atomic>
#include <fstream>
#include <set>

#include "cachelib/allocator/Cache.h"
#include "cachelib/allocator/RebalanceStrategy.h"
#include "cachelib/allocator/SlabReleaseStats.h"
#include "cachelib/common/PeriodicWorker.h"

namespace facebook {
namespace cachelib {

class Tracker : public PeriodicWorker {
 public:
  // @param cache                 the cache interace
  Tracker(CacheBase& cache, std::string pathToFile) : cache_(cache) {
    log.open(pathToFile);
    log << fmt::format("cacheSize: {}",
                       cache_.getCacheMemoryStats().ramCacheSize)
        << std::endl;
  }

  ~Tracker() override {
    log.close();
    stop(std::chrono::milliseconds(0));
  }

 private:
  CacheBase& cache_;
  std::ofstream log;
  std::chrono::_V2::system_clock::time_point time{
      std::chrono::high_resolution_clock::now()};

  void work() override final {
    auto pools = cache_.getActivePools();
    log << fmt::format("{}: ",
                       std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::high_resolution_clock::now() - time)
                           .count());
    for (auto const& id : cache_.getPoolIds()) {
      auto pool_stats = cache_.getPoolStats(id);
      bool active = pools.find(id) != pools.end();
      log << fmt::format("pool-{} {{ usedMem={} freeMem={} }} ", id,
                         active ? pool_stats.poolSize : 0,
                         active ? pool_stats.freeMemoryBytes() : 0);
    }
    log << std::endl;
  }
};
} // namespace cachelib
} // namespace facebook
