#pragma once

#include <fstream>
#include <atomic>

#include "cachelib/allocator/Cache.h"
#include "cachelib/allocator/RebalanceStrategy.h"
#include "cachelib/allocator/SlabReleaseStats.h"
#include "cachelib/common/PeriodicWorker.h"

namespace facebook {
namespace cachelib {

class Tracker: public PeriodicWorker {
 public:
  // @param cache                 the cache interace
  Tracker(CacheBase& cache, std::string pathToFile) : cache_(cache) {
    log.open(pathToFile);
  }

  ~Tracker() override {
    log.close();
    stop(std::chrono::milliseconds(0));    
  }
 private:
  CacheBase& cache_;
  std::ofstream log;

  void work() override final {
       for (auto const& id : cache_.getPoolIds()) {
        auto pool_stats = cache_.getPoolStats(id);
        log << fmt::format("pool-{} {{ usedMem={} freeMem={} }} ", id, pool_stats.poolSize, pool_stats.freeMemoryBytes());
      }
      log << std::endl;
  }
};
} // namespace cachelib
} // namespace facebook
